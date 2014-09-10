// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <stack>

#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/thread_task_runner_handle.h"
#include "chrome/browser/drive/drive_uploader.h"
#include "chrome/browser/drive/fake_drive_service.h"
#include "chrome/browser/drive/test_util.h"
#include "chrome/browser/sync_file_system/drive_backend/callback_helper.h"
#include "chrome/browser/sync_file_system/drive_backend/drive_backend_constants.h"
#include "chrome/browser/sync_file_system/drive_backend/fake_drive_service_helper.h"
#include "chrome/browser/sync_file_system/drive_backend/metadata_database.h"
#include "chrome/browser/sync_file_system/drive_backend/metadata_database.pb.h"
#include "chrome/browser/sync_file_system/drive_backend/sync_engine.h"
#include "chrome/browser/sync_file_system/drive_backend/sync_engine_context.h"
#include "chrome/browser/sync_file_system/drive_backend/sync_worker.h"
#include "chrome/browser/sync_file_system/local/canned_syncable_file_system.h"
#include "chrome/browser/sync_file_system/local/local_file_sync_context.h"
#include "chrome/browser/sync_file_system/local/local_file_sync_service.h"
#include "chrome/browser/sync_file_system/local/sync_file_system_backend.h"
#include "chrome/browser/sync_file_system/sync_file_system_test_util.h"
#include "chrome/browser/sync_file_system/syncable_file_system_util.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/common/extension.h"
#include "google_apis/drive/drive_api_parser.h"
#include "net/url_request/url_request_context_getter.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/leveldatabase/src/helpers/memenv/memenv.h"
#include "third_party/leveldatabase/src/include/leveldb/env.h"

#define FPL(a) FILE_PATH_LITERAL(a)

namespace sync_file_system {
namespace drive_backend {

typedef storage::FileSystemOperation::FileEntryList FileEntryList;

namespace {

template <typename T>
void SetValueAndCallClosure(const base::Closure& closure,
                            T* arg_out,
                            T arg) {
  *arg_out = base::internal::CallbackForward(arg);
  closure.Run();
}

void SetSyncStatusAndUrl(const base::Closure& closure,
                         SyncStatusCode* status_out,
                         storage::FileSystemURL* url_out,
                         SyncStatusCode status,
                         const storage::FileSystemURL& url) {
  *status_out = status;
  *url_out = url;
  closure.Run();
}

}  // namespace

class DriveBackendSyncTest : public testing::Test,
                             public LocalFileSyncService::Observer,
                             public RemoteFileSyncService::Observer {
 public:
  DriveBackendSyncTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        pending_remote_changes_(0),
        pending_local_changes_(0) {}
  virtual ~DriveBackendSyncTest() {}

  virtual void SetUp() OVERRIDE {
    ASSERT_TRUE(base_dir_.CreateUniqueTempDir());
    in_memory_env_.reset(leveldb::NewMemEnv(leveldb::Env::Default()));

    io_task_runner_ = content::BrowserThread::GetMessageLoopProxyForThread(
        content::BrowserThread::IO);
    scoped_refptr<base::SequencedWorkerPool> worker_pool(
        content::BrowserThread::GetBlockingPool());
    worker_task_runner_ =
        worker_pool->GetSequencedTaskRunnerWithShutdownBehavior(
            worker_pool->GetSequenceToken(),
            base::SequencedWorkerPool::SKIP_ON_SHUTDOWN);
    file_task_runner_ = content::BrowserThread::GetMessageLoopProxyForThread(
        content::BrowserThread::FILE);
    scoped_refptr<base::SequencedTaskRunner> drive_task_runner =
        worker_pool->GetSequencedTaskRunnerWithShutdownBehavior(
            worker_pool->GetSequenceToken(),
            base::SequencedWorkerPool::SKIP_ON_SHUTDOWN);

    RegisterSyncableFileSystem();
    local_sync_service_ = LocalFileSyncService::CreateForTesting(
        &profile_, in_memory_env_.get());
    local_sync_service_->AddChangeObserver(this);

    scoped_ptr<drive::FakeDriveService>
        drive_service(new drive::FakeDriveService);
    drive_service->Initialize("test@example.com");
    ASSERT_TRUE(drive::test_util::SetUpTestEntries(drive_service.get()));

    scoped_ptr<drive::DriveUploaderInterface> uploader(
        new drive::DriveUploader(drive_service.get(),
                                 file_task_runner_.get()));

    fake_drive_service_helper_.reset(new FakeDriveServiceHelper(
        drive_service.get(), uploader.get(),
        kSyncRootFolderTitle));

    remote_sync_service_.reset(
        new SyncEngine(base::ThreadTaskRunnerHandle::Get(),  // ui_task_runner
                       worker_task_runner_.get(),
                       drive_task_runner.get(),
                       base_dir_.path(),
                       NULL,  // task_logger
                       NULL,  // notification_manager
                       NULL,  // extension_service
                       NULL,  // signin_manager
                       NULL,  // token_service
                       NULL,  // request_context
                       scoped_ptr<SyncEngine::DriveServiceFactory>(),
                       in_memory_env_.get()));
    remote_sync_service_->AddServiceObserver(this);
    remote_sync_service_->InitializeForTesting(
        drive_service.PassAs<drive::DriveServiceInterface>(),
        uploader.Pass(),
        scoped_ptr<SyncWorkerInterface>());
    remote_sync_service_->SetSyncEnabled(true);

    local_sync_service_->SetLocalChangeProcessor(remote_sync_service_.get());
    remote_sync_service_->SetRemoteChangeProcessor(local_sync_service_.get());
  }

  virtual void TearDown() OVERRIDE {
    typedef std::map<std::string, CannedSyncableFileSystem*>::iterator iterator;
    for (iterator itr = file_systems_.begin();
         itr != file_systems_.end(); ++itr) {
      itr->second->TearDown();
      delete itr->second;
    }
    file_systems_.clear();

    local_sync_service_->Shutdown();

    fake_drive_service_helper_.reset();
    local_sync_service_.reset();
    remote_sync_service_.reset();

    content::BrowserThread::GetBlockingPool()->FlushForTesting();
    base::RunLoop().RunUntilIdle();
    RevokeSyncableFileSystem();
  }

  virtual void OnRemoteChangeQueueUpdated(int64 pending_changes_hint) OVERRIDE {
    pending_remote_changes_ = pending_changes_hint;
  }

  virtual void OnLocalChangeAvailable(int64 pending_changes_hint) OVERRIDE {
    pending_local_changes_ = pending_changes_hint;
  }

 protected:
  storage::FileSystemURL CreateURL(const std::string& app_id,
                                   const base::FilePath::StringType& path) {
    return CreateURL(app_id, base::FilePath(path));
  }

  storage::FileSystemURL CreateURL(const std::string& app_id,
                                   const base::FilePath& path) {
    GURL origin = extensions::Extension::GetBaseURLFromExtensionId(app_id);
    return CreateSyncableFileSystemURL(origin, path);
  }

  bool GetAppRootFolderID(const std::string& app_id,
                          std::string* folder_id) {
    base::RunLoop run_loop;
    bool success = false;
    FileTracker tracker;
    PostTaskAndReplyWithResult(
        worker_task_runner_.get(),
        FROM_HERE,
        base::Bind(&MetadataDatabase::FindAppRootTracker,
                   base::Unretained(metadata_database()),
                   app_id,
                   &tracker),
        base::Bind(
            &SetValueAndCallClosure<bool>, run_loop.QuitClosure(), &success));
    run_loop.Run();
    if (!success)
      return false;
    *folder_id = tracker.file_id();
    return true;
  }

  std::string GetFileIDByPath(const std::string& app_id,
                              const base::FilePath::StringType& path) {
    return GetFileIDByPath(app_id, base::FilePath(path));
  }

  std::string GetFileIDByPath(const std::string& app_id,
                              const base::FilePath& path) {
    base::RunLoop run_loop;
    bool success = false;
    FileTracker tracker;
    base::FilePath result_path;
    base::FilePath normalized_path = path.NormalizePathSeparators();
    PostTaskAndReplyWithResult(
        worker_task_runner_.get(),
        FROM_HERE,
        base::Bind(&MetadataDatabase::FindNearestActiveAncestor,
                   base::Unretained(metadata_database()),
                   app_id,
                   normalized_path,
                   &tracker,
                   &result_path),
        base::Bind(
            &SetValueAndCallClosure<bool>, run_loop.QuitClosure(), &success));
    run_loop.Run();
    EXPECT_TRUE(success);
    EXPECT_EQ(normalized_path, result_path);
    return tracker.file_id();
  }

  SyncStatusCode RegisterApp(const std::string& app_id) {
    GURL origin = extensions::Extension::GetBaseURLFromExtensionId(app_id);
    if (!ContainsKey(file_systems_, app_id)) {
      CannedSyncableFileSystem* file_system = new CannedSyncableFileSystem(
          origin, in_memory_env_.get(),
          io_task_runner_.get(), file_task_runner_.get());
      file_system->SetUp(CannedSyncableFileSystem::QUOTA_DISABLED);

      SyncStatusCode status = SYNC_STATUS_UNKNOWN;
      base::RunLoop run_loop;
      local_sync_service_->MaybeInitializeFileSystemContext(
          origin, file_system->file_system_context(),
          base::Bind(&SetValueAndCallClosure<SyncStatusCode>,
                     run_loop.QuitClosure(), &status));
      run_loop.Run();
      EXPECT_EQ(SYNC_STATUS_OK, status);

      file_system->backend()->sync_context()->
          set_mock_notify_changes_duration_in_sec(0);

      EXPECT_EQ(base::File::FILE_OK, file_system->OpenFileSystem());
      file_systems_[app_id] = file_system;
    }

    SyncStatusCode status = SYNC_STATUS_UNKNOWN;
    base::RunLoop run_loop;
    remote_sync_service_->RegisterOrigin(
        origin,
        base::Bind(&SetValueAndCallClosure<SyncStatusCode>,
                   run_loop.QuitClosure(), &status));
    run_loop.Run();
    return status;
  }

  void AddLocalFolder(const std::string& app_id,
                      const base::FilePath::StringType& path) {
    ASSERT_TRUE(ContainsKey(file_systems_, app_id));
    EXPECT_EQ(base::File::FILE_OK,
              file_systems_[app_id]->CreateDirectory(
                  CreateURL(app_id, path)));
  }

  void AddOrUpdateLocalFile(const std::string& app_id,
                            const base::FilePath::StringType& path,
                            const std::string& content) {
    storage::FileSystemURL url(CreateURL(app_id, path));
    ASSERT_TRUE(ContainsKey(file_systems_, app_id));
    EXPECT_EQ(base::File::FILE_OK, file_systems_[app_id]->CreateFile(url));
    int64 bytes_written = file_systems_[app_id]->WriteString(url, content);
    EXPECT_EQ(static_cast<int64>(content.size()), bytes_written);
    base::RunLoop().RunUntilIdle();
  }

  void UpdateLocalFile(const std::string& app_id,
                       const base::FilePath::StringType& path,
                       const std::string& content) {
    ASSERT_TRUE(ContainsKey(file_systems_, app_id));
    int64 bytes_written = file_systems_[app_id]->WriteString(
        CreateURL(app_id, path), content);
    EXPECT_EQ(static_cast<int64>(content.size()), bytes_written);
    base::RunLoop().RunUntilIdle();
  }

  void RemoveLocal(const std::string& app_id,
                   const base::FilePath::StringType& path) {
    ASSERT_TRUE(ContainsKey(file_systems_, app_id));
    EXPECT_EQ(base::File::FILE_OK,
              file_systems_[app_id]->Remove(
                  CreateURL(app_id, path),
                  true /* recursive */));
    base::RunLoop().RunUntilIdle();
  }

  SyncStatusCode ProcessLocalChange() {
    SyncStatusCode status = SYNC_STATUS_UNKNOWN;
    storage::FileSystemURL url;
    base::RunLoop run_loop;
    local_sync_service_->ProcessLocalChange(base::Bind(
        &SetSyncStatusAndUrl, run_loop.QuitClosure(), &status, &url));
    run_loop.Run();
    return status;
  }

  SyncStatusCode ProcessRemoteChange() {
    SyncStatusCode status = SYNC_STATUS_UNKNOWN;
    storage::FileSystemURL url;
    base::RunLoop run_loop;
    remote_sync_service_->ProcessRemoteChange(base::Bind(
        &SetSyncStatusAndUrl, run_loop.QuitClosure(), &status, &url));
    run_loop.Run();
    return status;
  }

  int64 GetLargestChangeID() {
    scoped_ptr<google_apis::AboutResource> about_resource;
    EXPECT_EQ(google_apis::HTTP_SUCCESS,
              fake_drive_service_helper()->GetAboutResource(&about_resource));
    if (!about_resource)
      return 0;
    return about_resource->largest_change_id();
  }

  void FetchRemoteChanges() {
    remote_sync_service_->OnNotificationReceived();
    WaitForIdleWorker();
  }

  SyncStatusCode ProcessChangesUntilDone() {
    int task_limit = 100;
    SyncStatusCode local_sync_status;
    SyncStatusCode remote_sync_status;
    while (true) {
      base::RunLoop().RunUntilIdle();
      WaitForIdleWorker();

      if (!task_limit--)
        return SYNC_STATUS_ABORT;

      local_sync_status = ProcessLocalChange();
      if (local_sync_status != SYNC_STATUS_OK &&
          local_sync_status != SYNC_STATUS_NO_CHANGE_TO_SYNC &&
          local_sync_status != SYNC_STATUS_FILE_BUSY)
        return local_sync_status;

      remote_sync_status = ProcessRemoteChange();
      if (remote_sync_status != SYNC_STATUS_OK &&
          remote_sync_status != SYNC_STATUS_NO_CHANGE_TO_SYNC &&
          remote_sync_status != SYNC_STATUS_FILE_BUSY)
        return remote_sync_status;

      if (local_sync_status == SYNC_STATUS_NO_CHANGE_TO_SYNC &&
          remote_sync_status == SYNC_STATUS_NO_CHANGE_TO_SYNC) {

        {
          base::RunLoop run_loop;
          remote_sync_service_->PromoteDemotedChanges(run_loop.QuitClosure());
          run_loop.Run();
        }

        {
          base::RunLoop run_loop;
          local_sync_service_->PromoteDemotedChanges(run_loop.QuitClosure());
          run_loop.Run();
        }

        if (pending_remote_changes_ || pending_local_changes_)
          continue;

        base::RunLoop run_loop;
        int64 largest_fetched_change_id = -1;
        PostTaskAndReplyWithResult(
            worker_task_runner_.get(),
            FROM_HERE,
            base::Bind(&MetadataDatabase::GetLargestFetchedChangeID,
                       base::Unretained(metadata_database())),
            base::Bind(&SetValueAndCallClosure<int64>,
                       run_loop.QuitClosure(),
                       &largest_fetched_change_id));
        run_loop.Run();
        if (largest_fetched_change_id != GetLargestChangeID()) {
          FetchRemoteChanges();
          continue;
        }
        break;
      }
    }
    return SYNC_STATUS_OK;
  }

  // Verifies local and remote files/folders are consistent.
  // This function checks:
  //  - Each registered origin has corresponding remote folder.
  //  - Each local file/folder has corresponding remote one.
  //  - Each remote file/folder has corresponding local one.
  // TODO(tzik): Handle conflict case. i.e. allow remote file has different
  // file content if the corresponding local file conflicts to it.
  void VerifyConsistency() {
    std::string sync_root_folder_id;
    google_apis::GDataErrorCode error =
        fake_drive_service_helper_->GetSyncRootFolderID(&sync_root_folder_id);
    if (sync_root_folder_id.empty()) {
      EXPECT_EQ(google_apis::HTTP_NOT_FOUND, error);
      EXPECT_TRUE(file_systems_.empty());
      return;
    }
    EXPECT_EQ(google_apis::HTTP_SUCCESS, error);

    ScopedVector<google_apis::ResourceEntry> remote_entries;
    EXPECT_EQ(google_apis::HTTP_SUCCESS,
              fake_drive_service_helper_->ListFilesInFolder(
                  sync_root_folder_id, &remote_entries));
    std::map<std::string, const google_apis::ResourceEntry*> app_root_by_title;
    for (ScopedVector<google_apis::ResourceEntry>::iterator itr =
             remote_entries.begin();
         itr != remote_entries.end();
         ++itr) {
      const google_apis::ResourceEntry& remote_entry = **itr;
      EXPECT_FALSE(ContainsKey(app_root_by_title, remote_entry.title()));
      app_root_by_title[remote_entry.title()] = *itr;
    }

    for (std::map<std::string, CannedSyncableFileSystem*>::const_iterator itr =
             file_systems_.begin();
         itr != file_systems_.end(); ++itr) {
      const std::string& app_id = itr->first;
      SCOPED_TRACE(testing::Message() << "Verifying app: " << app_id);
      CannedSyncableFileSystem* file_system = itr->second;
      ASSERT_TRUE(ContainsKey(app_root_by_title, app_id));
      VerifyConsistencyForFolder(
          app_id, base::FilePath(),
          app_root_by_title[app_id]->resource_id(),
          file_system);
    }
  }

  void VerifyConsistencyForFolder(const std::string& app_id,
                                  const base::FilePath& path,
                                  const std::string& folder_id,
                                  CannedSyncableFileSystem* file_system) {
    SCOPED_TRACE(testing::Message() << "Verifying folder: " << path.value());

    ScopedVector<google_apis::ResourceEntry> remote_entries;
    EXPECT_EQ(google_apis::HTTP_SUCCESS,
              fake_drive_service_helper_->ListFilesInFolder(
                  folder_id, &remote_entries));
    std::map<std::string, const google_apis::ResourceEntry*>
        remote_entry_by_title;
    for (size_t i = 0; i < remote_entries.size(); ++i) {
      google_apis::ResourceEntry* remote_entry = remote_entries[i];
      EXPECT_FALSE(ContainsKey(remote_entry_by_title, remote_entry->title()))
          << "title: " << remote_entry->title();
      remote_entry_by_title[remote_entry->title()] = remote_entry;
    }

    storage::FileSystemURL url(CreateURL(app_id, path));
    FileEntryList local_entries;
    EXPECT_EQ(base::File::FILE_OK,
              file_system->ReadDirectory(url, &local_entries));
    for (FileEntryList::iterator itr = local_entries.begin();
         itr != local_entries.end();
         ++itr) {
      const storage::DirectoryEntry& local_entry = *itr;
      storage::FileSystemURL entry_url(
          CreateURL(app_id, path.Append(local_entry.name)));
      std::string title =
          storage::VirtualPath::BaseName(entry_url.path()).AsUTF8Unsafe();
      SCOPED_TRACE(testing::Message() << "Verifying entry: " << title);

      ASSERT_TRUE(ContainsKey(remote_entry_by_title, title));
      const google_apis::ResourceEntry& remote_entry =
          *remote_entry_by_title[title];
      if (local_entry.is_directory) {
        ASSERT_TRUE(remote_entry.is_folder());
        VerifyConsistencyForFolder(app_id, entry_url.path(),
                                   remote_entry.resource_id(),
                                   file_system);
      } else {
        ASSERT_TRUE(remote_entry.is_file());
        VerifyConsistencyForFile(app_id, entry_url.path(),
                                 remote_entry.resource_id(),
                                 file_system);
      }
      remote_entry_by_title.erase(title);
    }

    EXPECT_TRUE(remote_entry_by_title.empty());
  }

  void VerifyConsistencyForFile(const std::string& app_id,
                                const base::FilePath& path,
                                const std::string& file_id,
                                CannedSyncableFileSystem* file_system) {
    storage::FileSystemURL url(CreateURL(app_id, path));
    std::string file_content;
    EXPECT_EQ(google_apis::HTTP_SUCCESS,
              fake_drive_service_helper_->ReadFile(file_id, &file_content));
    EXPECT_EQ(base::File::FILE_OK,
              file_system->VerifyFile(url, file_content));
  }

  size_t CountApp() {
    return file_systems_.size();
  }

  size_t CountLocalFile(const std::string& app_id) {
    if (!ContainsKey(file_systems_, app_id))
      return 0;

    CannedSyncableFileSystem* file_system = file_systems_[app_id];
    std::stack<base::FilePath> folders;
    folders.push(base::FilePath());  // root folder

    size_t result = 1;
    while (!folders.empty()) {
      storage::FileSystemURL url(CreateURL(app_id, folders.top()));
      folders.pop();

      FileEntryList entries;
      EXPECT_EQ(base::File::FILE_OK,
                file_system->ReadDirectory(url, &entries));
      for (FileEntryList::iterator itr = entries.begin();
           itr != entries.end(); ++itr) {
        ++result;
        if (itr->is_directory)
          folders.push(url.path().Append(itr->name));
      }
    }

    return result;
  }

  void VerifyLocalFile(const std::string& app_id,
                       const base::FilePath::StringType& path,
                       const std::string& content) {
    SCOPED_TRACE(testing::Message() << "Verifying local file: "
                                    << "app_id = " << app_id
                                    << ", path = " << path);
    ASSERT_TRUE(ContainsKey(file_systems_, app_id));
    EXPECT_EQ(base::File::FILE_OK,
              file_systems_[app_id]->VerifyFile(
                  CreateURL(app_id, path), content));
  }

  void VerifyLocalFolder(const std::string& app_id,
                         const base::FilePath::StringType& path) {
    SCOPED_TRACE(testing::Message() << "Verifying local file: "
                                    << "app_id = " << app_id
                                    << ", path = " << path);
    ASSERT_TRUE(ContainsKey(file_systems_, app_id));
    EXPECT_EQ(base::File::FILE_OK,
              file_systems_[app_id]->DirectoryExists(CreateURL(app_id, path)));
  }

  size_t CountMetadata() {
    size_t count = 0;
    base::RunLoop run_loop;
    PostTaskAndReplyWithResult(
        worker_task_runner_.get(),
        FROM_HERE,
        base::Bind(&MetadataDatabase::CountFileMetadata,
                   base::Unretained(metadata_database())),
        base::Bind(
            &SetValueAndCallClosure<size_t>, run_loop.QuitClosure(), &count));
    run_loop.Run();
    return count;
  }

  size_t CountTracker() {
    size_t count = 0;
    base::RunLoop run_loop;
    PostTaskAndReplyWithResult(
        worker_task_runner_.get(),
        FROM_HERE,
        base::Bind(&MetadataDatabase::CountFileTracker,
                   base::Unretained(metadata_database())),
        base::Bind(
            &SetValueAndCallClosure<size_t>, run_loop.QuitClosure(), &count));
    run_loop.Run();
    return count;
  }

  drive::FakeDriveService* fake_drive_service() {
    return static_cast<drive::FakeDriveService*>(
        remote_sync_service_->drive_service_.get());
  }

  FakeDriveServiceHelper* fake_drive_service_helper() {
    return fake_drive_service_helper_.get();
  }

  void WaitForIdleWorker() {
    base::RunLoop run_loop;
    worker_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&SyncWorker::CallOnIdleForTesting,
                   base::Unretained(sync_worker()),
                   RelayCallbackToCurrentThread(
                       FROM_HERE,
                       run_loop.QuitClosure())));
    run_loop.Run();
  }

 private:
  SyncWorker* sync_worker() {
    return static_cast<SyncWorker*>(remote_sync_service_->sync_worker_.get());
  }

  // MetadataDatabase is normally used on the worker thread.
  // Use this only when there is no task running on the worker.
  MetadataDatabase* metadata_database() {
    return sync_worker()->context_->metadata_database_.get();
  }

  content::TestBrowserThreadBundle thread_bundle_;

  base::ScopedTempDir base_dir_;
  scoped_ptr<leveldb::Env> in_memory_env_;
  TestingProfile profile_;

  scoped_ptr<SyncEngine> remote_sync_service_;
  scoped_ptr<LocalFileSyncService> local_sync_service_;

  int64 pending_remote_changes_;
  int64 pending_local_changes_;

  scoped_ptr<FakeDriveServiceHelper> fake_drive_service_helper_;
  std::map<std::string, CannedSyncableFileSystem*> file_systems_;


  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  scoped_refptr<base::SequencedTaskRunner> worker_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> file_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(DriveBackendSyncTest);
};

TEST_F(DriveBackendSyncTest, LocalToRemoteBasicTest) {
  std::string app_id = "example";

  RegisterApp(app_id);
  AddOrUpdateLocalFile(app_id, FPL("file"), "abcde");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(2u, CountLocalFile(app_id));
  VerifyLocalFile(app_id, FPL("file"), "abcde");

  EXPECT_EQ(3u, CountMetadata());
  EXPECT_EQ(3u, CountTracker());
}

TEST_F(DriveBackendSyncTest, RemoteToLocalBasicTest) {
  std::string app_id = "example";
  RegisterApp(app_id);

  std::string app_root_folder_id;
  EXPECT_TRUE(GetAppRootFolderID(app_id, &app_root_folder_id));

  std::string file_id;
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddFile(
                app_root_folder_id, "file", "abcde", &file_id));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(2u, CountLocalFile(app_id));
  VerifyLocalFile(app_id, FPL("file"), "abcde");

  EXPECT_EQ(3u, CountMetadata());
  EXPECT_EQ(3u, CountTracker());
}

TEST_F(DriveBackendSyncTest, LocalFileUpdateTest) {
  std::string app_id = "example";
  const base::FilePath::StringType kPath(FPL("file"));

  RegisterApp(app_id);
  AddOrUpdateLocalFile(app_id, kPath, "abcde");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  UpdateLocalFile(app_id, kPath, "1234567890");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(2u, CountLocalFile(app_id));
  VerifyLocalFile(app_id, FPL("file"), "1234567890");

  EXPECT_EQ(3u, CountMetadata());
  EXPECT_EQ(3u, CountTracker());
}

TEST_F(DriveBackendSyncTest, RemoteFileUpdateTest) {
  std::string app_id = "example";

  RegisterApp(app_id);
  std::string remote_file_id;
  std::string app_root_folder_id;
  EXPECT_TRUE(GetAppRootFolderID(app_id, &app_root_folder_id));
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddFile(
                app_root_folder_id, "file", "abcde", &remote_file_id));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->UpdateFile(
                remote_file_id, "1234567890"));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(2u, CountLocalFile(app_id));
  VerifyLocalFile(app_id, FPL("file"), "1234567890");

  EXPECT_EQ(3u, CountMetadata());
  EXPECT_EQ(3u, CountTracker());
}

TEST_F(DriveBackendSyncTest, LocalFileDeletionTest) {
  std::string app_id = "example";
  const base::FilePath::StringType path(FPL("file"));

  RegisterApp(app_id);
  AddOrUpdateLocalFile(app_id, path, "abcde");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  RemoveLocal(app_id, path);

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(1u, CountLocalFile(app_id));

  EXPECT_EQ(2u, CountMetadata());
  EXPECT_EQ(2u, CountTracker());
}

TEST_F(DriveBackendSyncTest, RemoteFileDeletionTest) {
  std::string app_id = "example";
  const base::FilePath::StringType path(FPL("file"));

  RegisterApp(app_id);
  AddOrUpdateLocalFile(app_id, path, "abcde");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  std::string file_id = GetFileIDByPath(app_id, path);
  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(file_id));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(1u, CountLocalFile(app_id));

  EXPECT_EQ(2u, CountMetadata());
  EXPECT_EQ(2u, CountTracker());
}

TEST_F(DriveBackendSyncTest, RemoteRenameTest) {
  std::string app_id = "example";
  const base::FilePath::StringType path(FPL("file"));

  RegisterApp(app_id);
  AddOrUpdateLocalFile(app_id, path, "abcde");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  std::string file_id = GetFileIDByPath(app_id, path);
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->RenameResource(
                file_id, "renamed_file"));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(2u, CountLocalFile(app_id));
  VerifyLocalFile(app_id, FPL("renamed_file"), "abcde");

  EXPECT_EQ(3u, CountMetadata());
  EXPECT_EQ(3u, CountTracker());
}

TEST_F(DriveBackendSyncTest, RemoteRenameAndRevertTest) {
  std::string app_id = "example";
  const base::FilePath::StringType path(FPL("file"));

  RegisterApp(app_id);
  AddOrUpdateLocalFile(app_id, path, "abcde");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  std::string file_id = GetFileIDByPath(app_id, path);
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->RenameResource(
                file_id, "renamed_file"));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->RenameResource(
                file_id, base::FilePath(path).AsUTF8Unsafe()));

  FetchRemoteChanges();

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(2u, CountLocalFile(app_id));
  VerifyLocalFile(app_id, FPL("file"), "abcde");

  EXPECT_EQ(3u, CountMetadata());
  EXPECT_EQ(3u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ReorganizeToOtherFolder) {
  std::string app_id = "example";
  const base::FilePath::StringType path(FPL("file"));

  RegisterApp(app_id);
  AddLocalFolder(app_id, FPL("folder_src"));
  AddLocalFolder(app_id, FPL("folder_dest"));
  AddOrUpdateLocalFile(app_id, FPL("folder_src/file"), "abcde");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  std::string file_id = GetFileIDByPath(app_id, FPL("folder_src/file"));
  std::string src_folder_id = GetFileIDByPath(app_id, FPL("folder_src"));
  std::string dest_folder_id = GetFileIDByPath(app_id, FPL("folder_dest"));
  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->RemoveResourceFromDirectory(
                src_folder_id, file_id));
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddResourceToDirectory(
                dest_folder_id, file_id));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(4u, CountLocalFile(app_id));
  VerifyLocalFolder(app_id, FPL("folder_dest"));
  VerifyLocalFile(app_id, FPL("folder_dest/file"), "abcde");

  EXPECT_EQ(5u, CountMetadata());
  EXPECT_EQ(5u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ReorganizeToOtherApp) {
  std::string src_app_id = "src_app";
  std::string dest_app_id = "dest_app";

  RegisterApp(src_app_id);
  RegisterApp(dest_app_id);

  AddLocalFolder(src_app_id, FPL("folder_src"));
  AddLocalFolder(dest_app_id, FPL("folder_dest"));
  AddOrUpdateLocalFile(src_app_id, FPL("folder_src/file"), "abcde");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  std::string file_id = GetFileIDByPath(src_app_id, FPL("folder_src/file"));
  std::string src_folder_id = GetFileIDByPath(src_app_id, FPL("folder_src"));
  std::string dest_folder_id = GetFileIDByPath(dest_app_id, FPL("folder_dest"));
  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->RemoveResourceFromDirectory(
                src_folder_id, file_id));
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddResourceToDirectory(
                dest_folder_id, file_id));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(2u, CountApp());
  EXPECT_EQ(2u, CountLocalFile(src_app_id));
  EXPECT_EQ(3u, CountLocalFile(dest_app_id));
  VerifyLocalFile(dest_app_id, FPL("folder_dest/file"), "abcde");

  EXPECT_EQ(6u, CountMetadata());
  EXPECT_EQ(6u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ReorganizeToUnmanagedArea) {
  std::string app_id = "example";

  RegisterApp(app_id);

  AddLocalFolder(app_id, FPL("folder_src"));
  AddOrUpdateLocalFile(app_id, FPL("folder_src/file_orphaned"), "abcde");
  AddOrUpdateLocalFile(app_id, FPL("folder_src/file_under_sync_root"), "123");
  AddOrUpdateLocalFile(app_id, FPL("folder_src/file_under_drive_root"), "hoge");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  std::string file_orphaned_id =
      GetFileIDByPath(app_id, FPL("folder_src/file_orphaned"));
  std::string file_under_sync_root_id =
      GetFileIDByPath(app_id, FPL("folder_src/file_under_sync_root"));
  std::string file_under_drive_root_id =
      GetFileIDByPath(app_id, FPL("folder_src/file_under_drive_root"));

  std::string folder_id = GetFileIDByPath(app_id, FPL("folder_src"));
  std::string sync_root_folder_id;
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->GetSyncRootFolderID(
                &sync_root_folder_id));

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->RemoveResourceFromDirectory(
                folder_id, file_orphaned_id));
  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->RemoveResourceFromDirectory(
                folder_id, file_under_sync_root_id));
  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->RemoveResourceFromDirectory(
                folder_id, file_under_drive_root_id));

  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddResourceToDirectory(
                sync_root_folder_id, file_under_sync_root_id));
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddResourceToDirectory(
                fake_drive_service()->GetRootResourceId(),
                file_under_drive_root_id));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(2u, CountLocalFile(app_id));

  EXPECT_EQ(4u, CountMetadata());
  EXPECT_EQ(4u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ReorganizeToMultipleParents) {
  std::string app_id = "example";

  RegisterApp(app_id);

  AddLocalFolder(app_id, FPL("parent1"));
  AddLocalFolder(app_id, FPL("parent2"));
  AddOrUpdateLocalFile(app_id, FPL("parent1/file"), "abcde");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  std::string file_id = GetFileIDByPath(app_id, FPL("parent1/file"));
  std::string parent2_folder_id = GetFileIDByPath(app_id, FPL("parent2"));
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddResourceToDirectory(
                parent2_folder_id, file_id));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(4u, CountLocalFile(app_id));
  VerifyLocalFolder(app_id, FPL("parent1"));
  VerifyLocalFolder(app_id, FPL("parent2"));
  VerifyLocalFile(app_id, FPL("parent1/file"), "abcde");

  EXPECT_EQ(5u, CountMetadata());
  EXPECT_EQ(5u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ReorganizeAndRevert) {
  std::string app_id = "example";

  RegisterApp(app_id);

  AddLocalFolder(app_id, FPL("folder"));
  AddLocalFolder(app_id, FPL("folder_temp"));
  AddOrUpdateLocalFile(app_id, FPL("folder/file"), "abcde");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  std::string file_id = GetFileIDByPath(app_id, FPL("folder/file"));
  std::string folder_id = GetFileIDByPath(app_id, FPL("folder"));
  std::string folder_temp_id = GetFileIDByPath(app_id, FPL("folder_temp"));
  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->RemoveResourceFromDirectory(
                folder_id, file_id));
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddResourceToDirectory(
                folder_temp_id, file_id));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->RemoveResourceFromDirectory(
                folder_temp_id, file_id));
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddResourceToDirectory(
                folder_id, file_id));

  FetchRemoteChanges();

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(4u, CountLocalFile(app_id));
  VerifyLocalFolder(app_id, FPL("folder"));
  VerifyLocalFile(app_id, FPL("folder/file"), "abcde");

  EXPECT_EQ(5u, CountMetadata());
  EXPECT_EQ(5u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_ConflictTest_AddFolder_AddFolder) {
  std::string app_id = "example";

  RegisterApp(app_id);
  std::string app_root_folder_id = GetFileIDByPath(app_id, FPL(""));

  AddLocalFolder(app_id, FPL("conflict_to_pending_remote"));
  AddLocalFolder(app_id, FPL("conflict_to_existing_remote"));

  std::string remote_folder_id;
  EXPECT_EQ(google_apis::HTTP_CREATED,
            fake_drive_service_helper()->AddFolder(
                app_root_folder_id,
                "conflict_to_pending_remote", &remote_folder_id));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_CREATED,
            fake_drive_service_helper()->AddFolder(
                app_root_folder_id,
                "conflict_to_existing_remote", &remote_folder_id));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(3u, CountLocalFile(app_id));
  VerifyLocalFolder(app_id, FPL("conflict_to_pending_remote"));
  VerifyLocalFolder(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(4u, CountMetadata());
  EXPECT_EQ(4u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_AddFolder_DeleteFolder) {
  std::string app_id = "example";

  RegisterApp(app_id);

  AddLocalFolder(app_id, FPL("conflict_to_pending_remote"));
  AddLocalFolder(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  // Test body starts from here.
  RemoveLocal(app_id, FPL("conflict_to_pending_remote"));
  AddLocalFolder(app_id, FPL("conflict_to_pending_remote"));
  RemoveLocal(app_id, FPL("conflict_to_existing_remote"));
  AddLocalFolder(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_pending_remote"))));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_existing_remote"))));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(2u, CountLocalFile(app_id));
  VerifyLocalFolder(app_id, FPL("conflict_to_pending_remote"));

  EXPECT_EQ(3u, CountMetadata());
  EXPECT_EQ(3u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_AddFolder_AddFile) {
  std::string app_id = "example";

  RegisterApp(app_id);
  std::string app_root_folder_id = GetFileIDByPath(app_id, FPL(""));

  AddLocalFolder(app_id, FPL("conflict_to_pending_remote"));
  AddLocalFolder(app_id, FPL("conflict_to_existing_remote"));

  std::string file_id;
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddFile(
                app_root_folder_id, "conflict_to_pending_remote", "foo",
                &file_id));
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->UpdateModificationTime(
                file_id,
                base::Time::Now() + base::TimeDelta::FromDays(1)));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddFile(
                app_root_folder_id, "conflict_to_existing_remote", "foo",
                &file_id));
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->UpdateModificationTime(
                file_id,
                base::Time::Now() + base::TimeDelta::FromDays(1)));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(3u, CountLocalFile(app_id));
  VerifyLocalFolder(app_id, FPL("conflict_to_pending_remote"));
  VerifyLocalFolder(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(4u, CountMetadata());
  EXPECT_EQ(4u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_AddFolder_DeleteFile) {
  std::string app_id = "example";

  RegisterApp(app_id);
  std::string app_root_folder_id = GetFileIDByPath(app_id, FPL(""));

  AddOrUpdateLocalFile(app_id, FPL("conflict_to_pending_remote"), "foo");
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_existing_remote"), "bar");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  // Test body starts from here.
  RemoveLocal(app_id, FPL("conflict_to_pending_remote"));
  AddLocalFolder(app_id, FPL("conflict_to_pending_remote"));

  RemoveLocal(app_id, FPL("conflict_to_existing_remote"));
  AddLocalFolder(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_pending_remote"))));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_existing_remote"))));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(3u, CountLocalFile(app_id));
  VerifyLocalFolder(app_id, FPL("conflict_to_pending_remote"));
  VerifyLocalFolder(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(4u, CountMetadata());
  EXPECT_EQ(4u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_DeleteFolder_AddFolder) {
  std::string app_id = "example";

  RegisterApp(app_id);
  std::string app_root_folder_id = GetFileIDByPath(app_id, FPL(""));
  AddLocalFolder(app_id, FPL("conflict_to_pending_remote"));
  AddLocalFolder(app_id, FPL("conflict_to_existing_remote"));

  RemoveLocal(app_id, FPL("conflict_to_pending_remote"));
  RemoveLocal(app_id, FPL("conflict_to_existing_remote"));

  std::string file_id;
  EXPECT_EQ(google_apis::HTTP_CREATED,
            fake_drive_service_helper()->AddFolder(
                app_root_folder_id,
                "conflict_to_pending_remote", &file_id));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_CREATED,
            fake_drive_service_helper()->AddFolder(
                app_root_folder_id,
                "conflict_to_existing_remote", NULL));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(3u, CountLocalFile(app_id));
  VerifyLocalFolder(app_id, FPL("conflict_to_pending_remote"));
  VerifyLocalFolder(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(4u, CountMetadata());
  EXPECT_EQ(4u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_DeleteFolder_DeleteFolder) {
  std::string app_id = "example";

  RegisterApp(app_id);
  std::string app_root_folder_id = GetFileIDByPath(app_id, FPL(""));

  AddLocalFolder(app_id, FPL("conflict_to_pending_remote"));
  AddLocalFolder(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  // Test body starts from here.
  RemoveLocal(app_id, FPL("conflict_to_pending_remote"));
  RemoveLocal(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_pending_remote"))));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_existing_remote"))));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(1u, CountLocalFile(app_id));

  EXPECT_EQ(2u, CountMetadata());
  EXPECT_EQ(2u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_DeleteFolder_AddFile) {
  std::string app_id = "example";

  RegisterApp(app_id);
  std::string app_root_folder_id = GetFileIDByPath(app_id, FPL(""));

  AddLocalFolder(app_id, FPL("conflict_to_pending_remote"));
  AddLocalFolder(app_id, FPL("conflict_to_existing_remote"));
  RemoveLocal(app_id, FPL("conflict_to_pending_remote"));
  RemoveLocal(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddFile(
                app_root_folder_id, "conflict_to_pending_remote", "foo", NULL));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddFile(
                app_root_folder_id, "conflict_to_existing_remote", "bar",
                NULL));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(3u, CountLocalFile(app_id));
  VerifyLocalFile(app_id, FPL("conflict_to_pending_remote"), "foo");
  VerifyLocalFile(app_id, FPL("conflict_to_existing_remote"), "bar");

  EXPECT_EQ(4u, CountMetadata());
  EXPECT_EQ(4u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_DeleteFolder_DeleteFile) {
  std::string app_id = "example";

  RegisterApp(app_id);
  std::string app_root_folder_id = GetFileIDByPath(app_id, FPL(""));
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_pending_remote"), "foo");
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_existing_remote"), "bar");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  // Test body starts from here.
  RemoveLocal(app_id, FPL("conflict_to_pending_remote"));
  RemoveLocal(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_pending_remote"))));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_existing_remote"))));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(1u, CountLocalFile(app_id));

  EXPECT_EQ(2u, CountMetadata());
  EXPECT_EQ(2u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_AddFile_AddFolder) {
  std::string app_id = "example";

  RegisterApp(app_id);
  std::string app_root_folder_id = GetFileIDByPath(app_id, FPL(""));

  AddOrUpdateLocalFile(app_id, FPL("conflict_to_pending_remote"), "foo");
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_existing_remote"), "bar");

  std::string file_id;
  EXPECT_EQ(google_apis::HTTP_CREATED,
            fake_drive_service_helper()->AddFolder(
                app_root_folder_id, "conflict_to_pending_remote",
                &file_id));
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->UpdateModificationTime(
                file_id,
                base::Time::Now() - base::TimeDelta::FromDays(1)));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_CREATED,
            fake_drive_service_helper()->AddFolder(
                app_root_folder_id, "conflict_to_existing_remote",
                &file_id));
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->UpdateModificationTime(
                file_id,
                base::Time::Now() - base::TimeDelta::FromDays(1)));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(3u, CountLocalFile(app_id));
  VerifyLocalFolder(app_id, FPL("conflict_to_pending_remote"));
  VerifyLocalFolder(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(4u, CountMetadata());
  EXPECT_EQ(4u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_AddFile_DeleteFolder) {
  std::string app_id = "example";

  RegisterApp(app_id);
  std::string app_root_folder_id = GetFileIDByPath(app_id, FPL(""));

  AddLocalFolder(app_id, FPL("conflict_to_pending_remote"));
  AddLocalFolder(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  // Test body starts from here.
  RemoveLocal(app_id, FPL("conflict_to_pending_remote"));
  RemoveLocal(app_id, FPL("conflict_to_existing_remote"));
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_pending_remote"), "foo");
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_existing_remote"), "bar");

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_pending_remote"))));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_existing_remote"))));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(3u, CountLocalFile(app_id));
  VerifyLocalFile(app_id, FPL("conflict_to_pending_remote"), "foo");
  VerifyLocalFile(app_id, FPL("conflict_to_existing_remote"), "bar");

  EXPECT_EQ(4u, CountMetadata());
  EXPECT_EQ(4u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_AddFile_AddFile) {
  std::string app_id = "example";

  RegisterApp(app_id);

  std::string app_root_folder_id = GetFileIDByPath(app_id, FPL(""));
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_pending_remote"), "hoge");
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_existing_remote"), "fuga");

  std::string file_id;
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddFile(
                app_root_folder_id, "conflict_to_pending_remote", "foo",
                &file_id));
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->UpdateModificationTime(
                file_id,
                base::Time::Now() + base::TimeDelta::FromDays(1)));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddFile(
                app_root_folder_id, "conflict_to_existing_remote", "bar",
                &file_id));
  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->UpdateModificationTime(
                file_id,
                base::Time::Now() + base::TimeDelta::FromDays(1)));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(3u, CountLocalFile(app_id));
  VerifyLocalFile(app_id, FPL("conflict_to_pending_remote"), "foo");
  VerifyLocalFile(app_id, FPL("conflict_to_existing_remote"), "bar");

  EXPECT_EQ(4u, CountMetadata());
  EXPECT_EQ(4u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_AddFile_DeleteFile) {
  std::string app_id = "example";

  RegisterApp(app_id);

  AddOrUpdateLocalFile(app_id, FPL("conflict_to_pending_remote"), "foo");
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_existing_remote"), "bar");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  // Test body starts from here.
  RemoveLocal(app_id, FPL("conflict_to_pending_remote"));
  RemoveLocal(app_id, FPL("conflict_to_existing_remote"));
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_pending_remote"), "hoge");
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_existing_remote"), "fuga");

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_pending_remote"))));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_existing_remote"))));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(3u, CountLocalFile(app_id));
  VerifyLocalFile(app_id, FPL("conflict_to_pending_remote"), "hoge");
  VerifyLocalFile(app_id, FPL("conflict_to_existing_remote"), "fuga");

  EXPECT_EQ(4u, CountMetadata());
  EXPECT_EQ(4u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_UpdateFile_DeleteFile) {
  std::string app_id = "example";

  RegisterApp(app_id);

  AddOrUpdateLocalFile(app_id, FPL("conflict_to_pending_remote"), "foo");
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_existing_remote"), "bar");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  // Test body starts from here.
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_pending_remote"), "hoge");
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_existing_remote"), "fuga");

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_pending_remote"))));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_existing_remote"))));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(3u, CountLocalFile(app_id));
  VerifyLocalFile(app_id, FPL("conflict_to_pending_remote"), "hoge");
  VerifyLocalFile(app_id, FPL("conflict_to_existing_remote"), "fuga");

  EXPECT_EQ(4u, CountMetadata());
  EXPECT_EQ(4u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_DeleteFile_AddFolder) {
  std::string app_id = "example";

  RegisterApp(app_id);

  std::string app_root_folder_id = GetFileIDByPath(app_id, FPL(""));
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_pending_remote"), "foo");
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_existing_remote"), "bar");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  // Test body starts from here.
  RemoveLocal(app_id, FPL("conflict_to_pending_remote"));
  RemoveLocal(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(google_apis::HTTP_CREATED,
            fake_drive_service_helper()->AddFolder(
                app_root_folder_id, "conflict_to_pending_remote", NULL));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_CREATED,
            fake_drive_service_helper()->AddFolder(
                app_root_folder_id, "conflict_to_existing_remote", NULL));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(3u, CountLocalFile(app_id));
  VerifyLocalFolder(app_id, FPL("conflict_to_pending_remote"));
  VerifyLocalFolder(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(4u, CountMetadata());
  EXPECT_EQ(4u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_DeleteFile_DeleteFolder) {
  std::string app_id = "example";

  RegisterApp(app_id);

  AddLocalFolder(app_id, FPL("conflict_to_pending_remote"));
  AddLocalFolder(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  // Test body starts from here.
  RemoveLocal(app_id, FPL("conflict_to_pending_remote"));
  RemoveLocal(app_id, FPL("conflict_to_existing_remote"));

  AddOrUpdateLocalFile(app_id, FPL("conflict_to_pending_remote"), "foo");
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_existing_remote"), "bar");

  RemoveLocal(app_id, FPL("conflict_to_pending_remote"));
  RemoveLocal(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_pending_remote"))));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_existing_remote"))));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(1u, CountLocalFile(app_id));

  EXPECT_EQ(2u, CountMetadata());
  EXPECT_EQ(2u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_DeleteFile_AddFile) {
  std::string app_id = "example";

  RegisterApp(app_id);

  std::string app_root_folder_id = GetFileIDByPath(app_id, FPL(""));
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_pending_remote"), "foo");
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_existing_remote"), "bar");
  RemoveLocal(app_id, FPL("conflict_to_pending_remote"));
  RemoveLocal(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddFile(
                app_root_folder_id, "conflict_to_pending_remote", "hoge",
                NULL));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->AddFile(
                app_root_folder_id, "conflict_to_existing_remote", "fuga",
                NULL));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(3u, CountLocalFile(app_id));
  VerifyLocalFile(app_id, FPL("conflict_to_pending_remote"), "hoge");
  VerifyLocalFile(app_id, FPL("conflict_to_existing_remote"), "fuga");

  EXPECT_EQ(4u, CountMetadata());
  EXPECT_EQ(4u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_DeleteFile_UpdateFile) {
  std::string app_id = "example";

  RegisterApp(app_id);

  std::string app_root_folder_id = GetFileIDByPath(app_id, FPL(""));
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_pending_remote"), "foo");
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_existing_remote"), "bar");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  // Test body starts from here.
  RemoveLocal(app_id, FPL("conflict_to_pending_remote"));
  RemoveLocal(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->UpdateFile(
                GetFileIDByPath(app_id, FPL("conflict_to_pending_remote")),
                "hoge"));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_SUCCESS,
            fake_drive_service_helper()->UpdateFile(
                GetFileIDByPath(app_id, FPL("conflict_to_existing_remote")),
                "fuga"));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(3u, CountLocalFile(app_id));
  VerifyLocalFile(app_id, FPL("conflict_to_pending_remote"), "hoge");
  VerifyLocalFile(app_id, FPL("conflict_to_existing_remote"), "fuga");

  EXPECT_EQ(4u, CountMetadata());
  EXPECT_EQ(4u, CountTracker());
}

TEST_F(DriveBackendSyncTest, ConflictTest_DeleteFile_DeleteFile) {
  std::string app_id = "example";

  RegisterApp(app_id);

  std::string app_root_folder_id = GetFileIDByPath(app_id, FPL(""));
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_pending_remote"), "foo");
  AddOrUpdateLocalFile(app_id, FPL("conflict_to_existing_remote"), "bar");

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  // Test body starts from here.
  RemoveLocal(app_id, FPL("conflict_to_pending_remote"));
  RemoveLocal(app_id, FPL("conflict_to_existing_remote"));

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_pending_remote"))));

  FetchRemoteChanges();

  EXPECT_EQ(google_apis::HTTP_NO_CONTENT,
            fake_drive_service_helper()->DeleteResource(
                GetFileIDByPath(app_id, FPL("conflict_to_existing_remote"))));

  EXPECT_EQ(SYNC_STATUS_OK, ProcessChangesUntilDone());
  VerifyConsistency();

  EXPECT_EQ(1u, CountApp());
  EXPECT_EQ(1u, CountLocalFile(app_id));

  EXPECT_EQ(2u, CountMetadata());
  EXPECT_EQ(2u, CountTracker());
}

}  // namespace drive_backend
}  // namespace sync_file_system
