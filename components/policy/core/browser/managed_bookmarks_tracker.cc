// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/browser/managed_bookmarks_tracker.h"

#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/prefs/pref_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace policy {

const char ManagedBookmarksTracker::kName[] = "name";
const char ManagedBookmarksTracker::kUrl[] = "url";
const char ManagedBookmarksTracker::kChildren[] = "children";

ManagedBookmarksTracker::ManagedBookmarksTracker(
    BookmarkModel* model,
    PrefService* prefs,
    const GetManagementDomainCallback& callback)
    : model_(model),
      managed_node_(NULL),
      prefs_(prefs),
      get_management_domain_callback_(callback) {
}

ManagedBookmarksTracker::~ManagedBookmarksTracker() {}

scoped_ptr<base::ListValue>
ManagedBookmarksTracker::GetInitialManagedBookmarks() {
  const base::ListValue* list =
      prefs_->GetList(bookmarks::prefs::kManagedBookmarks);
  return make_scoped_ptr(list->DeepCopy());
}

// static
int64 ManagedBookmarksTracker::LoadInitial(BookmarkNode* folder,
                                           const base::ListValue* list,
                                           int64 next_node_id) {
  for (size_t i = 0; i < list->GetSize(); ++i) {
    // Extract the data for the next bookmark from the |list|.
    base::string16 title;
    GURL url;
    const base::ListValue* children = NULL;
    if (!LoadBookmark(list, i, &title, &url, &children))
      continue;

    BookmarkNode* child = new BookmarkNode(next_node_id++, url);
    child->SetTitle(title);
    folder->Add(child, folder->child_count());
    if (children) {
      child->set_type(BookmarkNode::FOLDER);
      child->set_date_folder_modified(base::Time::Now());
      next_node_id = LoadInitial(child, children, next_node_id);
    } else {
      child->set_type(BookmarkNode::URL);
      child->set_date_added(base::Time::Now());
    }
  }

  return next_node_id;
}

void ManagedBookmarksTracker::Init(BookmarkPermanentNode* managed_node) {
  managed_node_ = managed_node;
  registrar_.Init(prefs_);
  registrar_.Add(bookmarks::prefs::kManagedBookmarks,
                 base::Bind(&ManagedBookmarksTracker::ReloadManagedBookmarks,
                            base::Unretained(this)));
  // Reload now just in case something changed since the initial load started.
  ReloadManagedBookmarks();
}

void ManagedBookmarksTracker::ReloadManagedBookmarks() {
  // Update the managed bookmarks folder title, in case the user just signed
  // into or out of a managed account.
  base::string16 title;
  std::string domain = get_management_domain_callback_.Run();
  if (domain.empty()) {
    title =
        l10n_util::GetStringUTF16(IDS_BOOKMARK_BAR_MANAGED_FOLDER_DEFAULT_NAME);
  } else {
    title = l10n_util::GetStringFUTF16(
        IDS_BOOKMARK_BAR_MANAGED_FOLDER_DOMAIN_NAME, base::UTF8ToUTF16(domain));
  }
  model_->SetTitle(managed_node_, title);

  // Recursively update all the managed bookmarks and folders.
  const base::ListValue* list =
      prefs_->GetList(bookmarks::prefs::kManagedBookmarks);
  UpdateBookmarks(managed_node_, list);

  // The managed bookmarks folder isn't visible when that policy isn't present.
  managed_node_->set_visible(!managed_node_->empty());
}

void ManagedBookmarksTracker::UpdateBookmarks(const BookmarkNode* folder,
                                              const base::ListValue* list) {
  int folder_index = 0;
  for (size_t i = 0; i < list->GetSize(); ++i) {
    // Extract the data for the next bookmark from the |list|.
    base::string16 title;
    GURL url;
    const base::ListValue* children = NULL;
    if (!LoadBookmark(list, i, &title, &url, &children)) {
      // Skip this bookmark from |list| but don't advance |folder_index|.
      continue;
    }

    // Look for a bookmark at |folder_index| or ahead that matches the current
    // bookmark from the pref.
    const BookmarkNode* existing = NULL;
    for (int k = folder_index; k < folder->child_count(); ++k) {
      const BookmarkNode* node = folder->GetChild(k);
      if (node->GetTitle() == title &&
          ((children && node->is_folder()) ||
           (!children && node->url() == url))) {
        existing = node;
        break;
      }
    }

    if (existing) {
      // Reuse the existing node. The Move() is a nop if |existing| is already
      // at |folder_index|.
      model_->Move(existing, folder, folder_index);
      if (children)
        UpdateBookmarks(existing, children);
    } else {
      // Create a new node for this bookmark now.
      if (children) {
        const BookmarkNode* sub =
            model_->AddFolder(folder, folder_index, title);
        UpdateBookmarks(sub, children);
      } else {
        model_->AddURL(folder, folder_index, title, url);
      }
    }

    // The |folder_index| index of |folder| has been updated, so advance it.
    ++folder_index;
  }

  // Remove any extra children of |folder| that haven't been reused.
  while (folder->child_count() != folder_index)
    model_->Remove(folder, folder_index);
}

// static
bool ManagedBookmarksTracker::LoadBookmark(const base::ListValue* list,
                                           size_t index,
                                           base::string16* title,
                                           GURL* url,
                                           const base::ListValue** children) {
  std::string spec;
  *url = GURL();
  *children = NULL;
  const base::DictionaryValue* dict = NULL;
  if (!list->GetDictionary(index, &dict) ||
      !dict->GetString(kName, title) ||
      (!dict->GetString(kUrl, &spec) &&
       !dict->GetList(kChildren, children))) {
    // Should never happen after policy validation.
    NOTREACHED();
    return false;
  }
  if (!*children)
    *url = GURL(spec);
  return true;
}

}  // namespace policy
