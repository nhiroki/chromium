// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_DOWNLOADS_DOWNLOADS_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_DOWNLOADS_DOWNLOADS_API_H_

#include <set>
#include <string>

#include "base/files/file_path.h"
#include "base/scoped_observer.h"
#include "base/time/time.h"
#include "chrome/browser/download/all_download_item_notifier.h"
#include "chrome/browser/download/download_danger_prompt.h"
#include "chrome/browser/download/download_path_reservation_tracker.h"
#include "chrome/browser/extensions/chrome_extension_function.h"
#include "chrome/common/extensions/api/downloads.h"
#include "content/public/browser/download_manager.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/browser/warning_set.h"

class DownloadFileIconExtractor;
class DownloadQuery;

namespace content {
class ResourceContext;
class ResourceDispatcherHost;
}

namespace extensions {
class ExtensionRegistry;
}

// Functions in the chrome.downloads namespace facilitate
// controlling downloads from extensions. See the full API doc at
// http://goo.gl/6hO1n

namespace download_extension_errors {

// Errors that can be returned through chrome.runtime.lastError.message.
extern const char kEmptyFile[];
extern const char kFileAlreadyDeleted[];
extern const char kFileNotRemoved[];
extern const char kIconNotFound[];
extern const char kInvalidDangerType[];
extern const char kInvalidFilename[];
extern const char kInvalidFilter[];
extern const char kInvalidHeaderName[];
extern const char kInvalidHeaderValue[];
extern const char kInvalidHeaderUnsafe[];
extern const char kInvalidId[];
extern const char kInvalidOrderBy[];
extern const char kInvalidQueryLimit[];
extern const char kInvalidState[];
extern const char kInvalidURL[];
extern const char kInvisibleContext[];
extern const char kNotComplete[];
extern const char kNotDangerous[];
extern const char kNotInProgress[];
extern const char kNotResumable[];
extern const char kOpenPermission[];
extern const char kShelfDisabled[];
extern const char kShelfPermission[];
extern const char kTooManyListeners[];
extern const char kUnexpectedDeterminer[];
extern const char kUserGesture[];

}  // namespace download_extension_errors

namespace extensions {

class DownloadedByExtension : public base::SupportsUserData::Data {
 public:
  static DownloadedByExtension* Get(content::DownloadItem* item);

  DownloadedByExtension(content::DownloadItem* item,
                        const std::string& id,
                        const std::string& name);

  const std::string& id() const { return id_; }
  const std::string& name() const { return name_; }

 private:
  static const char kKey[];

  std::string id_;
  std::string name_;

  DISALLOW_COPY_AND_ASSIGN(DownloadedByExtension);
};

class DownloadsDownloadFunction : public ChromeAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("downloads.download", DOWNLOADS_DOWNLOAD)
  DownloadsDownloadFunction();
  bool RunAsync() override;

 protected:
  ~DownloadsDownloadFunction() override;

 private:
  void OnStarted(const base::FilePath& creator_suggested_filename,
                 extensions::api::downloads::FilenameConflictAction
                     creator_conflict_action,
                 content::DownloadItem* item,
                 content::DownloadInterruptReason interrupt_reason);

  DISALLOW_COPY_AND_ASSIGN(DownloadsDownloadFunction);
};

class DownloadsSearchFunction : public ChromeSyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("downloads.search", DOWNLOADS_SEARCH)
  DownloadsSearchFunction();
  bool RunSync() override;

 protected:
  ~DownloadsSearchFunction() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadsSearchFunction);
};

class DownloadsPauseFunction : public ChromeSyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("downloads.pause", DOWNLOADS_PAUSE)
  DownloadsPauseFunction();
  bool RunSync() override;

 protected:
  ~DownloadsPauseFunction() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadsPauseFunction);
};

class DownloadsResumeFunction : public ChromeSyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("downloads.resume", DOWNLOADS_RESUME)
  DownloadsResumeFunction();
  bool RunSync() override;

 protected:
  ~DownloadsResumeFunction() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadsResumeFunction);
};

class DownloadsCancelFunction : public ChromeSyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("downloads.cancel", DOWNLOADS_CANCEL)
  DownloadsCancelFunction();
  bool RunSync() override;

 protected:
  ~DownloadsCancelFunction() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadsCancelFunction);
};

class DownloadsEraseFunction : public ChromeSyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("downloads.erase", DOWNLOADS_ERASE)
  DownloadsEraseFunction();
  bool RunSync() override;

 protected:
  ~DownloadsEraseFunction() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadsEraseFunction);
};

class DownloadsRemoveFileFunction : public ChromeAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("downloads.removeFile", DOWNLOADS_REMOVEFILE)
  DownloadsRemoveFileFunction();
  bool RunAsync() override;

 protected:
  ~DownloadsRemoveFileFunction() override;

 private:
  void Done(bool success);

  DISALLOW_COPY_AND_ASSIGN(DownloadsRemoveFileFunction);
};

class DownloadsAcceptDangerFunction : public ChromeAsyncExtensionFunction {
 public:
  typedef base::Callback<void(DownloadDangerPrompt*)> OnPromptCreatedCallback;
  static void OnPromptCreatedForTesting(
      OnPromptCreatedCallback* callback) {
    on_prompt_created_ = callback;
  }

  DECLARE_EXTENSION_FUNCTION("downloads.acceptDanger", DOWNLOADS_ACCEPTDANGER)
  DownloadsAcceptDangerFunction();
  bool RunAsync() override;

 protected:
  ~DownloadsAcceptDangerFunction() override;
  void DangerPromptCallback(int download_id,
                            DownloadDangerPrompt::Action action);

 private:
  void PromptOrWait(int download_id, int retries);

  static OnPromptCreatedCallback* on_prompt_created_;
  DISALLOW_COPY_AND_ASSIGN(DownloadsAcceptDangerFunction);
};

class DownloadsShowFunction : public ChromeAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("downloads.show", DOWNLOADS_SHOW)
  DownloadsShowFunction();
  bool RunAsync() override;

 protected:
  ~DownloadsShowFunction() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadsShowFunction);
};

class DownloadsShowDefaultFolderFunction : public ChromeAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION(
      "downloads.showDefaultFolder", DOWNLOADS_SHOWDEFAULTFOLDER)
  DownloadsShowDefaultFolderFunction();
  bool RunAsync() override;

 protected:
  ~DownloadsShowDefaultFolderFunction() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadsShowDefaultFolderFunction);
};

class DownloadsOpenFunction : public ChromeSyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("downloads.open", DOWNLOADS_OPEN)
  DownloadsOpenFunction();
  bool RunSync() override;

 protected:
  ~DownloadsOpenFunction() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadsOpenFunction);
};

class DownloadsSetShelfEnabledFunction : public ChromeSyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("downloads.setShelfEnabled",
                             DOWNLOADS_SETSHELFENABLED)
  DownloadsSetShelfEnabledFunction();
  bool RunSync() override;

 protected:
  ~DownloadsSetShelfEnabledFunction() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadsSetShelfEnabledFunction);
};

class DownloadsDragFunction : public ChromeAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("downloads.drag", DOWNLOADS_DRAG)
  DownloadsDragFunction();
  bool RunAsync() override;

 protected:
  ~DownloadsDragFunction() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadsDragFunction);
};

class DownloadsGetFileIconFunction : public ChromeAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("downloads.getFileIcon", DOWNLOADS_GETFILEICON)
  DownloadsGetFileIconFunction();
  bool RunAsync() override;
  void SetIconExtractorForTesting(DownloadFileIconExtractor* extractor);

 protected:
  ~DownloadsGetFileIconFunction() override;

 private:
  void OnIconURLExtracted(const std::string& url);
  base::FilePath path_;
  scoped_ptr<DownloadFileIconExtractor> icon_extractor_;
  DISALLOW_COPY_AND_ASSIGN(DownloadsGetFileIconFunction);
};

// Observes a single DownloadManager and many DownloadItems and dispatches
// onCreated and onErased events.
class ExtensionDownloadsEventRouter
    : public extensions::EventRouter::Observer,
      public extensions::ExtensionRegistryObserver,
      public AllDownloadItemNotifier::Observer {
 public:
  typedef base::Callback<void(
      const base::FilePath& changed_filename,
      DownloadPathReservationTracker::FilenameConflictAction)>
    FilenameChangedCallback;

  static void SetDetermineFilenameTimeoutSecondsForTesting(int s);

  // The logic for how to handle conflicting filename suggestions from multiple
  // extensions is split out here for testing.
  static void DetermineFilenameInternal(
      const base::FilePath& filename,
      extensions::api::downloads::FilenameConflictAction conflict_action,
      const std::string& suggesting_extension_id,
      const base::Time& suggesting_install_time,
      const std::string& incumbent_extension_id,
      const base::Time& incumbent_install_time,
      std::string* winner_extension_id,
      base::FilePath* determined_filename,
      extensions::api::downloads::FilenameConflictAction*
        determined_conflict_action,
      extensions::WarningSet* warnings);

  // A downloads.onDeterminingFilename listener has returned. If the extension
  // wishes to override the download's filename, then |filename| will be
  // non-empty. |filename| will be interpreted as a relative path, appended to
  // the default downloads directory. If the extension wishes to overwrite any
  // existing files, then |overwrite| will be true. Returns true on success,
  // false otherwise.
  static bool DetermineFilename(
      Profile* profile,
      bool include_incognito,
      const std::string& ext_id,
      int download_id,
      const base::FilePath& filename,
      extensions::api::downloads::FilenameConflictAction conflict_action,
      std::string* error);

  explicit ExtensionDownloadsEventRouter(
      Profile* profile, content::DownloadManager* manager);
  ~ExtensionDownloadsEventRouter() override;

  void SetShelfEnabled(const extensions::Extension* extension, bool enabled);
  bool IsShelfEnabled() const;

  // Called by ChromeDownloadManagerDelegate during the filename determination
  // process, allows extensions to change the item's target filename. If no
  // extension wants to change the target filename, then |no_change| will be
  // called and the filename determination process will continue as normal. If
  // an extension wants to change the target filename, then |change| will be
  // called with the new filename and a flag indicating whether the new file
  // should overwrite any old files of the same name.
  void OnDeterminingFilename(
      content::DownloadItem* item,
      const base::FilePath& suggested_path,
      const base::Closure& no_change,
      const FilenameChangedCallback& change);

  // AllDownloadItemNotifier::Observer.
  void OnDownloadCreated(content::DownloadManager* manager,
                         content::DownloadItem* download_item) override;
  void OnDownloadUpdated(content::DownloadManager* manager,
                         content::DownloadItem* download_item) override;
  void OnDownloadRemoved(content::DownloadManager* manager,
                         content::DownloadItem* download_item) override;

  // extensions::EventRouter::Observer.
  void OnListenerRemoved(const extensions::EventListenerInfo& details) override;

  // Used for testing.
  struct DownloadsNotificationSource {
    std::string event_name;
    Profile* profile;
  };

  void CheckForHistoryFilesRemoval();

 private:
  void DispatchEvent(
      events::HistogramValue histogram_value,
      const std::string& event_name,
      bool include_incognito,
      const extensions::Event::WillDispatchCallback& will_dispatch_callback,
      base::Value* json_arg);

  // extensions::ExtensionRegistryObserver.
  void OnExtensionUnloaded(
      content::BrowserContext* browser_context,
      const extensions::Extension* extension,
      extensions::UnloadedExtensionInfo::Reason reason) override;

  Profile* profile_;
  AllDownloadItemNotifier notifier_;
  std::set<const extensions::Extension*> shelf_disabling_extensions_;

  base::Time last_checked_removal_;

  // Listen to extension unloaded notifications.
  ScopedObserver<extensions::ExtensionRegistry,
                 extensions::ExtensionRegistryObserver>
      extension_registry_observer_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionDownloadsEventRouter);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_DOWNLOADS_DOWNLOADS_API_H_
