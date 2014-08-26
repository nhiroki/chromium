// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Disable everything on windows only. http://crbug.com/306144
#ifndef OS_WIN

#include <algorithm>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/message_loop/message_loop.h"
#include "base/prefs/pref_service.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/waitable_event.h"
#include "chrome/browser/download/download_file_icon_extractor.h"
#include "chrome/browser/download/download_service.h"
#include "chrome/browser/download/download_service_factory.h"
#include "chrome/browser/download/download_test_file_activity_observer.h"
#include "chrome/browser/extensions/api/downloads/downloads_api.h"
#include "chrome/browser/extensions/browser_action_test_util.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/history/download_row.h"
#include "chrome/browser/net/url_request_mock_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/page_transition_types.h"
#include "content/public/test/download_test_observer.h"
#include "content/test/net/url_request_slow_download_job.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/notification_types.h"
#include "net/base/data_url.h"
#include "net/base/net_util.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_job.h"
#include "net/url_request/url_request_job_factory.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "webkit/browser/fileapi/file_system_context.h"
#include "webkit/browser/fileapi/file_system_operation_runner.h"
#include "webkit/browser/fileapi/file_system_url.h"

using content::BrowserContext;
using content::BrowserThread;
using content::DownloadItem;
using content::DownloadManager;
using content::URLRequestSlowDownloadJob;

namespace errors = download_extension_errors;

namespace extensions {
namespace downloads = api::downloads;

namespace {

// Comparator that orders download items by their ID. Can be used with
// std::sort.
struct DownloadIdComparator {
  bool operator() (DownloadItem* first, DownloadItem* second) {
    return first->GetId() < second->GetId();
  }
};

class DownloadsEventsListener : public content::NotificationObserver {
 public:
  DownloadsEventsListener()
    : waiting_(false) {
    registrar_.Add(this,
                   extensions::NOTIFICATION_EXTENSION_DOWNLOADS_EVENT,
                   content::NotificationService::AllSources());
  }

  virtual ~DownloadsEventsListener() {
    registrar_.Remove(this,
                      extensions::NOTIFICATION_EXTENSION_DOWNLOADS_EVENT,
                      content::NotificationService::AllSources());
    STLDeleteElements(&events_);
  }

  void ClearEvents() {
    STLDeleteElements(&events_);
    events_.clear();
  }

  class Event {
   public:
    Event(Profile* profile,
          const std::string& event_name,
          const std::string& json_args,
          base::Time caught)
      : profile_(profile),
        event_name_(event_name),
        json_args_(json_args),
        args_(base::JSONReader::Read(json_args)),
        caught_(caught) {
    }

    const base::Time& caught() { return caught_; }

    bool Satisfies(const Event& other) const {
      return other.SatisfiedBy(*this);
    }

    bool SatisfiedBy(const Event& other) const {
      if ((profile_ != other.profile_) ||
          (event_name_ != other.event_name_))
        return false;
      if (((event_name_ == downloads::OnDeterminingFilename::kEventName) ||
           (event_name_ == downloads::OnCreated::kEventName) ||
           (event_name_ == downloads::OnChanged::kEventName)) &&
          args_.get() && other.args_.get()) {
        base::ListValue* left_list = NULL;
        base::DictionaryValue* left_dict = NULL;
        base::ListValue* right_list = NULL;
        base::DictionaryValue* right_dict = NULL;
        if (!args_->GetAsList(&left_list) ||
            !other.args_->GetAsList(&right_list) ||
            !left_list->GetDictionary(0, &left_dict) ||
            !right_list->GetDictionary(0, &right_dict))
          return false;
        for (base::DictionaryValue::Iterator iter(*left_dict);
             !iter.IsAtEnd(); iter.Advance()) {
          base::Value* right_value = NULL;
          if (!right_dict->HasKey(iter.key()) ||
              (right_dict->Get(iter.key(), &right_value) &&
               !iter.value().Equals(right_value))) {
            return false;
          }
        }
        return true;
      } else if ((event_name_ == downloads::OnErased::kEventName) &&
                 args_.get() && other.args_.get()) {
        int my_id = -1, other_id = -1;
        return (args_->GetAsInteger(&my_id) &&
                other.args_->GetAsInteger(&other_id) &&
                my_id == other_id);
      }
      return json_args_ == other.json_args_;
    }

    std::string Debug() {
      return base::StringPrintf("Event(%p, %s, %s, %f)",
                                profile_,
                                event_name_.c_str(),
                                json_args_.c_str(),
                                caught_.ToJsTime());
    }

   private:
    Profile* profile_;
    std::string event_name_;
    std::string json_args_;
    scoped_ptr<base::Value> args_;
    base::Time caught_;

    DISALLOW_COPY_AND_ASSIGN(Event);
  };

  typedef ExtensionDownloadsEventRouter::DownloadsNotificationSource
    DownloadsNotificationSource;

  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE {
    switch (type) {
      case extensions::NOTIFICATION_EXTENSION_DOWNLOADS_EVENT: {
          DownloadsNotificationSource* dns =
              content::Source<DownloadsNotificationSource>(source).ptr();
          Event* new_event = new Event(
              dns->profile,
              dns->event_name,
              *content::Details<std::string>(details).ptr(), base::Time::Now());
          events_.push_back(new_event);
          if (waiting_ &&
              waiting_for_.get() &&
              new_event->Satisfies(*waiting_for_)) {
            waiting_ = false;
            base::MessageLoopForUI::current()->Quit();
          }
          break;
        }
      default:
        NOTREACHED();
    }
  }

  bool WaitFor(Profile* profile,
               const std::string& event_name,
               const std::string& json_args) {
    waiting_for_.reset(new Event(profile, event_name, json_args, base::Time()));
    for (std::deque<Event*>::const_iterator iter = events_.begin();
         iter != events_.end(); ++iter) {
      if ((*iter)->Satisfies(*waiting_for_.get())) {
        return true;
      }
    }
    waiting_ = true;
    content::RunMessageLoop();
    bool success = !waiting_;
    if (waiting_) {
      // Print the events that were caught since the last WaitFor() call to help
      // find the erroneous event.
      // TODO(benjhayden) Fuzzy-match and highlight the erroneous event.
      for (std::deque<Event*>::const_iterator iter = events_.begin();
          iter != events_.end(); ++iter) {
        if ((*iter)->caught() > last_wait_) {
          LOG(INFO) << "Caught " << (*iter)->Debug();
        }
      }
      if (waiting_for_.get()) {
        LOG(INFO) << "Timed out waiting for " << waiting_for_->Debug();
      }
      waiting_ = false;
    }
    waiting_for_.reset();
    last_wait_ = base::Time::Now();
    return success;
  }

 private:
  bool waiting_;
  base::Time last_wait_;
  scoped_ptr<Event> waiting_for_;
  content::NotificationRegistrar registrar_;
  std::deque<Event*> events_;

  DISALLOW_COPY_AND_ASSIGN(DownloadsEventsListener);
};

class DownloadExtensionTest : public ExtensionApiTest {
 public:
  DownloadExtensionTest()
    : extension_(NULL),
      incognito_browser_(NULL),
      current_browser_(NULL) {
  }

 protected:
  // Used with CreateHistoryDownloads
  struct HistoryDownloadInfo {
    // Filename to use. CreateHistoryDownloads will append this filename to the
    // temporary downloads directory specified by downloads_directory().
    const base::FilePath::CharType*   filename;

    // State for the download. Note that IN_PROGRESS downloads will be created
    // as CANCELLED.
    DownloadItem::DownloadState state;

    // Danger type for the download. Only use DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS
    // and DOWNLOAD_DANGER_TYPE_DANGEROUS_CONTENT.
    content::DownloadDangerType danger_type;
  };

  void LoadExtension(const char* name) {
    // Store the created Extension object so that we can attach it to
    // ExtensionFunctions.  Also load the extension in incognito profiles for
    // testing incognito.
    extension_ = LoadExtensionIncognito(test_data_dir_.AppendASCII(name));
    CHECK(extension_);
    content::WebContents* tab = chrome::AddSelectedTabWithURL(
        current_browser(),
        extension_->GetResourceURL("empty.html"),
        content::PAGE_TRANSITION_LINK);
    EventRouter::Get(current_browser()->profile())
        ->AddEventListener(downloads::OnCreated::kEventName,
                           tab->GetRenderProcessHost(),
                           GetExtensionId());
    EventRouter::Get(current_browser()->profile())
        ->AddEventListener(downloads::OnChanged::kEventName,
                           tab->GetRenderProcessHost(),
                           GetExtensionId());
    EventRouter::Get(current_browser()->profile())
        ->AddEventListener(downloads::OnErased::kEventName,
                           tab->GetRenderProcessHost(),
                           GetExtensionId());
  }

  content::RenderProcessHost* AddFilenameDeterminer() {
    ExtensionDownloadsEventRouter::SetDetermineFilenameTimeoutSecondsForTesting(
        2);
    content::WebContents* tab = chrome::AddSelectedTabWithURL(
        current_browser(),
        extension_->GetResourceURL("empty.html"),
        content::PAGE_TRANSITION_LINK);
    EventRouter::Get(current_browser()->profile())
        ->AddEventListener(downloads::OnDeterminingFilename::kEventName,
                           tab->GetRenderProcessHost(),
                           GetExtensionId());
    return tab->GetRenderProcessHost();
  }

  void RemoveFilenameDeterminer(content::RenderProcessHost* host) {
    EventRouter::Get(current_browser()->profile())->RemoveEventListener(
        downloads::OnDeterminingFilename::kEventName, host, GetExtensionId());
  }

  Browser* current_browser() { return current_browser_; }

  // InProcessBrowserTest
  virtual void SetUpOnMainThread() OVERRIDE {
    ExtensionApiTest::SetUpOnMainThread();
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&chrome_browser_net::SetUrlRequestMocksEnabled, true));
    InProcessBrowserTest::SetUpOnMainThread();
    GoOnTheRecord();
    CreateAndSetDownloadsDirectory();
    current_browser()->profile()->GetPrefs()->SetBoolean(
        prefs::kPromptForDownload, false);
    GetOnRecordManager()->RemoveAllDownloads();
    events_listener_.reset(new DownloadsEventsListener());
    // Disable file chooser for current profile.
    DownloadTestFileActivityObserver observer(current_browser()->profile());
    observer.EnableFileChooser(false);
  }

  void GoOnTheRecord() { current_browser_ = browser(); }

  void GoOffTheRecord() {
    if (!incognito_browser_) {
      incognito_browser_ = CreateIncognitoBrowser();
      GetOffRecordManager()->RemoveAllDownloads();
      // Disable file chooser for incognito profile.
      DownloadTestFileActivityObserver observer(incognito_browser_->profile());
      observer.EnableFileChooser(false);
    }
    current_browser_ = incognito_browser_;
  }

  bool WaitFor(const std::string& event_name, const std::string& json_args) {
    return events_listener_->WaitFor(
        current_browser()->profile(), event_name, json_args);
  }

  bool WaitForInterruption(
      DownloadItem* item,
      content::DownloadInterruptReason expected_error,
      const std::string& on_created_event) {
    if (!WaitFor(downloads::OnCreated::kEventName, on_created_event))
      return false;
    // Now, onCreated is always fired before interruption.
    return WaitFor(
        downloads::OnChanged::kEventName,
        base::StringPrintf(
            "[{\"id\": %d,"
            "  \"error\": {\"current\": \"%s\"},"
            "  \"state\": {"
            "    \"previous\": \"in_progress\","
            "    \"current\": \"interrupted\"}}]",
            item->GetId(),
            content::DownloadInterruptReasonToString(expected_error).c_str()));
  }

  void ClearEvents() {
    events_listener_->ClearEvents();
  }

  std::string GetExtensionURL() {
    return extension_->url().spec();
  }
  std::string GetExtensionId() {
    return extension_->id();
  }

  std::string GetFilename(const char* path) {
    std::string result =
      downloads_directory_.path().AppendASCII(path).AsUTF8Unsafe();
#if defined(OS_WIN)
    for (std::string::size_type next = result.find("\\");
         next != std::string::npos;
         next = result.find("\\", next)) {
      result.replace(next, 1, "\\\\");
      next += 2;
    }
#endif
    return result;
  }

  DownloadManager* GetOnRecordManager() {
    return BrowserContext::GetDownloadManager(browser()->profile());
  }
  DownloadManager* GetOffRecordManager() {
    return BrowserContext::GetDownloadManager(
        browser()->profile()->GetOffTheRecordProfile());
  }
  DownloadManager* GetCurrentManager() {
    return (current_browser_ == incognito_browser_) ?
      GetOffRecordManager() : GetOnRecordManager();
  }

  // Creates a set of history downloads based on the provided |history_info|
  // array. |count| is the number of elements in |history_info|. On success,
  // |items| will contain |count| DownloadItems in the order that they were
  // specified in |history_info|. Returns true on success and false otherwise.
  bool CreateHistoryDownloads(const HistoryDownloadInfo* history_info,
                              size_t count,
                              DownloadManager::DownloadVector* items) {
    DownloadIdComparator download_id_comparator;
    base::Time current = base::Time::Now();
    items->clear();
    GetOnRecordManager()->GetAllDownloads(items);
    CHECK_EQ(0, static_cast<int>(items->size()));
    std::vector<GURL> url_chain;
    url_chain.push_back(GURL());
    for (size_t i = 0; i < count; ++i) {
      DownloadItem* item = GetOnRecordManager()->CreateDownloadItem(
          content::DownloadItem::kInvalidId + 1 + i,
          downloads_directory().Append(history_info[i].filename),
          downloads_directory().Append(history_info[i].filename),
          url_chain, GURL(),    // URL Chain, referrer
          std::string(), std::string(), // mime_type, original_mime_type
          current, current,  // start_time, end_time
          std::string(), std::string(), // etag, last_modified
          1, 1,              // received_bytes, total_bytes
          history_info[i].state,  // state
          content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
          content::DOWNLOAD_INTERRUPT_REASON_NONE,
          false);                 // opened
      items->push_back(item);
    }

    // Order by ID so that they are in the order that we created them.
    std::sort(items->begin(), items->end(), download_id_comparator);
    // Set the danger type if necessary.
    for (size_t i = 0; i < count; ++i) {
      if (history_info[i].danger_type !=
          content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS) {
        EXPECT_EQ(content::DOWNLOAD_DANGER_TYPE_DANGEROUS_CONTENT,
                  history_info[i].danger_type);
        items->at(i)->OnContentCheckCompleted(history_info[i].danger_type);
      }
    }
    return true;
  }

  void CreateSlowTestDownloads(
      size_t count, DownloadManager::DownloadVector* items) {
    for (size_t i = 0; i < count; ++i) {
      scoped_ptr<content::DownloadTestObserver> observer(
          CreateInProgressDownloadObserver(1));
      GURL slow_download_url(URLRequestSlowDownloadJob::kUnknownSizeUrl);
      ui_test_utils::NavigateToURLWithDisposition(
          current_browser(), slow_download_url, CURRENT_TAB,
          ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
      observer->WaitForFinished();
      EXPECT_EQ(
          1u, observer->NumDownloadsSeenInState(DownloadItem::IN_PROGRESS));
    }
    GetCurrentManager()->GetAllDownloads(items);
    ASSERT_EQ(count, items->size());
  }

  DownloadItem* CreateSlowTestDownload() {
    scoped_ptr<content::DownloadTestObserver> observer(
        CreateInProgressDownloadObserver(1));
    GURL slow_download_url(URLRequestSlowDownloadJob::kUnknownSizeUrl);
    DownloadManager* manager = GetCurrentManager();

    EXPECT_EQ(0, manager->NonMaliciousInProgressCount());
    EXPECT_EQ(0, manager->InProgressCount());
    if (manager->InProgressCount() != 0)
      return NULL;

    ui_test_utils::NavigateToURLWithDisposition(
        current_browser(), slow_download_url, CURRENT_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

    observer->WaitForFinished();
    EXPECT_EQ(1u, observer->NumDownloadsSeenInState(DownloadItem::IN_PROGRESS));

    DownloadManager::DownloadVector items;
    manager->GetAllDownloads(&items);

    DownloadItem* new_item = NULL;
    for (DownloadManager::DownloadVector::iterator iter = items.begin();
         iter != items.end(); ++iter) {
      if ((*iter)->GetState() == DownloadItem::IN_PROGRESS) {
        // There should be only one IN_PROGRESS item.
        EXPECT_EQ(NULL, new_item);
        new_item = *iter;
      }
    }
    return new_item;
  }

  void FinishPendingSlowDownloads() {
    scoped_ptr<content::DownloadTestObserver> observer(
        CreateDownloadObserver(1));
    GURL finish_url(URLRequestSlowDownloadJob::kFinishDownloadUrl);
    ui_test_utils::NavigateToURLWithDisposition(
        current_browser(), finish_url, NEW_FOREGROUND_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
    observer->WaitForFinished();
    EXPECT_EQ(1u, observer->NumDownloadsSeenInState(DownloadItem::COMPLETE));
  }

  content::DownloadTestObserver* CreateDownloadObserver(size_t download_count) {
    return new content::DownloadTestObserverTerminal(
        GetCurrentManager(), download_count,
        content::DownloadTestObserver::ON_DANGEROUS_DOWNLOAD_FAIL);
  }

  content::DownloadTestObserver* CreateInProgressDownloadObserver(
      size_t download_count) {
    return new content::DownloadTestObserverInProgress(
        GetCurrentManager(), download_count);
  }

  bool RunFunction(UIThreadExtensionFunction* function,
                   const std::string& args) {
    scoped_refptr<UIThreadExtensionFunction> delete_function(function);
    SetUpExtensionFunction(function);
    bool result = extension_function_test_utils::RunFunction(
        function, args, browser(), GetFlags());
    if (!result) {
      LOG(ERROR) << function->GetError();
    }
    return result;
  }

  extension_function_test_utils::RunFunctionFlags GetFlags() {
    return current_browser()->profile()->IsOffTheRecord() ?
           extension_function_test_utils::INCLUDE_INCOGNITO :
           extension_function_test_utils::NONE;
  }

  // extension_function_test_utils::RunFunction*() only uses browser for its
  // profile(), so pass it the on-record browser so that it always uses the
  // on-record profile to match real-life behavior.

  base::Value* RunFunctionAndReturnResult(
      scoped_refptr<UIThreadExtensionFunction> function,
      const std::string& args) {
    SetUpExtensionFunction(function.get());
    return extension_function_test_utils::RunFunctionAndReturnSingleResult(
        function.get(), args, browser(), GetFlags());
  }

  std::string RunFunctionAndReturnError(
      scoped_refptr<UIThreadExtensionFunction> function,
      const std::string& args) {
    SetUpExtensionFunction(function.get());
    return extension_function_test_utils::RunFunctionAndReturnError(
        function.get(), args, browser(), GetFlags());
  }

  bool RunFunctionAndReturnString(
      scoped_refptr<UIThreadExtensionFunction> function,
      const std::string& args,
      std::string* result_string) {
    SetUpExtensionFunction(function.get());
    scoped_ptr<base::Value> result(RunFunctionAndReturnResult(function, args));
    EXPECT_TRUE(result.get());
    return result.get() && result->GetAsString(result_string);
  }

  std::string DownloadItemIdAsArgList(const DownloadItem* download_item) {
    return base::StringPrintf("[%d]", download_item->GetId());
  }

  const base::FilePath& downloads_directory() {
    return downloads_directory_.path();
  }

  DownloadsEventsListener* events_listener() { return events_listener_.get(); }

 private:
  void SetUpExtensionFunction(UIThreadExtensionFunction* function) {
    if (extension_) {
      // Recreate the tab each time for insulation.
      content::WebContents* tab = chrome::AddSelectedTabWithURL(
          current_browser(),
          extension_->GetResourceURL("empty.html"),
          content::PAGE_TRANSITION_LINK);
      function->set_extension(extension_);
      function->SetRenderViewHost(tab->GetRenderViewHost());
    }
  }

  void CreateAndSetDownloadsDirectory() {
    ASSERT_TRUE(downloads_directory_.CreateUniqueTempDir());
    current_browser()->profile()->GetPrefs()->SetFilePath(
        prefs::kDownloadDefaultDirectory,
        downloads_directory_.path());
  }

  base::ScopedTempDir downloads_directory_;
  const Extension* extension_;
  Browser* incognito_browser_;
  Browser* current_browser_;
  scoped_ptr<DownloadsEventsListener> events_listener_;

  DISALLOW_COPY_AND_ASSIGN(DownloadExtensionTest);
};

class MockIconExtractorImpl : public DownloadFileIconExtractor {
 public:
  MockIconExtractorImpl(const base::FilePath& path,
                        IconLoader::IconSize icon_size,
                        const std::string& response)
      : expected_path_(path),
        expected_icon_size_(icon_size),
        response_(response) {
  }
  virtual ~MockIconExtractorImpl() {}

  virtual bool ExtractIconURLForPath(const base::FilePath& path,
                                     float scale,
                                     IconLoader::IconSize icon_size,
                                     IconURLCallback callback) OVERRIDE {
    EXPECT_STREQ(expected_path_.value().c_str(), path.value().c_str());
    EXPECT_EQ(expected_icon_size_, icon_size);
    if (expected_path_ == path &&
        expected_icon_size_ == icon_size) {
      callback_ = callback;
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&MockIconExtractorImpl::RunCallback,
                     base::Unretained(this)));
      return true;
    } else {
      return false;
    }
  }

 private:
  void RunCallback() {
    callback_.Run(response_);
    // Drop the reference on extension function to avoid memory leaks.
    callback_ = IconURLCallback();
  }

  base::FilePath             expected_path_;
  IconLoader::IconSize expected_icon_size_;
  std::string          response_;
  IconURLCallback      callback_;
};

bool ItemNotInProgress(DownloadItem* item) {
  return item->GetState() != DownloadItem::IN_PROGRESS;
}

// Cancels the underlying DownloadItem when the ScopedCancellingItem goes out of
// scope. Like a scoped_ptr, but for DownloadItems.
class ScopedCancellingItem {
 public:
  explicit ScopedCancellingItem(DownloadItem* item) : item_(item) {}
  ~ScopedCancellingItem() {
    item_->Cancel(true);
    content::DownloadUpdatedObserver observer(
        item_, base::Bind(&ItemNotInProgress));
    observer.WaitForEvent();
  }
  DownloadItem* get() { return item_; }
 private:
  DownloadItem* item_;
  DISALLOW_COPY_AND_ASSIGN(ScopedCancellingItem);
};

// Cancels all the underlying DownloadItems when the ScopedItemVectorCanceller
// goes out of scope. Generalization of ScopedCancellingItem to many
// DownloadItems.
class ScopedItemVectorCanceller {
 public:
  explicit ScopedItemVectorCanceller(DownloadManager::DownloadVector* items)
    : items_(items) {
  }
  ~ScopedItemVectorCanceller() {
    for (DownloadManager::DownloadVector::const_iterator item = items_->begin();
         item != items_->end(); ++item) {
      if ((*item)->GetState() == DownloadItem::IN_PROGRESS)
        (*item)->Cancel(true);
      content::DownloadUpdatedObserver observer(
          (*item), base::Bind(&ItemNotInProgress));
      observer.WaitForEvent();
    }
  }

 private:
  DownloadManager::DownloadVector* items_;
  DISALLOW_COPY_AND_ASSIGN(ScopedItemVectorCanceller);
};

// Writes an HTML5 file so that it can be downloaded.
class HTML5FileWriter {
 public:
  static bool CreateFileForTesting(storage::FileSystemContext* context,
                                   const storage::FileSystemURL& path,
                                   const char* data,
                                   int length) {
    // Create a temp file.
    base::FilePath temp_file;
    if (!base::CreateTemporaryFile(&temp_file) ||
        base::WriteFile(temp_file, data, length) != length) {
      return false;
    }
    // Invoke the fileapi to copy it into the sandboxed filesystem.
    bool result = false;
    base::WaitableEvent done_event(true, false);
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&CreateFileForTestingOnIOThread,
                   base::Unretained(context),
                   path, temp_file,
                   base::Unretained(&result),
                   base::Unretained(&done_event)));
    // Wait for that to finish.
    done_event.Wait();
    base::DeleteFile(temp_file, false);
    return result;
  }

 private:
  static void CopyInCompletion(bool* result,
                               base::WaitableEvent* done_event,
                               base::File::Error error) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    *result = error == base::File::FILE_OK;
    done_event->Signal();
  }

  static void CreateFileForTestingOnIOThread(
      storage::FileSystemContext* context,
      const storage::FileSystemURL& path,
      const base::FilePath& temp_file,
      bool* result,
      base::WaitableEvent* done_event) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    context->operation_runner()->CopyInForeignFile(
        temp_file, path,
        base::Bind(&CopyInCompletion,
                   base::Unretained(result),
                   base::Unretained(done_event)));
  }
};

// TODO(benjhayden) Merge this with the other TestObservers.
class JustInProgressDownloadObserver
    : public content::DownloadTestObserverInProgress {
 public:
  JustInProgressDownloadObserver(
      DownloadManager* download_manager, size_t wait_count)
      : content::DownloadTestObserverInProgress(download_manager, wait_count) {
  }

  virtual ~JustInProgressDownloadObserver() {}

 private:
  virtual bool IsDownloadInFinalState(DownloadItem* item) OVERRIDE {
    return item->GetState() == DownloadItem::IN_PROGRESS;
  }

  DISALLOW_COPY_AND_ASSIGN(JustInProgressDownloadObserver);
};

bool ItemIsInterrupted(DownloadItem* item) {
  return item->GetState() == DownloadItem::INTERRUPTED;
}

content::DownloadInterruptReason InterruptReasonExtensionToContent(
    downloads::InterruptReason error) {
  switch (error) {
    case downloads::INTERRUPT_REASON_NONE:
      return content::DOWNLOAD_INTERRUPT_REASON_NONE;
#define INTERRUPT_REASON(name, value)      \
  case downloads::INTERRUPT_REASON_##name: \
    return content::DOWNLOAD_INTERRUPT_REASON_##name;
#include "content/public/browser/download_interrupt_reason_values.h"
#undef INTERRUPT_REASON
  }
  NOTREACHED();
  return content::DOWNLOAD_INTERRUPT_REASON_NONE;
}

downloads::InterruptReason InterruptReasonContentToExtension(
    content::DownloadInterruptReason error) {
  switch (error) {
    case content::DOWNLOAD_INTERRUPT_REASON_NONE:
      return downloads::INTERRUPT_REASON_NONE;
#define INTERRUPT_REASON(name, value)             \
  case content::DOWNLOAD_INTERRUPT_REASON_##name: \
    return downloads::INTERRUPT_REASON_##name;
#include "content/public/browser/download_interrupt_reason_values.h"
#undef INTERRUPT_REASON
  }
  NOTREACHED();
  return downloads::INTERRUPT_REASON_NONE;
}

}  // namespace

#if defined(OS_CHROMEOS)
// http://crbug.com/396510
#define MAYBE_DownloadExtensionTest_Open DISABLED_DownloadExtensionTest_Open
#else
#define MAYBE_DownloadExtensionTest_Open DownloadExtensionTest_Open
#endif
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       MAYBE_DownloadExtensionTest_Open) {
  LoadExtension("downloads_split");
  DownloadsOpenFunction* open_function = new DownloadsOpenFunction();
  open_function->set_user_gesture(true);
  EXPECT_STREQ(errors::kInvalidId,
               RunFunctionAndReturnError(
                   open_function,
                   "[-42]").c_str());

  DownloadItem* download_item = CreateSlowTestDownload();
  ASSERT_TRUE(download_item);
  EXPECT_FALSE(download_item->GetOpened());
  EXPECT_FALSE(download_item->GetOpenWhenComplete());
  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"mime\": \"application/octet-stream\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          download_item->GetURL().spec().c_str())));
  open_function = new DownloadsOpenFunction();
  open_function->set_user_gesture(true);
  EXPECT_STREQ(errors::kNotComplete,
               RunFunctionAndReturnError(
                   open_function,
                   DownloadItemIdAsArgList(download_item)).c_str());

  FinishPendingSlowDownloads();
  EXPECT_FALSE(download_item->GetOpened());

  open_function = new DownloadsOpenFunction();
  EXPECT_STREQ(errors::kUserGesture,
               RunFunctionAndReturnError(
                  open_function,
                  DownloadItemIdAsArgList(download_item)).c_str());
  EXPECT_FALSE(download_item->GetOpened());

  open_function = new DownloadsOpenFunction();
  open_function->set_user_gesture(true);
  EXPECT_TRUE(RunFunction(open_function,
                          DownloadItemIdAsArgList(download_item)));
  EXPECT_TRUE(download_item->GetOpened());
}

IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_PauseResumeCancelErase) {
  DownloadItem* download_item = CreateSlowTestDownload();
  ASSERT_TRUE(download_item);
  std::string error;

  // Call pause().  It should succeed and the download should be paused on
  // return.
  EXPECT_TRUE(RunFunction(new DownloadsPauseFunction(),
                          DownloadItemIdAsArgList(download_item)));
  EXPECT_TRUE(download_item->IsPaused());

  // Calling removeFile on a non-active download yields kNotComplete
  // and should not crash. http://crbug.com/319984
  error = RunFunctionAndReturnError(new DownloadsRemoveFileFunction(),
                                    DownloadItemIdAsArgList(download_item));
  EXPECT_STREQ(errors::kNotComplete, error.c_str());

  // Calling pause() twice shouldn't be an error.
  EXPECT_TRUE(RunFunction(new DownloadsPauseFunction(),
                          DownloadItemIdAsArgList(download_item)));
  EXPECT_TRUE(download_item->IsPaused());

  // Now try resuming this download.  It should succeed.
  EXPECT_TRUE(RunFunction(new DownloadsResumeFunction(),
                          DownloadItemIdAsArgList(download_item)));
  EXPECT_FALSE(download_item->IsPaused());

  // Resume again.  Resuming a download that wasn't paused is not an error.
  EXPECT_TRUE(RunFunction(new DownloadsResumeFunction(),
                          DownloadItemIdAsArgList(download_item)));
  EXPECT_FALSE(download_item->IsPaused());

  // Pause again.
  EXPECT_TRUE(RunFunction(new DownloadsPauseFunction(),
                          DownloadItemIdAsArgList(download_item)));
  EXPECT_TRUE(download_item->IsPaused());

  // And now cancel.
  EXPECT_TRUE(RunFunction(new DownloadsCancelFunction(),
                          DownloadItemIdAsArgList(download_item)));
  EXPECT_EQ(DownloadItem::CANCELLED, download_item->GetState());

  // Cancel again.  Shouldn't have any effect.
  EXPECT_TRUE(RunFunction(new DownloadsCancelFunction(),
                          DownloadItemIdAsArgList(download_item)));
  EXPECT_EQ(DownloadItem::CANCELLED, download_item->GetState());

  // Calling paused on a non-active download yields kNotInProgress.
  error = RunFunctionAndReturnError(
      new DownloadsPauseFunction(), DownloadItemIdAsArgList(download_item));
  EXPECT_STREQ(errors::kNotInProgress, error.c_str());

  // Calling resume on a non-active download yields kNotResumable
  error = RunFunctionAndReturnError(
      new DownloadsResumeFunction(), DownloadItemIdAsArgList(download_item));
  EXPECT_STREQ(errors::kNotResumable, error.c_str());

  // Calling pause on a non-existent download yields kInvalidId.
  error = RunFunctionAndReturnError(
      new DownloadsPauseFunction(), "[-42]");
  EXPECT_STREQ(errors::kInvalidId, error.c_str());

  // Calling resume on a non-existent download yields kInvalidId
  error = RunFunctionAndReturnError(
      new DownloadsResumeFunction(), "[-42]");
  EXPECT_STREQ(errors::kInvalidId, error.c_str());

  // Calling removeFile on a non-existent download yields kInvalidId.
  error = RunFunctionAndReturnError(
      new DownloadsRemoveFileFunction(), "[-42]");
  EXPECT_STREQ(errors::kInvalidId, error.c_str());

  int id = download_item->GetId();
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsEraseFunction(),
      base::StringPrintf("[{\"id\": %d}]", id)));
  DownloadManager::DownloadVector items;
  GetCurrentManager()->GetAllDownloads(&items);
  EXPECT_EQ(0UL, items.size());
  ASSERT_TRUE(result);
  download_item = NULL;
  base::ListValue* result_list = NULL;
  ASSERT_TRUE(result->GetAsList(&result_list));
  ASSERT_EQ(1UL, result_list->GetSize());
  int element = -1;
  ASSERT_TRUE(result_list->GetInteger(0, &element));
  EXPECT_EQ(id, element);
}

scoped_refptr<UIThreadExtensionFunction> MockedGetFileIconFunction(
    const base::FilePath& expected_path,
    IconLoader::IconSize icon_size,
    const std::string& response) {
  scoped_refptr<DownloadsGetFileIconFunction> function(
      new DownloadsGetFileIconFunction());
  function->SetIconExtractorForTesting(new MockIconExtractorImpl(
      expected_path, icon_size, response));
  return function;
}

// Test downloads.getFileIcon() on in-progress, finished, cancelled and deleted
// download items.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
    DownloadExtensionTest_FileIcon_Active) {
  DownloadItem* download_item = CreateSlowTestDownload();
  ASSERT_TRUE(download_item);
  ASSERT_FALSE(download_item->GetTargetFilePath().empty());
  std::string args32(base::StringPrintf("[%d, {\"size\": 32}]",
                     download_item->GetId()));
  std::string result_string;

  // Get the icon for the in-progress download.  This call should succeed even
  // if the file type isn't registered.
  // Test whether the correct path is being pased into the icon extractor.
  EXPECT_TRUE(RunFunctionAndReturnString(MockedGetFileIconFunction(
          download_item->GetTargetFilePath(), IconLoader::NORMAL, "foo"),
      base::StringPrintf("[%d, {}]", download_item->GetId()), &result_string));

  // Now try a 16x16 icon.
  EXPECT_TRUE(RunFunctionAndReturnString(MockedGetFileIconFunction(
          download_item->GetTargetFilePath(), IconLoader::SMALL, "foo"),
      base::StringPrintf("[%d, {\"size\": 16}]", download_item->GetId()),
      &result_string));

  // Explicitly asking for 32x32 should give us a 32x32 icon.
  EXPECT_TRUE(RunFunctionAndReturnString(MockedGetFileIconFunction(
          download_item->GetTargetFilePath(), IconLoader::NORMAL, "foo"),
      args32, &result_string));

  // Finish the download and try again.
  FinishPendingSlowDownloads();
  EXPECT_EQ(DownloadItem::COMPLETE, download_item->GetState());
  EXPECT_TRUE(RunFunctionAndReturnString(MockedGetFileIconFunction(
          download_item->GetTargetFilePath(), IconLoader::NORMAL, "foo"),
      args32, &result_string));

  // Check the path passed to the icon extractor post-completion.
  EXPECT_TRUE(RunFunctionAndReturnString(MockedGetFileIconFunction(
          download_item->GetTargetFilePath(), IconLoader::NORMAL, "foo"),
      args32, &result_string));

  // Now create another download.
  download_item = CreateSlowTestDownload();
  ASSERT_TRUE(download_item);
  ASSERT_FALSE(download_item->GetTargetFilePath().empty());
  args32 = base::StringPrintf("[%d, {\"size\": 32}]", download_item->GetId());

  // Cancel the download. As long as the download has a target path, we should
  // be able to query the file icon.
  download_item->Cancel(true);
  ASSERT_FALSE(download_item->GetTargetFilePath().empty());
  // Let cleanup complete on the FILE thread.
  content::RunAllPendingInMessageLoop(BrowserThread::FILE);
  // Check the path passed to the icon extractor post-cancellation.
  EXPECT_TRUE(RunFunctionAndReturnString(MockedGetFileIconFunction(
          download_item->GetTargetFilePath(), IconLoader::NORMAL, "foo"),
      args32,
      &result_string));

  // Simulate an error during icon load by invoking the mock with an empty
  // result string.
  std::string error = RunFunctionAndReturnError(
      MockedGetFileIconFunction(download_item->GetTargetFilePath(),
                                IconLoader::NORMAL,
                                std::string()),
      args32);
  EXPECT_STREQ(errors::kIconNotFound, error.c_str());

  // Once the download item is deleted, we should return kInvalidId.
  int id = download_item->GetId();
  download_item->Remove();
  download_item = NULL;
  EXPECT_EQ(static_cast<DownloadItem*>(NULL),
            GetCurrentManager()->GetDownload(id));
  error = RunFunctionAndReturnError(new DownloadsGetFileIconFunction(), args32);
  EXPECT_STREQ(errors::kInvalidId,
               error.c_str());
}

// Test that we can acquire file icons for history downloads regardless of
// whether they exist or not.  If the file doesn't exist we should receive a
// generic icon from the OS/toolkit that may or may not be specific to the file
// type.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
    DownloadExtensionTest_FileIcon_History) {
  const HistoryDownloadInfo kHistoryInfo[] = {
    { FILE_PATH_LITERAL("real.txt"),
      DownloadItem::COMPLETE,
      content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS },
    { FILE_PATH_LITERAL("fake.txt"),
      DownloadItem::COMPLETE,
      content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS }
  };
  DownloadManager::DownloadVector all_downloads;
  ASSERT_TRUE(CreateHistoryDownloads(kHistoryInfo, arraysize(kHistoryInfo),
                                     &all_downloads));

  base::FilePath real_path = all_downloads[0]->GetTargetFilePath();
  base::FilePath fake_path = all_downloads[1]->GetTargetFilePath();

  EXPECT_EQ(0, base::WriteFile(real_path, "", 0));
  ASSERT_TRUE(base::PathExists(real_path));
  ASSERT_FALSE(base::PathExists(fake_path));

  for (DownloadManager::DownloadVector::iterator iter = all_downloads.begin();
       iter != all_downloads.end();
       ++iter) {
    std::string result_string;
    // Use a MockIconExtractorImpl to test if the correct path is being passed
    // into the DownloadFileIconExtractor.
    EXPECT_TRUE(RunFunctionAndReturnString(MockedGetFileIconFunction(
            (*iter)->GetTargetFilePath(), IconLoader::NORMAL, "hello"),
        base::StringPrintf("[%d, {\"size\": 32}]", (*iter)->GetId()),
        &result_string));
    EXPECT_STREQ("hello", result_string.c_str());
  }
}

// Test passing the empty query to search().
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_SearchEmptyQuery) {
  ScopedCancellingItem item(CreateSlowTestDownload());
  ASSERT_TRUE(item.get());

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsSearchFunction(), "[{}]"));
  ASSERT_TRUE(result.get());
  base::ListValue* result_list = NULL;
  ASSERT_TRUE(result->GetAsList(&result_list));
  ASSERT_EQ(1UL, result_list->GetSize());
}

// Test the |filenameRegex| parameter for search().
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
    DownloadExtensionTest_SearchFilenameRegex) {
  const HistoryDownloadInfo kHistoryInfo[] = {
    { FILE_PATH_LITERAL("foobar"),
      DownloadItem::COMPLETE,
      content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS },
    { FILE_PATH_LITERAL("baz"),
      DownloadItem::COMPLETE,
      content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS }
  };
  DownloadManager::DownloadVector all_downloads;
  ASSERT_TRUE(CreateHistoryDownloads(kHistoryInfo, arraysize(kHistoryInfo),
                                     &all_downloads));

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsSearchFunction(), "[{\"filenameRegex\": \"foobar\"}]"));
  ASSERT_TRUE(result.get());
  base::ListValue* result_list = NULL;
  ASSERT_TRUE(result->GetAsList(&result_list));
  ASSERT_EQ(1UL, result_list->GetSize());
  base::DictionaryValue* item_value = NULL;
  ASSERT_TRUE(result_list->GetDictionary(0, &item_value));
  int item_id = -1;
  ASSERT_TRUE(item_value->GetInteger("id", &item_id));
  ASSERT_EQ(all_downloads[0]->GetId(), static_cast<uint32>(item_id));
}

// Test the |id| parameter for search().
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest, DownloadExtensionTest_SearchId) {
  DownloadManager::DownloadVector items;
  CreateSlowTestDownloads(2, &items);
  ScopedItemVectorCanceller delete_items(&items);

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsSearchFunction(), base::StringPrintf(
          "[{\"id\": %u}]", items[0]->GetId())));
  ASSERT_TRUE(result.get());
  base::ListValue* result_list = NULL;
  ASSERT_TRUE(result->GetAsList(&result_list));
  ASSERT_EQ(1UL, result_list->GetSize());
  base::DictionaryValue* item_value = NULL;
  ASSERT_TRUE(result_list->GetDictionary(0, &item_value));
  int item_id = -1;
  ASSERT_TRUE(item_value->GetInteger("id", &item_id));
  ASSERT_EQ(items[0]->GetId(), static_cast<uint32>(item_id));
}

// Test specifying both the |id| and |filename| parameters for search().
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
    DownloadExtensionTest_SearchIdAndFilename) {
  DownloadManager::DownloadVector items;
  CreateSlowTestDownloads(2, &items);
  ScopedItemVectorCanceller delete_items(&items);

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsSearchFunction(),
      "[{\"id\": 0, \"filename\": \"foobar\"}]"));
  ASSERT_TRUE(result.get());
  base::ListValue* result_list = NULL;
  ASSERT_TRUE(result->GetAsList(&result_list));
  ASSERT_EQ(0UL, result_list->GetSize());
}

// Test a single |orderBy| parameter for search().
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
    DownloadExtensionTest_SearchOrderBy) {
  const HistoryDownloadInfo kHistoryInfo[] = {
    { FILE_PATH_LITERAL("zzz"),
      DownloadItem::COMPLETE,
      content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS },
    { FILE_PATH_LITERAL("baz"),
      DownloadItem::COMPLETE,
      content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS }
  };
  DownloadManager::DownloadVector items;
  ASSERT_TRUE(CreateHistoryDownloads(kHistoryInfo, arraysize(kHistoryInfo),
                                     &items));

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsSearchFunction(), "[{\"orderBy\": [\"filename\"]}]"));
  ASSERT_TRUE(result.get());
  base::ListValue* result_list = NULL;
  ASSERT_TRUE(result->GetAsList(&result_list));
  ASSERT_EQ(2UL, result_list->GetSize());
  base::DictionaryValue* item0_value = NULL;
  base::DictionaryValue* item1_value = NULL;
  ASSERT_TRUE(result_list->GetDictionary(0, &item0_value));
  ASSERT_TRUE(result_list->GetDictionary(1, &item1_value));
  std::string item0_name, item1_name;
  ASSERT_TRUE(item0_value->GetString("filename", &item0_name));
  ASSERT_TRUE(item1_value->GetString("filename", &item1_name));
  ASSERT_GT(items[0]->GetTargetFilePath().value(),
            items[1]->GetTargetFilePath().value());
  ASSERT_LT(item0_name, item1_name);
}

// Test specifying an empty |orderBy| parameter for search().
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
    DownloadExtensionTest_SearchOrderByEmpty) {
  const HistoryDownloadInfo kHistoryInfo[] = {
    { FILE_PATH_LITERAL("zzz"),
      DownloadItem::COMPLETE,
      content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS },
    { FILE_PATH_LITERAL("baz"),
      DownloadItem::COMPLETE,
      content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS }
  };
  DownloadManager::DownloadVector items;
  ASSERT_TRUE(CreateHistoryDownloads(kHistoryInfo, arraysize(kHistoryInfo),
                                     &items));

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsSearchFunction(), "[{\"orderBy\": []}]"));
  ASSERT_TRUE(result.get());
  base::ListValue* result_list = NULL;
  ASSERT_TRUE(result->GetAsList(&result_list));
  ASSERT_EQ(2UL, result_list->GetSize());
  base::DictionaryValue* item0_value = NULL;
  base::DictionaryValue* item1_value = NULL;
  ASSERT_TRUE(result_list->GetDictionary(0, &item0_value));
  ASSERT_TRUE(result_list->GetDictionary(1, &item1_value));
  std::string item0_name, item1_name;
  ASSERT_TRUE(item0_value->GetString("filename", &item0_name));
  ASSERT_TRUE(item1_value->GetString("filename", &item1_name));
  ASSERT_GT(items[0]->GetTargetFilePath().value(),
            items[1]->GetTargetFilePath().value());
  // The order of results when orderBy is empty is unspecified. When there are
  // no sorters, DownloadQuery does not call sort(), so the order of the results
  // depends on the order of the items in base::hash_map<uint32,...>
  // DownloadManagerImpl::downloads_, which is unspecified and differs between
  // libc++ and libstdc++. http://crbug.com/365334
}

// Test the |danger| option for search().
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
    DownloadExtensionTest_SearchDanger) {
  const HistoryDownloadInfo kHistoryInfo[] = {
    { FILE_PATH_LITERAL("zzz"),
      DownloadItem::COMPLETE,
      content::DOWNLOAD_DANGER_TYPE_DANGEROUS_CONTENT },
    { FILE_PATH_LITERAL("baz"),
      DownloadItem::COMPLETE,
      content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS }
  };
  DownloadManager::DownloadVector items;
  ASSERT_TRUE(CreateHistoryDownloads(kHistoryInfo, arraysize(kHistoryInfo),
                                     &items));

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsSearchFunction(), "[{\"danger\": \"content\"}]"));
  ASSERT_TRUE(result.get());
  base::ListValue* result_list = NULL;
  ASSERT_TRUE(result->GetAsList(&result_list));
  ASSERT_EQ(1UL, result_list->GetSize());
}

// Test the |state| option for search().
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
    DownloadExtensionTest_SearchState) {
  DownloadManager::DownloadVector items;
  CreateSlowTestDownloads(2, &items);
  ScopedItemVectorCanceller delete_items(&items);

  items[0]->Cancel(true);

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsSearchFunction(), "[{\"state\": \"in_progress\"}]"));
  ASSERT_TRUE(result.get());
  base::ListValue* result_list = NULL;
  ASSERT_TRUE(result->GetAsList(&result_list));
  ASSERT_EQ(1UL, result_list->GetSize());
}

// Test the |limit| option for search().
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_SearchLimit) {
  DownloadManager::DownloadVector items;
  CreateSlowTestDownloads(2, &items);
  ScopedItemVectorCanceller delete_items(&items);

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsSearchFunction(), "[{\"limit\": 1}]"));
  ASSERT_TRUE(result.get());
  base::ListValue* result_list = NULL;
  ASSERT_TRUE(result->GetAsList(&result_list));
  ASSERT_EQ(1UL, result_list->GetSize());
}

// Test invalid search parameters.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
    DownloadExtensionTest_SearchInvalid) {
  std::string error = RunFunctionAndReturnError(
      new DownloadsSearchFunction(), "[{\"filenameRegex\": \"(\"}]");
  EXPECT_STREQ(errors::kInvalidFilter,
      error.c_str());
  error = RunFunctionAndReturnError(
      new DownloadsSearchFunction(), "[{\"orderBy\": [\"goat\"]}]");
  EXPECT_STREQ(errors::kInvalidOrderBy,
      error.c_str());
  error = RunFunctionAndReturnError(
      new DownloadsSearchFunction(), "[{\"limit\": -1}]");
  EXPECT_STREQ(errors::kInvalidQueryLimit,
      error.c_str());
}

// Test searching using multiple conditions through multiple downloads.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
    DownloadExtensionTest_SearchPlural) {
  const HistoryDownloadInfo kHistoryInfo[] = {
    { FILE_PATH_LITERAL("aaa"),
      DownloadItem::CANCELLED,
      content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS },
    { FILE_PATH_LITERAL("zzz"),
      DownloadItem::COMPLETE,
      content::DOWNLOAD_DANGER_TYPE_DANGEROUS_CONTENT },
    { FILE_PATH_LITERAL("baz"),
      DownloadItem::COMPLETE,
      content::DOWNLOAD_DANGER_TYPE_DANGEROUS_CONTENT },
  };
  DownloadManager::DownloadVector items;
  ASSERT_TRUE(CreateHistoryDownloads(kHistoryInfo, arraysize(kHistoryInfo),
                                     &items));

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsSearchFunction(), "[{"
      "\"state\": \"complete\", "
      "\"danger\": \"content\", "
      "\"orderBy\": [\"filename\"], "
      "\"limit\": 1}]"));
  ASSERT_TRUE(result.get());
  base::ListValue* result_list = NULL;
  ASSERT_TRUE(result->GetAsList(&result_list));
  ASSERT_EQ(1UL, result_list->GetSize());
  base::DictionaryValue* item_value = NULL;
  ASSERT_TRUE(result_list->GetDictionary(0, &item_value));
  base::FilePath::StringType item_name;
  ASSERT_TRUE(item_value->GetString("filename", &item_name));
  ASSERT_EQ(items[2]->GetTargetFilePath().value(), item_name);
}

// Test that incognito downloads are only visible in incognito contexts, and
// test that on-record downloads are visible in both incognito and on-record
// contexts, for DownloadsSearchFunction, DownloadsPauseFunction,
// DownloadsResumeFunction, and DownloadsCancelFunction.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
    DownloadExtensionTest_SearchPauseResumeCancelGetFileIconIncognito) {
  scoped_ptr<base::Value> result_value;
  base::ListValue* result_list = NULL;
  base::DictionaryValue* result_dict = NULL;
  base::FilePath::StringType filename;
  bool is_incognito = false;
  std::string error;
  std::string on_item_arg;
  std::string off_item_arg;
  std::string result_string;

  // Set up one on-record item and one off-record item.
  // Set up the off-record item first because otherwise there are mysteriously 3
  // items total instead of 2.
  // TODO(benjhayden): Figure out where the third item comes from.
  GoOffTheRecord();
  DownloadItem* off_item = CreateSlowTestDownload();
  ASSERT_TRUE(off_item);
  off_item_arg = DownloadItemIdAsArgList(off_item);

  GoOnTheRecord();
  DownloadItem* on_item = CreateSlowTestDownload();
  ASSERT_TRUE(on_item);
  on_item_arg = DownloadItemIdAsArgList(on_item);
  ASSERT_TRUE(on_item->GetTargetFilePath() != off_item->GetTargetFilePath());

  // Extensions running in the incognito window should have access to both
  // items because the Test extension is in spanning mode.
  GoOffTheRecord();
  result_value.reset(RunFunctionAndReturnResult(
      new DownloadsSearchFunction(), "[{}]"));
  ASSERT_TRUE(result_value.get());
  ASSERT_TRUE(result_value->GetAsList(&result_list));
  ASSERT_EQ(2UL, result_list->GetSize());
  ASSERT_TRUE(result_list->GetDictionary(0, &result_dict));
  ASSERT_TRUE(result_dict->GetString("filename", &filename));
  ASSERT_TRUE(result_dict->GetBoolean("incognito", &is_incognito));
  EXPECT_TRUE(on_item->GetTargetFilePath() == base::FilePath(filename));
  EXPECT_FALSE(is_incognito);
  ASSERT_TRUE(result_list->GetDictionary(1, &result_dict));
  ASSERT_TRUE(result_dict->GetString("filename", &filename));
  ASSERT_TRUE(result_dict->GetBoolean("incognito", &is_incognito));
  EXPECT_TRUE(off_item->GetTargetFilePath() == base::FilePath(filename));
  EXPECT_TRUE(is_incognito);

  // Extensions running in the on-record window should have access only to the
  // on-record item.
  GoOnTheRecord();
  result_value.reset(RunFunctionAndReturnResult(
      new DownloadsSearchFunction(), "[{}]"));
  ASSERT_TRUE(result_value.get());
  ASSERT_TRUE(result_value->GetAsList(&result_list));
  ASSERT_EQ(1UL, result_list->GetSize());
  ASSERT_TRUE(result_list->GetDictionary(0, &result_dict));
  ASSERT_TRUE(result_dict->GetString("filename", &filename));
  EXPECT_TRUE(on_item->GetTargetFilePath() == base::FilePath(filename));
  ASSERT_TRUE(result_dict->GetBoolean("incognito", &is_incognito));
  EXPECT_FALSE(is_incognito);

  // Pausing/Resuming the off-record item while on the record should return an
  // error. Cancelling "non-existent" downloads is not an error.
  error = RunFunctionAndReturnError(new DownloadsPauseFunction(), off_item_arg);
  EXPECT_STREQ(errors::kInvalidId,
               error.c_str());
  error = RunFunctionAndReturnError(new DownloadsResumeFunction(),
                                    off_item_arg);
  EXPECT_STREQ(errors::kInvalidId,
               error.c_str());
  error = RunFunctionAndReturnError(
      new DownloadsGetFileIconFunction(),
      base::StringPrintf("[%d, {}]", off_item->GetId()));
  EXPECT_STREQ(errors::kInvalidId,
               error.c_str());

  GoOffTheRecord();

  // Do the FileIcon test for both the on- and off-items while off the record.
  // NOTE(benjhayden): This does not include the FileIcon test from history,
  // just active downloads. This shouldn't be a problem.
  EXPECT_TRUE(RunFunctionAndReturnString(MockedGetFileIconFunction(
          on_item->GetTargetFilePath(), IconLoader::NORMAL, "foo"),
      base::StringPrintf("[%d, {}]", on_item->GetId()), &result_string));
  EXPECT_TRUE(RunFunctionAndReturnString(MockedGetFileIconFunction(
          off_item->GetTargetFilePath(), IconLoader::NORMAL, "foo"),
      base::StringPrintf("[%d, {}]", off_item->GetId()), &result_string));

  // Do the pause/resume/cancel test for both the on- and off-items while off
  // the record.
  EXPECT_TRUE(RunFunction(new DownloadsPauseFunction(), on_item_arg));
  EXPECT_TRUE(on_item->IsPaused());
  EXPECT_TRUE(RunFunction(new DownloadsPauseFunction(), on_item_arg));
  EXPECT_TRUE(on_item->IsPaused());
  EXPECT_TRUE(RunFunction(new DownloadsResumeFunction(), on_item_arg));
  EXPECT_FALSE(on_item->IsPaused());
  EXPECT_TRUE(RunFunction(new DownloadsResumeFunction(), on_item_arg));
  EXPECT_FALSE(on_item->IsPaused());
  EXPECT_TRUE(RunFunction(new DownloadsPauseFunction(), on_item_arg));
  EXPECT_TRUE(on_item->IsPaused());
  EXPECT_TRUE(RunFunction(new DownloadsCancelFunction(), on_item_arg));
  EXPECT_EQ(DownloadItem::CANCELLED, on_item->GetState());
  EXPECT_TRUE(RunFunction(new DownloadsCancelFunction(), on_item_arg));
  EXPECT_EQ(DownloadItem::CANCELLED, on_item->GetState());
  error = RunFunctionAndReturnError(new DownloadsPauseFunction(), on_item_arg);
  EXPECT_STREQ(errors::kNotInProgress, error.c_str());
  error = RunFunctionAndReturnError(new DownloadsResumeFunction(), on_item_arg);
  EXPECT_STREQ(errors::kNotResumable, error.c_str());
  EXPECT_TRUE(RunFunction(new DownloadsPauseFunction(), off_item_arg));
  EXPECT_TRUE(off_item->IsPaused());
  EXPECT_TRUE(RunFunction(new DownloadsPauseFunction(), off_item_arg));
  EXPECT_TRUE(off_item->IsPaused());
  EXPECT_TRUE(RunFunction(new DownloadsResumeFunction(), off_item_arg));
  EXPECT_FALSE(off_item->IsPaused());
  EXPECT_TRUE(RunFunction(new DownloadsResumeFunction(), off_item_arg));
  EXPECT_FALSE(off_item->IsPaused());
  EXPECT_TRUE(RunFunction(new DownloadsPauseFunction(), off_item_arg));
  EXPECT_TRUE(off_item->IsPaused());
  EXPECT_TRUE(RunFunction(new DownloadsCancelFunction(), off_item_arg));
  EXPECT_EQ(DownloadItem::CANCELLED, off_item->GetState());
  EXPECT_TRUE(RunFunction(new DownloadsCancelFunction(), off_item_arg));
  EXPECT_EQ(DownloadItem::CANCELLED, off_item->GetState());
  error = RunFunctionAndReturnError(new DownloadsPauseFunction(), off_item_arg);
  EXPECT_STREQ(errors::kNotInProgress, error.c_str());
  error = RunFunctionAndReturnError(new DownloadsResumeFunction(),
                                    off_item_arg);
  EXPECT_STREQ(errors::kNotResumable, error.c_str());
}

// Test that we can start a download and that the correct sequence of events is
// fired for it.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_Download_Basic) {
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();
  GoOnTheRecord();

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

// Test that we can start a download from an incognito context, and that the
// download knows that it's incognito.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_Download_Incognito) {
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  GoOffTheRecord();
  std::string download_url = test_server()->GetURL("slow?0").spec();

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": true,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\":%d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\":%d,"
                          "  \"state\": {"
                          "    \"current\": \"complete\","
                          "    \"previous\": \"in_progress\"}}]",
                          result_id)));
}

#if defined(OS_WIN)
// This test is very flaky on Win. http://crbug.com/248438
#define MAYBE_DownloadExtensionTest_Download_UnsafeHeaders \
    DISABLED_DownloadExtensionTest_Download_UnsafeHeaders
#else
#define MAYBE_DownloadExtensionTest_Download_UnsafeHeaders \
    DownloadExtensionTest_Download_UnsafeHeaders
#endif

// Test that we disallow certain headers case-insensitively.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       MAYBE_DownloadExtensionTest_Download_UnsafeHeaders) {
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  GoOnTheRecord();

  static const char* kUnsafeHeaders[] = {
    "Accept-chArsEt",
    "accept-eNcoding",
    "coNNection",
    "coNteNt-leNgth",
    "cooKIE",
    "cOOkie2",
    "coNteNt-traNsfer-eNcodiNg",
    "dAtE",
    "ExpEcT",
    "hOsT",
    "kEEp-aLivE",
    "rEfErEr",
    "tE",
    "trAilER",
    "trANsfer-eNcodiNg",
    "upGRAde",
    "usER-agENt",
    "viA",
    "pRoxY-",
    "sEc-",
    "pRoxY-probably-not-evil",
    "sEc-probably-not-evil",
    "oRiGiN",
    "Access-Control-Request-Headers",
    "Access-Control-Request-Method",
  };

  for (size_t index = 0; index < arraysize(kUnsafeHeaders); ++index) {
    std::string download_url = test_server()->GetURL("slow?0").spec();
    EXPECT_STREQ(errors::kInvalidHeaderUnsafe,
                  RunFunctionAndReturnError(new DownloadsDownloadFunction(),
                                            base::StringPrintf(
        "[{\"url\": \"%s\","
        "  \"filename\": \"unsafe-header-%d.txt\","
        "  \"headers\": [{"
        "    \"name\": \"%s\","
        "    \"value\": \"unsafe\"}]}]",
        download_url.c_str(),
        static_cast<int>(index),
        kUnsafeHeaders[index])).c_str());
  }
}

// Tests that invalid header names and values are rejected.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_Download_InvalidHeaders) {
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  GoOnTheRecord();
  std::string download_url = test_server()->GetURL("slow?0").spec();
  EXPECT_STREQ(errors::kInvalidHeaderName,
               RunFunctionAndReturnError(new DownloadsDownloadFunction(),
                                         base::StringPrintf(
      "[{\"url\": \"%s\","
      "  \"filename\": \"unsafe-header-crlf.txt\","
      "  \"headers\": [{"
      "    \"name\": \"Header\\r\\nSec-Spoof: Hey\\r\\nX-Split:X\","
      "    \"value\": \"unsafe\"}]}]",
      download_url.c_str())).c_str());

  EXPECT_STREQ(errors::kInvalidHeaderValue,
               RunFunctionAndReturnError(new DownloadsDownloadFunction(),
                                         base::StringPrintf(
      "[{\"url\": \"%s\","
      "  \"filename\": \"unsafe-header-crlf.txt\","
      "  \"headers\": [{"
      "    \"name\": \"Invalid-value\","
      "    \"value\": \"hey\\r\\nSec-Spoof: Hey\"}]}]",
      download_url.c_str())).c_str());
}

#if defined(OS_WIN)
#define MAYBE_DownloadExtensionTest_Download_Subdirectory\
        DISABLED_DownloadExtensionTest_Download_Subdirectory
#else
#define MAYBE_DownloadExtensionTest_Download_Subdirectory\
        DownloadExtensionTest_Download_Subdirectory
#endif
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       MAYBE_DownloadExtensionTest_Download_Subdirectory) {
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();
  GoOnTheRecord();

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\","
          "  \"filename\": \"sub/dir/ect/ory.txt\"}]",
          download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("sub/dir/ect/ory.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

// Test that invalid filenames are disallowed.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_Download_InvalidFilename) {
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();
  GoOnTheRecord();

  EXPECT_STREQ(errors::kInvalidFilename,
                RunFunctionAndReturnError(new DownloadsDownloadFunction(),
                                          base::StringPrintf(
      "[{\"url\": \"%s\","
      "  \"filename\": \"../../../../../etc/passwd\"}]",
      download_url.c_str())).c_str());
}

// flaky on mac: crbug.com/392288
#if defined(OS_MACOSX)
#define MAYBE_DownloadExtensionTest_Download_InvalidURLs \
        DISABLED_DownloadExtensionTest_Download_InvalidURLs
#else
#define MAYBE_DownloadExtensionTest_Download_InvalidURLs \
        DownloadExtensionTest_Download_InvalidURLs
#endif

// Test that downloading invalid URLs immediately returns kInvalidURLError.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       MAYBE_DownloadExtensionTest_Download_InvalidURLs) {
  LoadExtension("downloads_split");
  GoOnTheRecord();

  static const char* kInvalidURLs[] = {
    "foo bar",
    "../hello",
    "/hello",
    "http://",
    "#frag",
    "foo/bar.html#frag",
    "google.com/",
  };

  for (size_t index = 0; index < arraysize(kInvalidURLs); ++index) {
    EXPECT_STREQ(errors::kInvalidURL,
                  RunFunctionAndReturnError(new DownloadsDownloadFunction(),
                                            base::StringPrintf(
        "[{\"url\": \"%s\"}]", kInvalidURLs[index])).c_str())
      << kInvalidURLs[index];
  }

  EXPECT_STREQ("NETWORK_INVALID_REQUEST", RunFunctionAndReturnError(
      new DownloadsDownloadFunction(),
      "[{\"url\": \"javascript:document.write(\\\"hello\\\");\"}]").c_str());
  EXPECT_STREQ("NETWORK_INVALID_REQUEST", RunFunctionAndReturnError(
      new DownloadsDownloadFunction(),
      "[{\"url\": \"javascript:return false;\"}]").c_str());
  EXPECT_STREQ("NETWORK_FAILED", RunFunctionAndReturnError(
      new DownloadsDownloadFunction(),
      "[{\"url\": \"ftp://example.com/example.txt\"}]").c_str());
}

// TODO(benjhayden): Set up a test ftp server, add ftp://localhost* to
// permissions, test downloading from ftp.

// Valid URLs plus fragments are still valid URLs.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_Download_URLFragment) {
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0#fragment").spec();
  GoOnTheRecord();

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

// conflictAction may be specified without filename.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_Download_ConflictAction) {
  static char kFilename[] = "download.txt";
  LoadExtension("downloads_split");
  std::string download_url = "data:text/plain,hello";
  GoOnTheRecord();

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
      "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename(kFilename).c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));

  result.reset(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\",  \"conflictAction\": \"overwrite\"}]",
          download_url.c_str())));
  ASSERT_TRUE(result.get());
  result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller2(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename(kFilename).c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

// Valid data URLs are valid URLs.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_Download_DataURL) {
  LoadExtension("downloads_split");
  std::string download_url = "data:text/plain,hello";
  GoOnTheRecord();

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
        new DownloadsDownloadFunction(), base::StringPrintf(
      "[{\"url\": \"%s\","
      "  \"filename\": \"data.txt\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("data.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

// Valid file URLs are valid URLs.
#if defined(OS_WIN)
// Disabled due to crbug.com/175711
#define MAYBE_DownloadExtensionTest_Download_File \
        DISABLED_DownloadExtensionTest_Download_File
#else
#define MAYBE_DownloadExtensionTest_Download_File \
        DownloadExtensionTest_Download_File
#endif
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       MAYBE_DownloadExtensionTest_Download_File) {
  GoOnTheRecord();
  LoadExtension("downloads_split");
  std::string download_url = "file:///";
#if defined(OS_WIN)
  download_url += "C:/";
#endif

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
        new DownloadsDownloadFunction(), base::StringPrintf(
      "[{\"url\": \"%s\","
      "  \"filename\": \"file.txt\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"mime\": \"text/html\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("file.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

// Test that auth-basic-succeed would fail if the resource requires the
// Authorization header and chrome fails to propagate it back to the server.
// This tests both that testserver.py does not succeed when it should fail as
// well as how the downloads extension API exposes the failure to extensions.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_Download_AuthBasic_Fail) {
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("auth-basic").spec();
  GoOnTheRecord();

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
        new DownloadsDownloadFunction(), base::StringPrintf(
      "[{\"url\": \"%s\","
      "  \"filename\": \"auth-basic-fail.txt\"}]",
      download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitForInterruption(
      item,
      content::DOWNLOAD_INTERRUPT_REASON_SERVER_UNAUTHORIZED,
      base::StringPrintf("[{\"danger\": \"safe\","
                         "  \"incognito\": false,"
                         "  \"mime\": \"text/html\","
                         "  \"paused\": false,"
                         "  \"url\": \"%s\"}]",
                         download_url.c_str())));
}

// Test that DownloadsDownloadFunction propagates |headers| to the URLRequest.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_Download_Headers) {
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("files/downloads/"
      "a_zip_file.zip?expected_headers=Foo:bar&expected_headers=Qx:yo").spec();
  GoOnTheRecord();

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
        new DownloadsDownloadFunction(), base::StringPrintf(
      "[{\"url\": \"%s\","
      "  \"filename\": \"headers-succeed.txt\","
      "  \"headers\": ["
      "    {\"name\": \"Foo\", \"value\": \"bar\"},"
      "    {\"name\": \"Qx\", \"value\":\"yo\"}]}]",
      download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"mime\": \"application/octet-stream\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("headers-succeed.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

// Test that headers-succeed would fail if the resource requires the headers and
// chrome fails to propagate them back to the server.  This tests both that
// testserver.py does not succeed when it should fail as well as how the
// downloads extension api exposes the failure to extensions.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_Download_Headers_Fail) {
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("files/downloads/"
      "a_zip_file.zip?expected_headers=Foo:bar&expected_headers=Qx:yo").spec();
  GoOnTheRecord();

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
        new DownloadsDownloadFunction(), base::StringPrintf(
      "[{\"url\": \"%s\","
      "  \"filename\": \"headers-fail.txt\"}]",
      download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitForInterruption(
      item,
      content::DOWNLOAD_INTERRUPT_REASON_SERVER_BAD_CONTENT,
      base::StringPrintf("[{\"danger\": \"safe\","
                         "  \"incognito\": false,"
                         "  \"bytesReceived\": 0.0,"
                         "  \"fileSize\": 0.0,"
                         "  \"mime\": \"\","
                         "  \"paused\": false,"
                         "  \"url\": \"%s\"}]",
                         download_url.c_str())));
}

// Test that DownloadsDownloadFunction propagates the Authorization header
// correctly.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_Download_AuthBasic) {
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("auth-basic").spec();
  // This is just base64 of 'username:secret'.
  static const char* kAuthorization = "dXNlcm5hbWU6c2VjcmV0";
  GoOnTheRecord();

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
        new DownloadsDownloadFunction(), base::StringPrintf(
      "[{\"url\": \"%s\","
      "  \"filename\": \"auth-basic-succeed.txt\","
      "  \"headers\": [{"
      "    \"name\": \"Authorization\","
      "    \"value\": \"Basic %s\"}]}]",
      download_url.c_str(), kAuthorization)));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"bytesReceived\": 0.0,"
                          "  \"fileSize\": 0.0,"
                          "  \"mime\": \"text/html\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

// Test that DownloadsDownloadFunction propagates the |method| and |body|
// parameters to the URLRequest.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_Download_Post) {
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("files/post/downloads/"
      "a_zip_file.zip?expected_body=BODY").spec();
  GoOnTheRecord();

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
        new DownloadsDownloadFunction(), base::StringPrintf(
      "[{\"url\": \"%s\","
      "  \"filename\": \"post-succeed.txt\","
      "  \"method\": \"POST\","
      "  \"body\": \"BODY\"}]",
      download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"mime\": \"application/octet-stream\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("post-succeed.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

// Test that downloadPostSuccess would fail if the resource requires the POST
// method, and chrome fails to propagate the |method| parameter back to the
// server. This tests both that testserver.py does not succeed when it should
// fail, and this tests how the downloads extension api exposes the failure to
// extensions.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_Download_Post_Get) {
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("files/post/downloads/"
      "a_zip_file.zip?expected_body=BODY").spec();
  GoOnTheRecord();

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
        new DownloadsDownloadFunction(), base::StringPrintf(
      "[{\"url\": \"%s\","
      "  \"body\": \"BODY\","
      "  \"filename\": \"post-get.txt\"}]",
      download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitForInterruption(
      item,
      content::DOWNLOAD_INTERRUPT_REASON_SERVER_BAD_CONTENT,
      base::StringPrintf("[{\"danger\": \"safe\","
                         "  \"incognito\": false,"
                         "  \"mime\": \"\","
                         "  \"paused\": false,"
                         "  \"id\": %d,"
                         "  \"url\": \"%s\"}]",
                         result_id,
                         download_url.c_str())));
}

// Test that downloadPostSuccess would fail if the resource requires the POST
// method, and chrome fails to propagate the |body| parameter back to the
// server. This tests both that testserver.py does not succeed when it should
// fail, and this tests how the downloads extension api exposes the failure to
// extensions.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_Download_Post_NoBody) {
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("files/post/downloads/"
      "a_zip_file.zip?expected_body=BODY").spec();
  GoOnTheRecord();

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
      "[{\"url\": \"%s\","
      "  \"method\": \"POST\","
      "  \"filename\": \"post-nobody.txt\"}]",
      download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitForInterruption(
      item,
      content::DOWNLOAD_INTERRUPT_REASON_SERVER_BAD_CONTENT,
      base::StringPrintf("[{\"danger\": \"safe\","
                         "  \"incognito\": false,"
                         "  \"mime\": \"\","
                         "  \"paused\": false,"
                         "  \"id\": %d,"
                         "  \"url\": \"%s\"}]",
                         result_id,
                         download_url.c_str())));
}

// Test that cancel()ing an in-progress download causes its state to transition
// to interrupted, and test that that state transition is detectable by an
// onChanged event listener.  TODO(benjhayden): Test other sources of
// interruptions such as server death.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_Download_Cancel) {
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL(
      "download-known-size").spec();
  GoOnTheRecord();

  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"mime\": \"application/octet-stream\","
                          "  \"paused\": false,"
                          "  \"id\": %d,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  item->Cancel(true);
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"error\": {\"current\":\"USER_CANCELED\"},"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"interrupted\"}}]",
                          result_id)));
}

// flaky on mac: crbug.com/392288
#if defined(OS_MACOSX)
#define MAYBE_DownloadExtensionTest_Download_FileSystemURL \
        DISABLED_DownloadExtensionTest_Download_FileSystemURL
#else
#define MAYBE_DownloadExtensionTest_Download_FileSystemURL \
        DownloadExtensionTest_Download_FileSystemURL
#endif

// Test downloading filesystem: URLs.
// NOTE: chrome disallows creating HTML5 FileSystem Files in incognito.
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       MAYBE_DownloadExtensionTest_Download_FileSystemURL) {
  static const char* kPayloadData = "on the record\ndata";
  GoOnTheRecord();
  LoadExtension("downloads_split");

  const std::string download_url = "filesystem:" + GetExtensionURL() +
    "temporary/on_record.txt";

  // Setup a file in the filesystem which we can download.
  ASSERT_TRUE(HTML5FileWriter::CreateFileForTesting(
      BrowserContext::GetDefaultStoragePartition(browser()->profile())
          ->GetFileSystemContext(),
      storage::FileSystemURL::CreateForTest(GURL(download_url)),
      kPayloadData,
      strlen(kPayloadData)));

  // Now download it.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));

  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("on_record.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
  std::string disk_data;
  EXPECT_TRUE(base::ReadFileToString(item->GetTargetFilePath(), &disk_data));
  EXPECT_STREQ(kPayloadData, disk_data.c_str());
}

IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_OnDeterminingFilename_NoChange) {
  GoOnTheRecord();
  LoadExtension("downloads_split");
  AddFilenameDeterminer();
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  // Wait for the onCreated and onDeterminingFilename events.
  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename.
  std::string error;
  ASSERT_TRUE(ExtensionDownloadsEventRouter::DetermineFilename(
      browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_EQ("", error);

  // The download should complete successfully.
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

// Disabled due to cross-platform flakes; http://crbug.com/370531.
IN_PROC_BROWSER_TEST_F(
    DownloadExtensionTest,
    DISABLED_DownloadExtensionTest_OnDeterminingFilename_Timeout) {
  GoOnTheRecord();
  LoadExtension("downloads_split");
  AddFilenameDeterminer();
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();

  ExtensionDownloadsEventRouter::SetDetermineFilenameTimeoutSecondsForTesting(
      0);

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  // Wait for the onCreated and onDeterminingFilename events.
  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
      base::StringPrintf("[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(
      downloads::OnDeterminingFilename::kEventName,
      base::StringPrintf("[{\"id\": %d,"
                         "  \"filename\":\"slow.txt\"}]",
                         result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Do not respond to the onDeterminingFilename.

  // The download should complete successfully.
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
      base::StringPrintf("[{\"id\": %d,"
                         "  \"filename\": {"
                         "    \"previous\": \"\","
                         "    \"current\": \"%s\"}}]",
                         result_id,
                         GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
      base::StringPrintf("[{\"id\": %d,"
                         "  \"state\": {"
                         "    \"previous\": \"in_progress\","
                         "    \"current\": \"complete\"}}]",
                         result_id)));
}

IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_OnDeterminingFilename_Twice) {
  GoOnTheRecord();
  LoadExtension("downloads_split");
  AddFilenameDeterminer();
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  // Wait for the onCreated and onDeterminingFilename events.
  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
      base::StringPrintf("[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(
      downloads::OnDeterminingFilename::kEventName,
      base::StringPrintf("[{\"id\": %d,"
                         "  \"filename\":\"slow.txt\"}]",
                         result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename.
  std::string error;
  ASSERT_TRUE(ExtensionDownloadsEventRouter::DetermineFilename(
      browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_EQ("", error);

  // Calling DetermineFilename again should return an error instead of calling
  // DownloadTargetDeterminer.
  ASSERT_FALSE(ExtensionDownloadsEventRouter::DetermineFilename(
      browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(FILE_PATH_LITERAL("different")),
      downloads::FILENAME_CONFLICT_ACTION_OVERWRITE,
      &error));
  EXPECT_EQ(errors::kTooManyListeners, error);

  // The download should complete successfully.
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
      base::StringPrintf("[{\"id\": %d,"
                         "  \"filename\": {"
                         "    \"previous\": \"\","
                         "    \"current\": \"%s\"}}]",
                         result_id,
                         GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
      base::StringPrintf("[{\"id\": %d,"
                         "  \"state\": {"
                         "    \"previous\": \"in_progress\","
                         "    \"current\": \"complete\"}}]",
                         result_id)));
}

IN_PROC_BROWSER_TEST_F(
    DownloadExtensionTest,
    DownloadExtensionTest_OnDeterminingFilename_DangerousOverride) {
  GoOnTheRecord();
  LoadExtension("downloads_split");
  AddFilenameDeterminer();
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename.
  std::string error;
  ASSERT_TRUE(ExtensionDownloadsEventRouter::DetermineFilename(
      browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(FILE_PATH_LITERAL("overridden.swf")),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_EQ("", error);

  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"danger\": {"
                          "    \"previous\":\"safe\","
                          "    \"current\":\"file\"}}]",
                          result_id)));

  item->ValidateDangerousDownload();
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"danger\": {"
                          "    \"previous\":\"file\","
                          "    \"current\":\"accepted\"}}]",
                          result_id)));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
  EXPECT_EQ(downloads_directory().AppendASCII("overridden.swf"),
            item->GetTargetFilePath());
}

IN_PROC_BROWSER_TEST_F(
    DownloadExtensionTest,
    DownloadExtensionTest_OnDeterminingFilename_ReferencesParentInvalid) {
  GoOnTheRecord();
  LoadExtension("downloads_split");
  AddFilenameDeterminer();
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename.
  std::string error;
  ASSERT_FALSE(ExtensionDownloadsEventRouter::DetermineFilename(
      browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(FILE_PATH_LITERAL("sneaky/../../sneaky.txt")),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_STREQ(errors::kInvalidFilename, error.c_str());
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

IN_PROC_BROWSER_TEST_F(
    DownloadExtensionTest,
    DownloadExtensionTest_OnDeterminingFilename_IllegalFilename) {
  GoOnTheRecord();
  LoadExtension("downloads_split");
  AddFilenameDeterminer();
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename.
  std::string error;
  ASSERT_FALSE(ExtensionDownloadsEventRouter::DetermineFilename(
      browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(FILE_PATH_LITERAL("<")),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_STREQ(errors::kInvalidFilename, error.c_str());
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

IN_PROC_BROWSER_TEST_F(
    DownloadExtensionTest,
    DownloadExtensionTest_OnDeterminingFilename_IllegalFilenameExtension) {
  GoOnTheRecord();
  LoadExtension("downloads_split");
  AddFilenameDeterminer();
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename.
  std::string error;
  ASSERT_FALSE(ExtensionDownloadsEventRouter::DetermineFilename(
      browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(FILE_PATH_LITERAL(
          "My Computer.{20D04FE0-3AEA-1069-A2D8-08002B30309D}/foo")),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_STREQ(errors::kInvalidFilename, error.c_str());
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}
#if defined(OS_WIN)
#define MAYBE_DownloadExtensionTest_OnDeterminingFilename_ReservedFilename\
  DISABLED_DownloadExtensionTest_OnDeterminingFilename_ReservedFilename
#else
#define MAYBE_DownloadExtensionTest_OnDeterminingFilename_ReservedFilename\
  DownloadExtensionTest_OnDeterminingFilename_ReservedFilename
#endif
IN_PROC_BROWSER_TEST_F(
    DownloadExtensionTest,
    MAYBE_DownloadExtensionTest_OnDeterminingFilename_ReservedFilename) {
  GoOnTheRecord();
  LoadExtension("downloads_split");
  AddFilenameDeterminer();
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename.
  std::string error;
  ASSERT_FALSE(ExtensionDownloadsEventRouter::DetermineFilename(
      browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(FILE_PATH_LITERAL("con.foo")),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_STREQ(errors::kInvalidFilename, error.c_str());
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

IN_PROC_BROWSER_TEST_F(
    DownloadExtensionTest,
    DownloadExtensionTest_OnDeterminingFilename_CurDirInvalid) {
  GoOnTheRecord();
  LoadExtension("downloads_split");
  AddFilenameDeterminer();
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename.
  std::string error;
  ASSERT_FALSE(ExtensionDownloadsEventRouter::DetermineFilename(
      browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(FILE_PATH_LITERAL(".")),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_STREQ(errors::kInvalidFilename, error.c_str());
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

IN_PROC_BROWSER_TEST_F(
    DownloadExtensionTest,
    DownloadExtensionTest_OnDeterminingFilename_ParentDirInvalid) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  GoOnTheRecord();
  LoadExtension("downloads_split");
  AddFilenameDeterminer();
  std::string download_url = test_server()->GetURL("slow?0").spec();

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename.
  std::string error;
  ASSERT_FALSE(ExtensionDownloadsEventRouter::DetermineFilename(
      browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(FILE_PATH_LITERAL("..")),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_STREQ(errors::kInvalidFilename, error.c_str());
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

IN_PROC_BROWSER_TEST_F(
    DownloadExtensionTest,
    DownloadExtensionTest_OnDeterminingFilename_AbsPathInvalid) {
  GoOnTheRecord();
  LoadExtension("downloads_split");
  AddFilenameDeterminer();
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename. Absolute paths should be rejected.
  std::string error;
  ASSERT_FALSE(ExtensionDownloadsEventRouter::DetermineFilename(
      browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      downloads_directory().Append(FILE_PATH_LITERAL("sneaky.txt")),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_STREQ(errors::kInvalidFilename, error.c_str());

  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

IN_PROC_BROWSER_TEST_F(
    DownloadExtensionTest,
    DownloadExtensionTest_OnDeterminingFilename_EmptyBasenameInvalid) {
  GoOnTheRecord();
  LoadExtension("downloads_split");
  AddFilenameDeterminer();
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename. Empty basenames should be rejected.
  std::string error;
  ASSERT_FALSE(ExtensionDownloadsEventRouter::DetermineFilename(
      browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(FILE_PATH_LITERAL("foo/")),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_STREQ(errors::kInvalidFilename, error.c_str());

  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

// conflictAction may be specified without filename.
IN_PROC_BROWSER_TEST_F(
    DownloadExtensionTest,
    DownloadExtensionTest_OnDeterminingFilename_Overwrite) {
  GoOnTheRecord();
  LoadExtension("downloads_split");
  AddFilenameDeterminer();
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());
  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename.
  std::string error;
  ASSERT_TRUE(ExtensionDownloadsEventRouter::DetermineFilename(
      browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_EQ("", error);

  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));

  // Start downloading a file.
  result.reset(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller2(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename.
  // Also test that DetermineFilename allows (chrome) extensions to set
  // filenames without (filename) extensions. (Don't ask about v8 extensions or
  // python extensions or kernel extensions or firefox extensions...)
  error = "";
  ASSERT_TRUE(ExtensionDownloadsEventRouter::DetermineFilename(
      browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(),
      downloads::FILENAME_CONFLICT_ACTION_OVERWRITE,
      &error));
  EXPECT_EQ("", error);

  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

IN_PROC_BROWSER_TEST_F(
    DownloadExtensionTest,
    DownloadExtensionTest_OnDeterminingFilename_Override) {
  GoOnTheRecord();
  LoadExtension("downloads_split");
  AddFilenameDeterminer();
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());
  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename.
  std::string error;
  ASSERT_TRUE(ExtensionDownloadsEventRouter::DetermineFilename(
      browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_EQ("", error);

  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("slow.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));

  // Start downloading a file.
  result.reset(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller2(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename.
  // Also test that DetermineFilename allows (chrome) extensions to set
  // filenames without (filename) extensions. (Don't ask about v8 extensions or
  // python extensions or kernel extensions or firefox extensions...)
  error = "";
  ASSERT_TRUE(ExtensionDownloadsEventRouter::DetermineFilename(
      browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(FILE_PATH_LITERAL("foo")),
      downloads::FILENAME_CONFLICT_ACTION_OVERWRITE,
      &error));
  EXPECT_EQ("", error);

  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("foo").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

// TODO test precedence rules: install_time

#if defined(OS_MACOSX)
#define MAYBE_DownloadExtensionTest_OnDeterminingFilename_RemoveFilenameDeterminer \
  DISABLED_DownloadExtensionTest_OnDeterminingFilename_RemoveFilenameDeterminer
#else
#define MAYBE_DownloadExtensionTest_OnDeterminingFilename_RemoveFilenameDeterminer \
  DownloadExtensionTest_OnDeterminingFilename_RemoveFilenameDeterminer
#endif
IN_PROC_BROWSER_TEST_F(
    DownloadExtensionTest,
    MAYBE_DownloadExtensionTest_OnDeterminingFilename_RemoveFilenameDeterminer) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  GoOnTheRecord();
  LoadExtension("downloads_split");
  content::RenderProcessHost* host = AddFilenameDeterminer();
  std::string download_url = test_server()->GetURL("slow?0").spec();

  // Start downloading a file.
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Remove a determiner while waiting for it.
  RemoveFilenameDeterminer(host);

  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

IN_PROC_BROWSER_TEST_F(
    DownloadExtensionTest,
    DownloadExtensionTest_OnDeterminingFilename_IncognitoSplit) {
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();

  GoOnTheRecord();
  AddFilenameDeterminer();

  GoOffTheRecord();
  AddFilenameDeterminer();

  // Start an on-record download.
  GoOnTheRecord();
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  // Wait for the onCreated and onDeterminingFilename events.
  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"incognito\": false,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename events.
  std::string error;
  ASSERT_TRUE(ExtensionDownloadsEventRouter::DetermineFilename(
      current_browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(FILE_PATH_LITERAL("42.txt")),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_EQ("", error);

  // The download should complete successfully.
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("42.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));

  // Start an incognito download for comparison.
  GoOffTheRecord();
  result.reset(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller2(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": true,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  // On-Record renderers should not see events for off-record items.
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"incognito\": true,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename.
  error = "";
  ASSERT_TRUE(ExtensionDownloadsEventRouter::DetermineFilename(
      current_browser()->profile(),
      false,
      GetExtensionId(),
      result_id,
      base::FilePath(FILE_PATH_LITERAL("5.txt")),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_EQ("", error);

  // The download should complete successfully.
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("5.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

IN_PROC_BROWSER_TEST_F(
    DownloadExtensionTest,
    DownloadExtensionTest_OnDeterminingFilename_IncognitoSpanning) {
  LoadExtension("downloads_spanning");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  std::string download_url = test_server()->GetURL("slow?0").spec();

  GoOnTheRecord();
  AddFilenameDeterminer();

  // There is a single extension renderer that sees both on-record and
  // off-record events. The extension functions see the on-record profile with
  // include_incognito=true.

  // Start an on-record download.
  GoOnTheRecord();
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  // Wait for the onCreated and onDeterminingFilename events.
  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"incognito\": false,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename events.
  std::string error;
  ASSERT_TRUE(ExtensionDownloadsEventRouter::DetermineFilename(
      current_browser()->profile(),
      true,
      GetExtensionId(),
      result_id,
      base::FilePath(FILE_PATH_LITERAL("42.txt")),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_EQ("", error);

  // The download should complete successfully.
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("42.txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));

  // Start an incognito download for comparison.
  GoOffTheRecord();
  result.reset(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(), base::StringPrintf(
          "[{\"url\": \"%s\"}]", download_url.c_str())));
  ASSERT_TRUE(result.get());
  result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ScopedCancellingItem canceller2(item);
  ASSERT_EQ(download_url, item->GetOriginalUrl().spec());

  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": true,"
                          "  \"id\": %d,"
                          "  \"mime\": \"text/plain\","
                          "  \"paused\": false,"
                          "  \"url\": \"%s\"}]",
                          result_id,
                          download_url.c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"incognito\": true,"
                          "  \"filename\":\"slow.txt\"}]",
                          result_id)));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  // Respond to the onDeterminingFilename.
  error = "";
  ASSERT_TRUE(ExtensionDownloadsEventRouter::DetermineFilename(
      current_browser()->profile(),
      true,
      GetExtensionId(),
      result_id,
      base::FilePath(FILE_PATH_LITERAL("42.txt")),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error));
  EXPECT_EQ("", error);

  // The download should complete successfully.
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          result_id,
                          GetFilename("42 (1).txt").c_str())));
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          result_id)));
}

#if defined(OS_WIN)
// This test is very flaky on Win XP and Aura. http://crbug.com/248438
#define MAYBE_DownloadExtensionTest_OnDeterminingFilename_InterruptedResume \
    DISABLED_DownloadExtensionTest_OnDeterminingFilename_InterruptedResume
#else
#define MAYBE_DownloadExtensionTest_OnDeterminingFilename_InterruptedResume \
    DownloadExtensionTest_OnDeterminingFilename_InterruptedResume
#endif

// Test download interruption while extensions determining filename. Should not
// re-dispatch onDeterminingFilename.
IN_PROC_BROWSER_TEST_F(
    DownloadExtensionTest,
    MAYBE_DownloadExtensionTest_OnDeterminingFilename_InterruptedResume) {
  CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kEnableDownloadResumption);
  LoadExtension("downloads_split");
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(test_server()->Start());
  GoOnTheRecord();
  content::RenderProcessHost* host = AddFilenameDeterminer();

  // Start a download.
  DownloadItem* item = NULL;
  {
    DownloadManager* manager = GetCurrentManager();
    scoped_ptr<content::DownloadTestObserver> observer(
        new JustInProgressDownloadObserver(manager, 1));
    ASSERT_EQ(0, manager->InProgressCount());
    ASSERT_EQ(0, manager->NonMaliciousInProgressCount());
    // Tabs created just for a download are automatically closed, invalidating
    // the download's WebContents. Downloads without WebContents cannot be
    // resumed. http://crbug.com/225901
    ui_test_utils::NavigateToURLWithDisposition(
        current_browser(),
        GURL(URLRequestSlowDownloadJob::kUnknownSizeUrl),
        CURRENT_TAB,
        ui_test_utils::BROWSER_TEST_NONE);
    observer->WaitForFinished();
    EXPECT_EQ(1u, observer->NumDownloadsSeenInState(DownloadItem::IN_PROGRESS));
    DownloadManager::DownloadVector items;
    manager->GetAllDownloads(&items);
    for (DownloadManager::DownloadVector::iterator iter = items.begin();
          iter != items.end(); ++iter) {
      if ((*iter)->GetState() == DownloadItem::IN_PROGRESS) {
        // There should be only one IN_PROGRESS item.
        EXPECT_EQ(NULL, item);
        item = *iter;
      }
    }
    ASSERT_TRUE(item);
  }
  ScopedCancellingItem canceller(item);

  // Wait for the onCreated and onDeterminingFilename event.
  ASSERT_TRUE(WaitFor(downloads::OnCreated::kEventName,
                      base::StringPrintf(
                          "[{\"danger\": \"safe\","
                          "  \"incognito\": false,"
                          "  \"id\": %d,"
                          "  \"mime\": \"application/octet-stream\","
                          "  \"paused\": false}]",
                          item->GetId())));
  ASSERT_TRUE(WaitFor(downloads::OnDeterminingFilename::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"incognito\": false,"
                          "  \"filename\":\"download-unknown-size\"}]",
                          item->GetId())));
  ASSERT_TRUE(item->GetTargetFilePath().empty());
  ASSERT_EQ(DownloadItem::IN_PROGRESS, item->GetState());

  ClearEvents();
  ui_test_utils::NavigateToURLWithDisposition(
      current_browser(),
      GURL(URLRequestSlowDownloadJob::kErrorDownloadUrl),
      NEW_BACKGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Errors caught before filename determination are delayed until after
  // filename determination.
  std::string error;
  ASSERT_TRUE(ExtensionDownloadsEventRouter::DetermineFilename(
      current_browser()->profile(),
      false,
      GetExtensionId(),
      item->GetId(),
      base::FilePath(FILE_PATH_LITERAL("42.txt")),
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY,
      &error))
      << error;
  EXPECT_EQ("", error);
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"filename\": {"
                          "    \"previous\": \"\","
                          "    \"current\": \"%s\"}}]",
                          item->GetId(),
                          GetFilename("42.txt").c_str())));

  content::DownloadUpdatedObserver interrupted(item, base::Bind(
      ItemIsInterrupted));
  ASSERT_TRUE(interrupted.WaitForEvent());
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"error\":{\"current\":\"NETWORK_FAILED\"},"
                          "  \"state\":{"
                          "    \"previous\":\"in_progress\","
                          "    \"current\":\"interrupted\"}}]",
                          item->GetId())));

  ClearEvents();
  // Downloads that are restarted on resumption trigger another download target
  // determination.
  RemoveFilenameDeterminer(host);
  item->Resume();

  // Errors caught before filename determination is complete are delayed until
  // after filename determination so that, on resumption, filename determination
  // does not need to be re-done. So, there will not be a second
  // onDeterminingFilename event.

  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"error\":{\"previous\":\"NETWORK_FAILED\"},"
                          "  \"state\":{"
                          "    \"previous\":\"interrupted\","
                          "    \"current\":\"in_progress\"}}]",
                          item->GetId())));

  ClearEvents();
  FinishPendingSlowDownloads();

  // The download should complete successfully.
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d,"
                          "  \"state\": {"
                          "    \"previous\": \"in_progress\","
                          "    \"current\": \"complete\"}}]",
                          item->GetId())));
}

IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       DownloadExtensionTest_SetShelfEnabled) {
  LoadExtension("downloads_split");
  EXPECT_TRUE(RunFunction(new DownloadsSetShelfEnabledFunction(), "[false]"));
  EXPECT_FALSE(DownloadServiceFactory::GetForBrowserContext(
      browser()->profile())->IsShelfEnabled());
  EXPECT_TRUE(RunFunction(new DownloadsSetShelfEnabledFunction(), "[true]"));
  EXPECT_TRUE(DownloadServiceFactory::GetForBrowserContext(
      browser()->profile())->IsShelfEnabled());
  // TODO(benjhayden) Test that existing shelves are hidden.
  // TODO(benjhayden) Test multiple extensions.
  // TODO(benjhayden) Test disabling extensions.
  // TODO(benjhayden) Test that browsers associated with other profiles are not
  // affected.
  // TODO(benjhayden) Test incognito.
}

// TODO(benjhayden) Figure out why DisableExtension() does not fire
// OnListenerRemoved.

// TODO(benjhayden) Test that the shelf is shown for download() both with and
// without a WebContents.

void OnDangerPromptCreated(DownloadDangerPrompt* prompt) {
  prompt->InvokeActionForTesting(DownloadDangerPrompt::ACCEPT);
}

#if defined(OS_MACOSX)
// Flakily triggers and assert on Mac.
// http://crbug.com/180759
#define MAYBE_DownloadExtensionTest_AcceptDanger DownloadExtensionTest_AcceptDanger
#else
#define MAYBE_DownloadExtensionTest_AcceptDanger DISABLED_DownloadExtensionTest_AcceptDanger
#endif
IN_PROC_BROWSER_TEST_F(DownloadExtensionTest,
                       MAYBE_DownloadExtensionTest_AcceptDanger) {
  // Download a file that will be marked dangerous; click the browser action
  // button; the browser action poup will call acceptDanger(); when the
  // DownloadDangerPrompt is created, pretend that the user clicks the Accept
  // button; wait until the download completes.
  LoadExtension("downloads_split");
  scoped_ptr<base::Value> result(RunFunctionAndReturnResult(
      new DownloadsDownloadFunction(),
      "[{\"url\": \"data:,\", \"filename\": \"dangerous.swf\"}]"));
  ASSERT_TRUE(result.get());
  int result_id = -1;
  ASSERT_TRUE(result->GetAsInteger(&result_id));
  DownloadItem* item = GetCurrentManager()->GetDownload(result_id);
  ASSERT_TRUE(item);
  ASSERT_TRUE(WaitFor(downloads::OnChanged::kEventName,
                      base::StringPrintf(
                          "[{\"id\": %d, "
                          "  \"danger\": {"
                          "    \"previous\": \"safe\","
                          "    \"current\": \"file\"}}]",
                          result_id)));
  ASSERT_TRUE(item->IsDangerous());
  ScopedCancellingItem canceller(item);
  scoped_ptr<content::DownloadTestObserver> observer(
      new content::DownloadTestObserverTerminal(
          GetCurrentManager(), 1,
          content::DownloadTestObserver::ON_DANGEROUS_DOWNLOAD_IGNORE));
  DownloadsAcceptDangerFunction::OnPromptCreatedCallback callback =
      base::Bind(&OnDangerPromptCreated);
  DownloadsAcceptDangerFunction::OnPromptCreatedForTesting(
      &callback);
  BrowserActionTestUtil(browser()).Press(0);
  observer->WaitForFinished();
}

class DownloadsApiTest : public ExtensionApiTest {
 public:
  DownloadsApiTest() {}
  virtual ~DownloadsApiTest() {}
 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadsApiTest);
};


IN_PROC_BROWSER_TEST_F(DownloadsApiTest, DownloadsApiTest) {
  ASSERT_TRUE(RunExtensionTest("downloads")) << message_;
}

TEST(DownloadInterruptReasonEnumsSynced,
     DownloadInterruptReasonEnumsSynced) {
#define INTERRUPT_REASON(name, value)                                        \
  EXPECT_EQ(InterruptReasonContentToExtension(                               \
                content::DOWNLOAD_INTERRUPT_REASON_##name),                  \
            downloads::INTERRUPT_REASON_##name);                             \
  EXPECT_EQ(                                                                 \
      InterruptReasonExtensionToContent(downloads::INTERRUPT_REASON_##name), \
      content::DOWNLOAD_INTERRUPT_REASON_##name);
#include "content/public/browser/download_interrupt_reason_values.h"
#undef INTERRUPT_REASON
}

TEST(ExtensionDetermineDownloadFilenameInternal,
     ExtensionDetermineDownloadFilenameInternal) {
  std::string winner_id;
  base::FilePath filename;
  downloads::FilenameConflictAction conflict_action =
      downloads::FILENAME_CONFLICT_ACTION_UNIQUIFY;
  ExtensionWarningSet warnings;

  // Empty incumbent determiner
  warnings.clear();
  ExtensionDownloadsEventRouter::DetermineFilenameInternal(
      base::FilePath(FILE_PATH_LITERAL("a")),
      downloads::FILENAME_CONFLICT_ACTION_OVERWRITE,
      "suggester",
      base::Time::Now(),
      "",
      base::Time(),
      &winner_id,
      &filename,
      &conflict_action,
      &warnings);
  EXPECT_EQ("suggester", winner_id);
  EXPECT_EQ(FILE_PATH_LITERAL("a"), filename.value());
  EXPECT_EQ(downloads::FILENAME_CONFLICT_ACTION_OVERWRITE, conflict_action);
  EXPECT_TRUE(warnings.empty());

  // Incumbent wins
  warnings.clear();
  ExtensionDownloadsEventRouter::DetermineFilenameInternal(
      base::FilePath(FILE_PATH_LITERAL("b")),
      downloads::FILENAME_CONFLICT_ACTION_PROMPT,
      "suggester",
      base::Time::Now() - base::TimeDelta::FromDays(1),
      "incumbent",
      base::Time::Now(),
      &winner_id,
      &filename,
      &conflict_action,
      &warnings);
  EXPECT_EQ("incumbent", winner_id);
  EXPECT_EQ(FILE_PATH_LITERAL("a"), filename.value());
  EXPECT_EQ(downloads::FILENAME_CONFLICT_ACTION_OVERWRITE, conflict_action);
  EXPECT_FALSE(warnings.empty());
  EXPECT_EQ(ExtensionWarning::kDownloadFilenameConflict,
            warnings.begin()->warning_type());
  EXPECT_EQ("suggester", warnings.begin()->extension_id());

  // Suggester wins
  warnings.clear();
  ExtensionDownloadsEventRouter::DetermineFilenameInternal(
      base::FilePath(FILE_PATH_LITERAL("b")),
      downloads::FILENAME_CONFLICT_ACTION_PROMPT,
      "suggester",
      base::Time::Now(),
      "incumbent",
      base::Time::Now() - base::TimeDelta::FromDays(1),
      &winner_id,
      &filename,
      &conflict_action,
      &warnings);
  EXPECT_EQ("suggester", winner_id);
  EXPECT_EQ(FILE_PATH_LITERAL("b"), filename.value());
  EXPECT_EQ(downloads::FILENAME_CONFLICT_ACTION_PROMPT, conflict_action);
  EXPECT_FALSE(warnings.empty());
  EXPECT_EQ(ExtensionWarning::kDownloadFilenameConflict,
            warnings.begin()->warning_type());
  EXPECT_EQ("incumbent", warnings.begin()->extension_id());
}

}  // namespace extensions

#endif  // http://crbug.com/3061144
