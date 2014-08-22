// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "chrome/browser/extensions/api/bookmarks/bookmarks_api.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/i18n/file_util_icu.h"
#include "base/i18n/time_formatting.h"
#include "base/lazy_instance.h"
#include "base/memory/scoped_ptr.h"
#include "base/path_service.h"
#include "base/prefs/pref_service.h"
#include "base/rand_util.h"
#include "base/sha1.h"
#include "base/stl_util.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/bookmarks/bookmark_html_writer.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/bookmarks/chrome_bookmark_client.h"
#include "chrome/browser/bookmarks/chrome_bookmark_client_factory.h"
#include "chrome/browser/extensions/api/bookmarks/bookmark_api_constants.h"
#include "chrome/browser/extensions/api/bookmarks/bookmark_api_helpers.h"
#include "chrome/browser/importer/external_process_importer_host.h"
#include "chrome/browser/importer/importer_uma.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/browser/ui/host_desktop.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/api/bookmarks.h"
#include "chrome/common/importer/importer_data_types.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_function_dispatcher.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/permissions/permissions_data.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(OS_WIN)
#include "ui/aura/remote_window_tree_host_win.h"
#endif

namespace extensions {

namespace keys = bookmark_api_constants;
namespace bookmarks = api::bookmarks;

using bookmarks::BookmarkTreeNode;
using bookmarks::CreateDetails;
using content::BrowserContext;
using content::BrowserThread;
using content::WebContents;

namespace {

// Generates a default path (including a default filename) that will be
// used for pre-populating the "Export Bookmarks" file chooser dialog box.
base::FilePath GetDefaultFilepathForBookmarkExport() {
  base::Time time = base::Time::Now();

  // Concatenate a date stamp to the filename.
#if defined(OS_POSIX)
  base::FilePath::StringType filename =
      l10n_util::GetStringFUTF8(IDS_EXPORT_BOOKMARKS_DEFAULT_FILENAME,
                                base::TimeFormatShortDateNumeric(time));
#elif defined(OS_WIN)
  base::FilePath::StringType filename =
      l10n_util::GetStringFUTF16(IDS_EXPORT_BOOKMARKS_DEFAULT_FILENAME,
                                 base::TimeFormatShortDateNumeric(time));
#endif

  base::i18n::ReplaceIllegalCharactersInPath(&filename, '_');

  base::FilePath default_path;
  PathService::Get(chrome::DIR_USER_DOCUMENTS, &default_path);
  return default_path.Append(filename);
}

bool IsEnhancedBookmarksExtensionActive(Profile* profile) {
  static const char *enhanced_extension_hashes[] = {
    "D5736E4B5CF695CB93A2FB57E4FDC6E5AFAB6FE2",  // http://crbug.com/312900
    "D57DE394F36DC1C3220E7604C575D29C51A6C495",  // http://crbug.com/319444
    "3F65507A3B39259B38C8173C6FFA3D12DF64CCE9"   // http://crbug.com/371562
  };
  const ExtensionSet& extensions =
      ExtensionRegistry::Get(profile)->enabled_extensions();
  for (ExtensionSet::const_iterator it = extensions.begin();
       it != extensions.end(); ++it) {
    const Extension* extension = *it;
    if (extension->permissions_data()->HasAPIPermission(
            APIPermission::kBookmarkManagerPrivate)) {
      std::string hash = base::SHA1HashString(extension->id());
      hash = base::HexEncode(hash.c_str(), hash.length());
      for (size_t i = 0; i < arraysize(enhanced_extension_hashes); i++)
        if (hash == enhanced_extension_hashes[i])
          return true;
    }
  }
  return false;
}

std::string ToBase36(int64 value) {
  DCHECK(value >= 0);
  std::string str;
  while (value > 0) {
    int digit = value % 36;
    value /= 36;
    str += (digit < 10 ? '0' + digit : 'a' + digit - 10);
  }
  std::reverse(str.begin(), str.end());
  return str;
}

// Generate a metadata ID based on a the current time and a random number for
// enhanced bookmarks, to be assigned pre-sync.
std::string GenerateEnhancedBookmarksID(bool is_folder) {
  static const char bookmark_prefix[] = "cc_";
  static const char folder_prefix[] = "cf_";
  // Use [0..range_mid) for bookmarks, [range_mid..2*range_mid) for folders.
  int range_mid = 36*36*36*36 / 2;
  int rand = base::RandInt(0, range_mid - 1);
  int64 unix_epoch_time_in_ms =
      (base::Time::Now() - base::Time::UnixEpoch()).InMilliseconds();
  return std::string(is_folder ? folder_prefix : bookmark_prefix) +
      ToBase36(is_folder ? range_mid + rand : rand) +
      ToBase36(unix_epoch_time_in_ms);
}

}  // namespace

bool BookmarksFunction::RunAsync() {
  BookmarkModel* model = BookmarkModelFactory::GetForProfile(GetProfile());
  if (!model->loaded()) {
    // Bookmarks are not ready yet.  We'll wait.
    model->AddObserver(this);
    AddRef();  // Balanced in Loaded().
    return true;
  }

  bool success = RunOnReady();
  if (success) {
    content::NotificationService::current()->Notify(
        extensions::NOTIFICATION_EXTENSION_BOOKMARKS_API_INVOKED,
        content::Source<const Extension>(extension()),
        content::Details<const BookmarksFunction>(this));
  }
  SendResponse(success);
  return true;
}

BookmarkModel* BookmarksFunction::GetBookmarkModel() {
  return BookmarkModelFactory::GetForProfile(GetProfile());
}

ChromeBookmarkClient* BookmarksFunction::GetChromeBookmarkClient() {
  return ChromeBookmarkClientFactory::GetForProfile(GetProfile());
}

bool BookmarksFunction::GetBookmarkIdAsInt64(const std::string& id_string,
                                             int64* id) {
  if (base::StringToInt64(id_string, id))
    return true;

  error_ = keys::kInvalidIdError;
  return false;
}

const BookmarkNode* BookmarksFunction::GetBookmarkNodeFromId(
    const std::string& id_string) {
  int64 id;
  if (!GetBookmarkIdAsInt64(id_string, &id))
    return NULL;

  const BookmarkNode* node = ::bookmarks::GetBookmarkNodeByID(
      BookmarkModelFactory::GetForProfile(GetProfile()), id);
  if (!node)
    error_ = keys::kNoNodeError;

  return node;
}

const BookmarkNode* BookmarksFunction::CreateBookmarkNode(
    BookmarkModel* model,
    const CreateDetails& details,
    const BookmarkNode::MetaInfoMap* meta_info) {
  int64 parentId;

  if (!details.parent_id.get()) {
    // Optional, default to "other bookmarks".
    parentId = model->other_node()->id();
  } else {
    if (!GetBookmarkIdAsInt64(*details.parent_id, &parentId))
      return NULL;
  }
  const BookmarkNode* parent =
      ::bookmarks::GetBookmarkNodeByID(model, parentId);
  if (!CanBeModified(parent))
    return NULL;

  int index;
  if (!details.index.get()) {  // Optional (defaults to end).
    index = parent->child_count();
  } else {
    index = *details.index;
    if (index > parent->child_count() || index < 0) {
      error_ = keys::kInvalidIndexError;
      return NULL;
    }
  }

  base::string16 title;  // Optional.
  if (details.title.get())
    title = base::UTF8ToUTF16(*details.title.get());

  std::string url_string;  // Optional.
  if (details.url.get())
    url_string = *details.url.get();

  GURL url(url_string);
  if (!url_string.empty() && !url.is_valid()) {
    error_ = keys::kInvalidUrlError;
    return NULL;
  }

  const BookmarkNode* node;
  if (url_string.length())
    node = model->AddURLWithCreationTimeAndMetaInfo(
        parent, index, title, url, base::Time::Now(), meta_info);
  else
    node = model->AddFolderWithMetaInfo(parent, index, title, meta_info);
  DCHECK(node);
  if (!node) {
    error_ = keys::kNoNodeError;
    return NULL;
  }

  return node;
}

bool BookmarksFunction::EditBookmarksEnabled() {
  PrefService* prefs = user_prefs::UserPrefs::Get(GetProfile());
  if (prefs->GetBoolean(prefs::kEditBookmarksEnabled))
    return true;
  error_ = keys::kEditBookmarksDisabled;
  return false;
}

bool BookmarksFunction::CanBeModified(const BookmarkNode* node) {
  if (!node) {
    error_ = keys::kNoParentError;
    return false;
  }
  if (node->is_root()) {
    error_ = keys::kModifySpecialError;
    return false;
  }
  ChromeBookmarkClient* client = GetChromeBookmarkClient();
  if (client->IsDescendantOfManagedNode(node)) {
    error_ = keys::kModifyManagedError;
    return false;
  }
  return true;
}

void BookmarksFunction::BookmarkModelChanged() {
}

void BookmarksFunction::BookmarkModelLoaded(BookmarkModel* model,
                                            bool ids_reassigned) {
  model->RemoveObserver(this);
  RunOnReady();
  Release();  // Balanced in RunOnReady().
}

BookmarkEventRouter::BookmarkEventRouter(Profile* profile)
    : browser_context_(profile),
      model_(BookmarkModelFactory::GetForProfile(profile)),
      client_(ChromeBookmarkClientFactory::GetForProfile(profile)) {
  model_->AddObserver(this);
}

BookmarkEventRouter::~BookmarkEventRouter() {
  if (model_) {
    model_->RemoveObserver(this);
  }
}

void BookmarkEventRouter::DispatchEvent(
    const std::string& event_name,
    scoped_ptr<base::ListValue> event_args) {
  EventRouter* event_router = EventRouter::Get(browser_context_);
  if (event_router) {
    event_router->BroadcastEvent(
        make_scoped_ptr(new extensions::Event(event_name, event_args.Pass())));
  }
}

void BookmarkEventRouter::BookmarkModelLoaded(BookmarkModel* model,
                                              bool ids_reassigned) {
  // TODO(erikkay): Perhaps we should send this event down to the extension
  // so they know when it's safe to use the API?
}

void BookmarkEventRouter::BookmarkModelBeingDeleted(BookmarkModel* model) {
  model_ = NULL;
}

void BookmarkEventRouter::BookmarkNodeMoved(BookmarkModel* model,
                                            const BookmarkNode* old_parent,
                                            int old_index,
                                            const BookmarkNode* new_parent,
                                            int new_index) {
  const BookmarkNode* node = new_parent->GetChild(new_index);
  bookmarks::OnMoved::MoveInfo move_info;
  move_info.parent_id = base::Int64ToString(new_parent->id());
  move_info.index = new_index;
  move_info.old_parent_id = base::Int64ToString(old_parent->id());
  move_info.old_index = old_index;

  DispatchEvent(
      bookmarks::OnMoved::kEventName,
      bookmarks::OnMoved::Create(base::Int64ToString(node->id()), move_info));
}

void BookmarkEventRouter::OnWillAddBookmarkNode(BookmarkModel* model,
                                                BookmarkNode* node) {
  // TODO(wittman): Remove this once extension hooks are in place to allow the
  // enhanced bookmarks extension to manage all bookmark creation code
  // paths. See http://crbug.com/383557.
  if (IsEnhancedBookmarksExtensionActive(Profile::FromBrowserContext(
          browser_context_))) {
    static const char key[] = "stars.id";
    std::string value;
    if (!node->GetMetaInfo(key, &value))
      node->SetMetaInfo(key, GenerateEnhancedBookmarksID(node->is_folder()));
  }
}

void BookmarkEventRouter::BookmarkNodeAdded(BookmarkModel* model,
                                            const BookmarkNode* parent,
                                            int index) {
  const BookmarkNode* node = parent->GetChild(index);
  scoped_ptr<BookmarkTreeNode> tree_node(
      bookmark_api_helpers::GetBookmarkTreeNode(client_, node, false, false));
  DispatchEvent(bookmarks::OnCreated::kEventName,
                bookmarks::OnCreated::Create(base::Int64ToString(node->id()),
                                             *tree_node));
}

void BookmarkEventRouter::BookmarkNodeRemoved(
    BookmarkModel* model,
    const BookmarkNode* parent,
    int index,
    const BookmarkNode* node,
    const std::set<GURL>& removed_urls) {
  bookmarks::OnRemoved::RemoveInfo remove_info;
  remove_info.parent_id = base::Int64ToString(parent->id());
  remove_info.index = index;

  DispatchEvent(bookmarks::OnRemoved::kEventName,
                bookmarks::OnRemoved::Create(base::Int64ToString(node->id()),
                                             remove_info));
}

void BookmarkEventRouter::BookmarkAllUserNodesRemoved(
    BookmarkModel* model,
    const std::set<GURL>& removed_urls) {
  NOTREACHED();
  // TODO(shashishekhar) Currently this notification is only used on Android,
  // which does not support extensions. If Desktop needs to support this, add
  // a new event to the extensions api.
}

void BookmarkEventRouter::BookmarkNodeChanged(BookmarkModel* model,
                                              const BookmarkNode* node) {
  // TODO(erikkay) The only three things that BookmarkModel sends this
  // notification for are title, url and favicon.  Since we're currently
  // ignoring favicon and since the notification doesn't say which one anyway,
  // for now we only include title and url.  The ideal thing would be to change
  // BookmarkModel to indicate what changed.
  bookmarks::OnChanged::ChangeInfo change_info;
  change_info.title = base::UTF16ToUTF8(node->GetTitle());
  if (node->is_url())
    change_info.url.reset(new std::string(node->url().spec()));

  DispatchEvent(bookmarks::OnChanged::kEventName,
                bookmarks::OnChanged::Create(base::Int64ToString(node->id()),
                                             change_info));
}

void BookmarkEventRouter::BookmarkNodeFaviconChanged(BookmarkModel* model,
                                                     const BookmarkNode* node) {
  // TODO(erikkay) anything we should do here?
}

void BookmarkEventRouter::BookmarkNodeChildrenReordered(
    BookmarkModel* model,
    const BookmarkNode* node) {
  bookmarks::OnChildrenReordered::ReorderInfo reorder_info;
  int childCount = node->child_count();
  for (int i = 0; i < childCount; ++i) {
    const BookmarkNode* child = node->GetChild(i);
    reorder_info.child_ids.push_back(base::Int64ToString(child->id()));
  }

  DispatchEvent(bookmarks::OnChildrenReordered::kEventName,
                bookmarks::OnChildrenReordered::Create(
                    base::Int64ToString(node->id()), reorder_info));
}

void BookmarkEventRouter::ExtensiveBookmarkChangesBeginning(
    BookmarkModel* model) {
  DispatchEvent(bookmarks::OnImportBegan::kEventName,
                bookmarks::OnImportBegan::Create());
}

void BookmarkEventRouter::ExtensiveBookmarkChangesEnded(BookmarkModel* model) {
  DispatchEvent(bookmarks::OnImportEnded::kEventName,
                bookmarks::OnImportEnded::Create());
}

BookmarksAPI::BookmarksAPI(BrowserContext* context)
    : browser_context_(context) {
  EventRouter* event_router = EventRouter::Get(browser_context_);
  event_router->RegisterObserver(this, bookmarks::OnCreated::kEventName);
  event_router->RegisterObserver(this, bookmarks::OnRemoved::kEventName);
  event_router->RegisterObserver(this, bookmarks::OnChanged::kEventName);
  event_router->RegisterObserver(this, bookmarks::OnMoved::kEventName);
  event_router->RegisterObserver(this,
                                 bookmarks::OnChildrenReordered::kEventName);
  event_router->RegisterObserver(this, bookmarks::OnImportBegan::kEventName);
  event_router->RegisterObserver(this, bookmarks::OnImportEnded::kEventName);
}

BookmarksAPI::~BookmarksAPI() {
}

void BookmarksAPI::Shutdown() {
  EventRouter::Get(browser_context_)->UnregisterObserver(this);
}

static base::LazyInstance<BrowserContextKeyedAPIFactory<BookmarksAPI> >
    g_factory = LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<BookmarksAPI>*
BookmarksAPI::GetFactoryInstance() {
  return g_factory.Pointer();
}

void BookmarksAPI::OnListenerAdded(const EventListenerInfo& details) {
  bookmark_event_router_.reset(
      new BookmarkEventRouter(Profile::FromBrowserContext(browser_context_)));
  EventRouter::Get(browser_context_)->UnregisterObserver(this);
}

bool BookmarksGetFunction::RunOnReady() {
  scoped_ptr<bookmarks::Get::Params> params(
      bookmarks::Get::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  std::vector<linked_ptr<BookmarkTreeNode> > nodes;
  ChromeBookmarkClient* client = GetChromeBookmarkClient();
  if (params->id_or_id_list.as_strings) {
    std::vector<std::string>& ids = *params->id_or_id_list.as_strings;
    size_t count = ids.size();
    EXTENSION_FUNCTION_VALIDATE(count > 0);
    for (size_t i = 0; i < count; ++i) {
      const BookmarkNode* node = GetBookmarkNodeFromId(ids[i]);
      if (!node)
        return false;
      bookmark_api_helpers::AddNode(client, node, &nodes, false);
    }
  } else {
    const BookmarkNode* node =
        GetBookmarkNodeFromId(*params->id_or_id_list.as_string);
    if (!node)
      return false;
    bookmark_api_helpers::AddNode(client, node, &nodes, false);
  }

  results_ = bookmarks::Get::Results::Create(nodes);
  return true;
}

bool BookmarksGetChildrenFunction::RunOnReady() {
  scoped_ptr<bookmarks::GetChildren::Params> params(
      bookmarks::GetChildren::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const BookmarkNode* node = GetBookmarkNodeFromId(params->id);
  if (!node)
    return false;

  std::vector<linked_ptr<BookmarkTreeNode> > nodes;
  int child_count = node->child_count();
  for (int i = 0; i < child_count; ++i) {
    const BookmarkNode* child = node->GetChild(i);
    bookmark_api_helpers::AddNode(
        GetChromeBookmarkClient(), child, &nodes, false);
  }

  results_ = bookmarks::GetChildren::Results::Create(nodes);
  return true;
}

bool BookmarksGetRecentFunction::RunOnReady() {
  scoped_ptr<bookmarks::GetRecent::Params> params(
      bookmarks::GetRecent::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  if (params->number_of_items < 1)
    return false;

  std::vector<const BookmarkNode*> nodes;
  ::bookmarks::GetMostRecentlyAddedEntries(
      BookmarkModelFactory::GetForProfile(GetProfile()),
      params->number_of_items,
      &nodes);

  std::vector<linked_ptr<BookmarkTreeNode> > tree_nodes;
  std::vector<const BookmarkNode*>::iterator i = nodes.begin();
  for (; i != nodes.end(); ++i) {
    const BookmarkNode* node = *i;
    bookmark_api_helpers::AddNode(
        GetChromeBookmarkClient(), node, &tree_nodes, false);
  }

  results_ = bookmarks::GetRecent::Results::Create(tree_nodes);
  return true;
}

bool BookmarksGetTreeFunction::RunOnReady() {
  std::vector<linked_ptr<BookmarkTreeNode> > nodes;
  const BookmarkNode* node =
      BookmarkModelFactory::GetForProfile(GetProfile())->root_node();
  bookmark_api_helpers::AddNode(GetChromeBookmarkClient(), node, &nodes, true);
  results_ = bookmarks::GetTree::Results::Create(nodes);
  return true;
}

bool BookmarksGetSubTreeFunction::RunOnReady() {
  scoped_ptr<bookmarks::GetSubTree::Params> params(
      bookmarks::GetSubTree::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const BookmarkNode* node = GetBookmarkNodeFromId(params->id);
  if (!node)
    return false;

  std::vector<linked_ptr<BookmarkTreeNode> > nodes;
  bookmark_api_helpers::AddNode(GetChromeBookmarkClient(), node, &nodes, true);
  results_ = bookmarks::GetSubTree::Results::Create(nodes);
  return true;
}

bool BookmarksSearchFunction::RunOnReady() {
  scoped_ptr<bookmarks::Search::Params> params(
      bookmarks::Search::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  PrefService* prefs = user_prefs::UserPrefs::Get(GetProfile());
  std::string lang = prefs->GetString(prefs::kAcceptLanguages);
  std::vector<const BookmarkNode*> nodes;
  if (params->query.as_string) {
    ::bookmarks::QueryFields query;
    query.word_phrase_query.reset(
        new base::string16(base::UTF8ToUTF16(*params->query.as_string)));
    ::bookmarks::GetBookmarksMatchingProperties(
        BookmarkModelFactory::GetForProfile(GetProfile()),
        query,
        std::numeric_limits<int>::max(),
        lang,
        &nodes);
  } else {
    DCHECK(params->query.as_object);
    const bookmarks::Search::Params::Query::Object& object =
        *params->query.as_object;
    ::bookmarks::QueryFields query;
    if (object.query) {
      query.word_phrase_query.reset(
          new base::string16(base::UTF8ToUTF16(*object.query)));
    }
    if (object.url)
      query.url.reset(new base::string16(base::UTF8ToUTF16(*object.url)));
    if (object.title)
      query.title.reset(new base::string16(base::UTF8ToUTF16(*object.title)));
    ::bookmarks::GetBookmarksMatchingProperties(
        BookmarkModelFactory::GetForProfile(GetProfile()),
        query,
        std::numeric_limits<int>::max(),
        lang,
        &nodes);
  }

  std::vector<linked_ptr<BookmarkTreeNode> > tree_nodes;
  ChromeBookmarkClient* client = GetChromeBookmarkClient();
  for (std::vector<const BookmarkNode*>::iterator node_iter = nodes.begin();
       node_iter != nodes.end(); ++node_iter) {
    bookmark_api_helpers::AddNode(client, *node_iter, &tree_nodes, false);
  }

  results_ = bookmarks::Search::Results::Create(tree_nodes);
  return true;
}

// static
bool BookmarksRemoveFunction::ExtractIds(const base::ListValue* args,
                                         std::list<int64>* ids,
                                         bool* invalid_id) {
  std::string id_string;
  if (!args->GetString(0, &id_string))
    return false;
  int64 id;
  if (base::StringToInt64(id_string, &id))
    ids->push_back(id);
  else
    *invalid_id = true;
  return true;
}

bool BookmarksRemoveFunction::RunOnReady() {
  if (!EditBookmarksEnabled())
    return false;

  scoped_ptr<bookmarks::Remove::Params> params(
      bookmarks::Remove::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  int64 id;
  if (!GetBookmarkIdAsInt64(params->id, &id))
    return false;

  bool recursive = false;
  if (name() == BookmarksRemoveTreeFunction::function_name())
    recursive = true;

  BookmarkModel* model = GetBookmarkModel();
  ChromeBookmarkClient* client = GetChromeBookmarkClient();
  if (!bookmark_api_helpers::RemoveNode(model, client, id, recursive, &error_))
    return false;

  return true;
}

bool BookmarksCreateFunction::RunOnReady() {
  if (!EditBookmarksEnabled())
    return false;

  scoped_ptr<bookmarks::Create::Params> params(
      bookmarks::Create::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  BookmarkModel* model = BookmarkModelFactory::GetForProfile(GetProfile());
  const BookmarkNode* node = CreateBookmarkNode(model, params->bookmark, NULL);
  if (!node)
    return false;

  scoped_ptr<BookmarkTreeNode> ret(bookmark_api_helpers::GetBookmarkTreeNode(
      GetChromeBookmarkClient(), node, false, false));
  results_ = bookmarks::Create::Results::Create(*ret);

  return true;
}

// static
bool BookmarksMoveFunction::ExtractIds(const base::ListValue* args,
                                       std::list<int64>* ids,
                                       bool* invalid_id) {
  // For now, Move accepts ID parameters in the same way as an Update.
  return BookmarksUpdateFunction::ExtractIds(args, ids, invalid_id);
}

bool BookmarksMoveFunction::RunOnReady() {
  if (!EditBookmarksEnabled())
    return false;

  scoped_ptr<bookmarks::Move::Params> params(
      bookmarks::Move::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const BookmarkNode* node = GetBookmarkNodeFromId(params->id);
  if (!node)
    return false;

  BookmarkModel* model = BookmarkModelFactory::GetForProfile(GetProfile());
  if (model->is_permanent_node(node)) {
    error_ = keys::kModifySpecialError;
    return false;
  }

  const BookmarkNode* parent = NULL;
  if (!params->destination.parent_id.get()) {
    // Optional, defaults to current parent.
    parent = node->parent();
  } else {
    int64 parentId;
    if (!GetBookmarkIdAsInt64(*params->destination.parent_id, &parentId))
      return false;

    parent = ::bookmarks::GetBookmarkNodeByID(model, parentId);
  }
  if (!CanBeModified(parent) || !CanBeModified(node))
    return false;

  int index;
  if (params->destination.index.get()) {  // Optional (defaults to end).
    index = *params->destination.index;
    if (index > parent->child_count() || index < 0) {
      error_ = keys::kInvalidIndexError;
      return false;
    }
  } else {
    index = parent->child_count();
  }

  model->Move(node, parent, index);

  scoped_ptr<BookmarkTreeNode> tree_node(
      bookmark_api_helpers::GetBookmarkTreeNode(
          GetChromeBookmarkClient(), node, false, false));
  results_ = bookmarks::Move::Results::Create(*tree_node);

  return true;
}

// static
bool BookmarksUpdateFunction::ExtractIds(const base::ListValue* args,
                                         std::list<int64>* ids,
                                         bool* invalid_id) {
  // For now, Update accepts ID parameters in the same way as an Remove.
  return BookmarksRemoveFunction::ExtractIds(args, ids, invalid_id);
}

bool BookmarksUpdateFunction::RunOnReady() {
  if (!EditBookmarksEnabled())
    return false;

  scoped_ptr<bookmarks::Update::Params> params(
      bookmarks::Update::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  // Optional but we need to distinguish non present from an empty title.
  base::string16 title;
  bool has_title = false;
  if (params->changes.title.get()) {
    title = base::UTF8ToUTF16(*params->changes.title);
    has_title = true;
  }

  // Optional.
  std::string url_string;
  if (params->changes.url.get())
    url_string = *params->changes.url;
  GURL url(url_string);
  if (!url_string.empty() && !url.is_valid()) {
    error_ = keys::kInvalidUrlError;
    return false;
  }

  const BookmarkNode* node = GetBookmarkNodeFromId(params->id);
  if (!CanBeModified(node))
    return false;

  BookmarkModel* model = BookmarkModelFactory::GetForProfile(GetProfile());
  if (model->is_permanent_node(node)) {
    error_ = keys::kModifySpecialError;
    return false;
  }
  if (has_title)
    model->SetTitle(node, title);
  if (!url.is_empty())
    model->SetURL(node, url);

  scoped_ptr<BookmarkTreeNode> tree_node(
      bookmark_api_helpers::GetBookmarkTreeNode(
          GetChromeBookmarkClient(), node, false, false));
  results_ = bookmarks::Update::Results::Create(*tree_node);
  return true;
}

BookmarksIOFunction::BookmarksIOFunction() {}

BookmarksIOFunction::~BookmarksIOFunction() {
  // There may be pending file dialogs, we need to tell them that we've gone
  // away so they don't try and call back to us.
  if (select_file_dialog_.get())
    select_file_dialog_->ListenerDestroyed();
}

void BookmarksIOFunction::SelectFile(ui::SelectFileDialog::Type type) {
  // GetDefaultFilepathForBookmarkExport() might have to touch the filesystem
  // (stat or access, for example), so this requires a thread with IO allowed.
  if (!BrowserThread::CurrentlyOn(BrowserThread::FILE)) {
    BrowserThread::PostTask(BrowserThread::FILE, FROM_HERE,
        base::Bind(&BookmarksIOFunction::SelectFile, this, type));
    return;
  }

  // Pre-populating the filename field in case this is a SELECT_SAVEAS_FILE
  // dialog. If not, there is no filename field in the dialog box.
  base::FilePath default_path;
  if (type == ui::SelectFileDialog::SELECT_SAVEAS_FILE)
    default_path = GetDefaultFilepathForBookmarkExport();
  else
    DCHECK(type == ui::SelectFileDialog::SELECT_OPEN_FILE);

  // After getting the |default_path|, ask the UI to display the file dialog.
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      base::Bind(&BookmarksIOFunction::ShowSelectFileDialog, this,
                 type, default_path));
}

void BookmarksIOFunction::ShowSelectFileDialog(
    ui::SelectFileDialog::Type type,
    const base::FilePath& default_path) {
  if (!dispatcher())
    return;  // Extension was unloaded.

  // Balanced in one of the three callbacks of SelectFileDialog:
  // either FileSelectionCanceled, MultiFilesSelected, or FileSelected
  AddRef();

  WebContents* web_contents = dispatcher()->delegate()->
      GetAssociatedWebContents();

  select_file_dialog_ = ui::SelectFileDialog::Create(
      this, new ChromeSelectFilePolicy(web_contents));
  ui::SelectFileDialog::FileTypeInfo file_type_info;
  file_type_info.extensions.resize(1);
  file_type_info.extensions[0].push_back(FILE_PATH_LITERAL("html"));
  gfx::NativeWindow owning_window = web_contents ?
      platform_util::GetTopLevel(web_contents->GetNativeView())
          : NULL;
#if defined(OS_WIN)
  if (!owning_window &&
      chrome::GetActiveDesktop() == chrome::HOST_DESKTOP_TYPE_ASH)
    owning_window = aura::RemoteWindowTreeHostWin::Instance()->GetAshWindow();
#endif
  // |web_contents| can be NULL (for background pages), which is fine. In such
  // a case if file-selection dialogs are forbidden by policy, we will not
  // show an InfoBar, which is better than letting one appear out of the blue.
  select_file_dialog_->SelectFile(type,
                                  base::string16(),
                                  default_path,
                                  &file_type_info,
                                  0,
                                  base::FilePath::StringType(),
                                  owning_window,
                                  NULL);
}

void BookmarksIOFunction::FileSelectionCanceled(void* params) {
  Release();  // Balanced in BookmarksIOFunction::SelectFile()
}

void BookmarksIOFunction::MultiFilesSelected(
    const std::vector<base::FilePath>& files, void* params) {
  Release();  // Balanced in BookmarsIOFunction::SelectFile()
  NOTREACHED() << "Should not be able to select multiple files";
}

bool BookmarksImportFunction::RunOnReady() {
  if (!EditBookmarksEnabled())
    return false;
  SelectFile(ui::SelectFileDialog::SELECT_OPEN_FILE);
  return true;
}

void BookmarksImportFunction::FileSelected(const base::FilePath& path,
                                           int index,
                                           void* params) {
  // Deletes itself.
  ExternalProcessImporterHost* importer_host = new ExternalProcessImporterHost;
  importer::SourceProfile source_profile;
  source_profile.importer_type = importer::TYPE_BOOKMARKS_FILE;
  source_profile.source_path = path;
  importer_host->StartImportSettings(source_profile,
                                     GetProfile(),
                                     importer::FAVORITES,
                                     new ProfileWriter(GetProfile()));

  importer::LogImporterUseToMetrics("BookmarksAPI",
                                    importer::TYPE_BOOKMARKS_FILE);
  Release();  // Balanced in BookmarksIOFunction::SelectFile()
}

bool BookmarksExportFunction::RunOnReady() {
  SelectFile(ui::SelectFileDialog::SELECT_SAVEAS_FILE);
  return true;
}

void BookmarksExportFunction::FileSelected(const base::FilePath& path,
                                           int index,
                                           void* params) {
  bookmark_html_writer::WriteBookmarks(GetProfile(), path, NULL);
  Release();  // Balanced in BookmarksIOFunction::SelectFile()
}

}  // namespace extensions
