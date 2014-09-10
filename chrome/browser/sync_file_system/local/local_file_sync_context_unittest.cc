// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync_file_system/local/local_file_sync_context.h"

#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "base/stl_util.h"
#include "chrome/browser/sync_file_system/local/canned_syncable_file_system.h"
#include "chrome/browser/sync_file_system/local/local_file_change_tracker.h"
#include "chrome/browser/sync_file_system/local/sync_file_system_backend.h"
#include "chrome/browser/sync_file_system/sync_file_metadata.h"
#include "chrome/browser/sync_file_system/sync_status_code.h"
#include "chrome/browser/sync_file_system/syncable_file_system_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/mock_blob_url_request_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation_runner.h"
#include "storage/browser/fileapi/isolated_context.h"
#include "storage/common/blob/scoped_file.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/leveldatabase/src/helpers/memenv/memenv.h"
#include "third_party/leveldatabase/src/include/leveldb/env.h"

#define FPL FILE_PATH_LITERAL

using content::BrowserThread;
using storage::FileSystemContext;
using storage::FileSystemURL;
using storage::FileSystemURLSet;

// This tests LocalFileSyncContext behavior in multi-thread /
// multi-file-system-context environment.
// Basic combined tests (single-thread / single-file-system-context)
// that involve LocalFileSyncContext are also in
// syncable_file_system_unittests.cc.

namespace sync_file_system {

namespace {
const char kOrigin1[] = "http://example.com";
const char kOrigin2[] = "http://chromium.org";
}

class LocalFileSyncContextTest : public testing::Test {
 protected:
  LocalFileSyncContextTest()
      : thread_bundle_(
            content::TestBrowserThreadBundle::REAL_FILE_THREAD |
            content::TestBrowserThreadBundle::REAL_IO_THREAD),
        status_(SYNC_FILE_ERROR_FAILED),
        file_error_(base::File::FILE_ERROR_FAILED),
        async_modify_finished_(false),
        has_inflight_prepare_for_sync_(false) {}

  virtual void SetUp() OVERRIDE {
    RegisterSyncableFileSystem();
    ASSERT_TRUE(dir_.CreateUniqueTempDir());
    in_memory_env_.reset(leveldb::NewMemEnv(leveldb::Env::Default()));

    ui_task_runner_ = base::MessageLoop::current()->message_loop_proxy();
    io_task_runner_ = BrowserThread::GetMessageLoopProxyForThread(
        BrowserThread::IO);
    file_task_runner_ = BrowserThread::GetMessageLoopProxyForThread(
        BrowserThread::IO);
  }

  virtual void TearDown() OVERRIDE {
    RevokeSyncableFileSystem();
  }

  void StartPrepareForSync(FileSystemContext* file_system_context,
                           const FileSystemURL& url,
                           LocalFileSyncContext::SyncMode sync_mode,
                           SyncFileMetadata* metadata,
                           FileChangeList* changes,
                           storage::ScopedFile* snapshot) {
    ASSERT_TRUE(changes != NULL);
    ASSERT_FALSE(has_inflight_prepare_for_sync_);
    status_ = SYNC_STATUS_UNKNOWN;
    has_inflight_prepare_for_sync_ = true;
    sync_context_->PrepareForSync(
        file_system_context,
        url,
        sync_mode,
        base::Bind(&LocalFileSyncContextTest::DidPrepareForSync,
                   base::Unretained(this), metadata, changes, snapshot));
  }

  SyncStatusCode PrepareForSync(FileSystemContext* file_system_context,
                                const FileSystemURL& url,
                                LocalFileSyncContext::SyncMode sync_mode,
                                SyncFileMetadata* metadata,
                                FileChangeList* changes,
                                storage::ScopedFile* snapshot) {
    StartPrepareForSync(file_system_context, url, sync_mode,
                        metadata, changes, snapshot);
    base::MessageLoop::current()->Run();
    return status_;
  }

  base::Closure GetPrepareForSyncClosure(
      FileSystemContext* file_system_context,
      const FileSystemURL& url,
      LocalFileSyncContext::SyncMode sync_mode,
      SyncFileMetadata* metadata,
      FileChangeList* changes,
      storage::ScopedFile* snapshot) {
    return base::Bind(&LocalFileSyncContextTest::StartPrepareForSync,
                      base::Unretained(this),
                      base::Unretained(file_system_context),
                      url, sync_mode, metadata, changes, snapshot);
  }

  void DidPrepareForSync(SyncFileMetadata* metadata_out,
                         FileChangeList* changes_out,
                         storage::ScopedFile* snapshot_out,
                         SyncStatusCode status,
                         const LocalFileSyncInfo& sync_file_info,
                         storage::ScopedFile snapshot) {
    ASSERT_TRUE(ui_task_runner_->RunsTasksOnCurrentThread());
    has_inflight_prepare_for_sync_ = false;
    status_ = status;
    *metadata_out = sync_file_info.metadata;
    *changes_out = sync_file_info.changes;
    if (snapshot_out)
      *snapshot_out = snapshot.Pass();
    base::MessageLoop::current()->Quit();
  }

  SyncStatusCode ApplyRemoteChange(FileSystemContext* file_system_context,
                                   const FileChange& change,
                                   const base::FilePath& local_path,
                                   const FileSystemURL& url,
                                   SyncFileType expected_file_type) {
    SCOPED_TRACE(testing::Message() << "ApplyChange for " <<
                 url.DebugString());

    // First we should call PrepareForSync to disable writing.
    SyncFileMetadata metadata;
    FileChangeList changes;
    EXPECT_EQ(SYNC_STATUS_OK,
              PrepareForSync(file_system_context, url,
                             LocalFileSyncContext::SYNC_EXCLUSIVE,
                             &metadata, &changes, NULL));
    EXPECT_EQ(expected_file_type, metadata.file_type);

    status_ = SYNC_STATUS_UNKNOWN;
    sync_context_->ApplyRemoteChange(
        file_system_context, change, local_path, url,
        base::Bind(&LocalFileSyncContextTest::DidApplyRemoteChange,
                   base::Unretained(this),
                   make_scoped_refptr(file_system_context), url));
    base::MessageLoop::current()->Run();
    return status_;
  }

  void DidApplyRemoteChange(FileSystemContext* file_system_context,
                            const FileSystemURL& url,
                            SyncStatusCode status) {
    status_ = status;
    sync_context_->FinalizeExclusiveSync(
        file_system_context, url,
        status == SYNC_STATUS_OK /* clear_local_changes */,
        base::MessageLoop::QuitClosure());
  }

  void StartModifyFileOnIOThread(CannedSyncableFileSystem* file_system,
                                 const FileSystemURL& url) {
    ASSERT_TRUE(file_system != NULL);
    if (!io_task_runner_->RunsTasksOnCurrentThread()) {
      async_modify_finished_ = false;
      ASSERT_TRUE(ui_task_runner_->RunsTasksOnCurrentThread());
      io_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&LocalFileSyncContextTest::StartModifyFileOnIOThread,
                     base::Unretained(this), file_system, url));
      return;
    }
    ASSERT_TRUE(io_task_runner_->RunsTasksOnCurrentThread());
    file_error_ = base::File::FILE_ERROR_FAILED;
    file_system->operation_runner()->Truncate(
        url, 1, base::Bind(&LocalFileSyncContextTest::DidModifyFile,
                           base::Unretained(this)));
  }

  base::File::Error WaitUntilModifyFileIsDone() {
    while (!async_modify_finished_)
      base::MessageLoop::current()->RunUntilIdle();
    return file_error_;
  }

  void DidModifyFile(base::File::Error error) {
    if (!ui_task_runner_->RunsTasksOnCurrentThread()) {
      ASSERT_TRUE(io_task_runner_->RunsTasksOnCurrentThread());
      ui_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&LocalFileSyncContextTest::DidModifyFile,
                     base::Unretained(this), error));
      return;
    }
    ASSERT_TRUE(ui_task_runner_->RunsTasksOnCurrentThread());
    file_error_ = error;
    async_modify_finished_ = true;
  }

  void SimulateFinishSync(FileSystemContext* file_system_context,
                          const FileSystemURL& url,
                          SyncStatusCode status,
                          LocalFileSyncContext::SyncMode sync_mode) {
    if (sync_mode == LocalFileSyncContext::SYNC_SNAPSHOT) {
      sync_context_->FinalizeSnapshotSync(
          file_system_context, url, status,
          base::Bind(&base::DoNothing));
    } else {
      sync_context_->FinalizeExclusiveSync(
          file_system_context, url,
          status == SYNC_STATUS_OK /* clear_local_changes */,
          base::Bind(&base::DoNothing));
    }
  }

  void PrepareForSync_Basic(LocalFileSyncContext::SyncMode sync_mode,
                            SyncStatusCode simulate_sync_finish_status) {
    CannedSyncableFileSystem file_system(GURL(kOrigin1),
                                         in_memory_env_.get(),
                                         io_task_runner_.get(),
                                         file_task_runner_.get());
    file_system.SetUp(CannedSyncableFileSystem::QUOTA_ENABLED);
    sync_context_ = new LocalFileSyncContext(
        dir_.path(), in_memory_env_.get(),
        ui_task_runner_.get(), io_task_runner_.get());
    ASSERT_EQ(SYNC_STATUS_OK,
              file_system.MaybeInitializeFileSystemContext(
                  sync_context_.get()));
    ASSERT_EQ(base::File::FILE_OK, file_system.OpenFileSystem());

    const FileSystemURL kFile(file_system.URL("file"));
    EXPECT_EQ(base::File::FILE_OK, file_system.CreateFile(kFile));

    SyncFileMetadata metadata;
    FileChangeList changes;
    EXPECT_EQ(SYNC_STATUS_OK,
              PrepareForSync(file_system.file_system_context(), kFile,
                             sync_mode, &metadata, &changes, NULL));
    EXPECT_EQ(1U, changes.size());
    EXPECT_TRUE(changes.list().back().IsFile());
    EXPECT_TRUE(changes.list().back().IsAddOrUpdate());

    // We should see the same set of changes.
    file_system.GetChangesForURLInTracker(kFile, &changes);
    EXPECT_EQ(1U, changes.size());
    EXPECT_TRUE(changes.list().back().IsFile());
    EXPECT_TRUE(changes.list().back().IsAddOrUpdate());

    SimulateFinishSync(file_system.file_system_context(), kFile,
                       simulate_sync_finish_status, sync_mode);

    file_system.GetChangesForURLInTracker(kFile, &changes);
    if (simulate_sync_finish_status == SYNC_STATUS_OK) {
      // The change's cleared.
      EXPECT_TRUE(changes.empty());
    } else {
      EXPECT_EQ(1U, changes.size());
      EXPECT_TRUE(changes.list().back().IsFile());
      EXPECT_TRUE(changes.list().back().IsAddOrUpdate());
    }

    sync_context_->ShutdownOnUIThread();
    sync_context_ = NULL;

    file_system.TearDown();
  }

  void PrepareForSync_WriteDuringSync(
      LocalFileSyncContext::SyncMode sync_mode) {
    CannedSyncableFileSystem file_system(GURL(kOrigin1),
                                         in_memory_env_.get(),
                                         io_task_runner_.get(),
                                         file_task_runner_.get());
    file_system.SetUp(CannedSyncableFileSystem::QUOTA_ENABLED);
    sync_context_ = new LocalFileSyncContext(
        dir_.path(), in_memory_env_.get(),
        ui_task_runner_.get(), io_task_runner_.get());
    ASSERT_EQ(SYNC_STATUS_OK,
              file_system.MaybeInitializeFileSystemContext(
                  sync_context_.get()));
    ASSERT_EQ(base::File::FILE_OK, file_system.OpenFileSystem());

    const FileSystemURL kFile(file_system.URL("file"));
    EXPECT_EQ(base::File::FILE_OK, file_system.CreateFile(kFile));

    SyncFileMetadata metadata;
    FileChangeList changes;
    storage::ScopedFile snapshot;
    EXPECT_EQ(SYNC_STATUS_OK,
              PrepareForSync(file_system.file_system_context(), kFile,
                             sync_mode, &metadata, &changes, &snapshot));
    EXPECT_EQ(1U, changes.size());
    EXPECT_TRUE(changes.list().back().IsFile());
    EXPECT_TRUE(changes.list().back().IsAddOrUpdate());

    EXPECT_EQ(sync_mode == LocalFileSyncContext::SYNC_SNAPSHOT,
              !snapshot.path().empty());

    // Tracker keeps same set of changes.
    file_system.GetChangesForURLInTracker(kFile, &changes);
    EXPECT_EQ(1U, changes.size());
    EXPECT_TRUE(changes.list().back().IsFile());
    EXPECT_TRUE(changes.list().back().IsAddOrUpdate());

    StartModifyFileOnIOThread(&file_system, kFile);

    if (sync_mode == LocalFileSyncContext::SYNC_SNAPSHOT) {
      // Write should succeed.
      EXPECT_EQ(base::File::FILE_OK, WaitUntilModifyFileIsDone());
    } else {
      base::MessageLoop::current()->RunUntilIdle();
      EXPECT_FALSE(async_modify_finished_);
    }

    SimulateFinishSync(file_system.file_system_context(), kFile,
                       SYNC_STATUS_OK, sync_mode);

    EXPECT_EQ(base::File::FILE_OK, WaitUntilModifyFileIsDone());

    // Sync succeeded, but the other change that was made during or
    // after sync is recorded.
    file_system.GetChangesForURLInTracker(kFile, &changes);
    EXPECT_EQ(1U, changes.size());
    EXPECT_TRUE(changes.list().back().IsFile());
    EXPECT_TRUE(changes.list().back().IsAddOrUpdate());

    sync_context_->ShutdownOnUIThread();
    sync_context_ = NULL;

    file_system.TearDown();
  }

  base::ScopedTempDir dir_;
  scoped_ptr<leveldb::Env> in_memory_env_;

  // These need to remain until the very end.
  content::TestBrowserThreadBundle thread_bundle_;

  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> file_task_runner_;

  scoped_refptr<LocalFileSyncContext> sync_context_;

  SyncStatusCode status_;
  base::File::Error file_error_;
  bool async_modify_finished_;
  bool has_inflight_prepare_for_sync_;
};

TEST_F(LocalFileSyncContextTest, ConstructAndDestruct) {
  sync_context_ =
      new LocalFileSyncContext(
          dir_.path(), in_memory_env_.get(),
          ui_task_runner_.get(), io_task_runner_.get());
  sync_context_->ShutdownOnUIThread();
}

TEST_F(LocalFileSyncContextTest, InitializeFileSystemContext) {
  CannedSyncableFileSystem file_system(GURL(kOrigin1),
                                       in_memory_env_.get(),
                                       io_task_runner_.get(),
                                       file_task_runner_.get());
  file_system.SetUp(CannedSyncableFileSystem::QUOTA_ENABLED);

  sync_context_ = new LocalFileSyncContext(
      dir_.path(), in_memory_env_.get(),
      ui_task_runner_.get(), io_task_runner_.get());

  // Initializes file_system using |sync_context_|.
  EXPECT_EQ(SYNC_STATUS_OK,
            file_system.MaybeInitializeFileSystemContext(sync_context_.get()));

  // Make sure everything's set up for file_system to be able to handle
  // syncable file system operations.
  EXPECT_TRUE(file_system.backend()->sync_context() != NULL);
  EXPECT_TRUE(file_system.backend()->change_tracker() != NULL);
  EXPECT_EQ(sync_context_.get(), file_system.backend()->sync_context());

  // Calling MaybeInitialize for the same context multiple times must be ok.
  EXPECT_EQ(SYNC_STATUS_OK,
            file_system.MaybeInitializeFileSystemContext(sync_context_.get()));
  EXPECT_EQ(sync_context_.get(), file_system.backend()->sync_context());

  // Opens the file_system, perform some operation and see if the change tracker
  // correctly captures the change.
  EXPECT_EQ(base::File::FILE_OK, file_system.OpenFileSystem());

  const FileSystemURL kURL(file_system.URL("foo"));
  EXPECT_EQ(base::File::FILE_OK, file_system.CreateFile(kURL));

  FileSystemURLSet urls;
  file_system.GetChangedURLsInTracker(&urls);
  ASSERT_EQ(1U, urls.size());
  EXPECT_TRUE(ContainsKey(urls, kURL));

  // Finishing the test.
  sync_context_->ShutdownOnUIThread();
  file_system.TearDown();
}

TEST_F(LocalFileSyncContextTest, MultipleFileSystemContexts) {
  CannedSyncableFileSystem file_system1(GURL(kOrigin1),
                                        in_memory_env_.get(),
                                        io_task_runner_.get(),
                                        file_task_runner_.get());
  CannedSyncableFileSystem file_system2(GURL(kOrigin2),
                                        in_memory_env_.get(),
                                        io_task_runner_.get(),
                                        file_task_runner_.get());
  file_system1.SetUp(CannedSyncableFileSystem::QUOTA_ENABLED);
  file_system2.SetUp(CannedSyncableFileSystem::QUOTA_ENABLED);

  sync_context_ = new LocalFileSyncContext(
      dir_.path(), in_memory_env_.get(),
      ui_task_runner_.get(), io_task_runner_.get());

  // Initializes file_system1 and file_system2.
  EXPECT_EQ(SYNC_STATUS_OK,
            file_system1.MaybeInitializeFileSystemContext(sync_context_.get()));
  EXPECT_EQ(SYNC_STATUS_OK,
            file_system2.MaybeInitializeFileSystemContext(sync_context_.get()));

  EXPECT_EQ(base::File::FILE_OK, file_system1.OpenFileSystem());
  EXPECT_EQ(base::File::FILE_OK, file_system2.OpenFileSystem());

  const FileSystemURL kURL1(file_system1.URL("foo"));
  const FileSystemURL kURL2(file_system2.URL("bar"));

  // Creates a file in file_system1.
  EXPECT_EQ(base::File::FILE_OK, file_system1.CreateFile(kURL1));

  // file_system1's tracker must have recorded the change.
  FileSystemURLSet urls;
  file_system1.GetChangedURLsInTracker(&urls);
  ASSERT_EQ(1U, urls.size());
  EXPECT_TRUE(ContainsKey(urls, kURL1));

  // file_system1's tracker must have no change.
  urls.clear();
  file_system2.GetChangedURLsInTracker(&urls);
  ASSERT_TRUE(urls.empty());

  // Creates a directory in file_system2.
  EXPECT_EQ(base::File::FILE_OK, file_system2.CreateDirectory(kURL2));

  // file_system1's tracker must have the change for kURL1 as before.
  urls.clear();
  file_system1.GetChangedURLsInTracker(&urls);
  ASSERT_EQ(1U, urls.size());
  EXPECT_TRUE(ContainsKey(urls, kURL1));

  // file_system2's tracker now must have the change for kURL2.
  urls.clear();
  file_system2.GetChangedURLsInTracker(&urls);
  ASSERT_EQ(1U, urls.size());
  EXPECT_TRUE(ContainsKey(urls, kURL2));

  SyncFileMetadata metadata;
  FileChangeList changes;
  EXPECT_EQ(SYNC_STATUS_OK,
            PrepareForSync(file_system1.file_system_context(), kURL1,
                           LocalFileSyncContext::SYNC_EXCLUSIVE,
                           &metadata, &changes, NULL));
  EXPECT_EQ(1U, changes.size());
  EXPECT_TRUE(changes.list().back().IsFile());
  EXPECT_TRUE(changes.list().back().IsAddOrUpdate());
  EXPECT_EQ(SYNC_FILE_TYPE_FILE, metadata.file_type);
  EXPECT_EQ(0, metadata.size);

  changes.clear();
  EXPECT_EQ(SYNC_STATUS_OK,
            PrepareForSync(file_system2.file_system_context(), kURL2,
                           LocalFileSyncContext::SYNC_EXCLUSIVE,
                           &metadata, &changes, NULL));
  EXPECT_EQ(1U, changes.size());
  EXPECT_FALSE(changes.list().back().IsFile());
  EXPECT_TRUE(changes.list().back().IsAddOrUpdate());
  EXPECT_EQ(SYNC_FILE_TYPE_DIRECTORY, metadata.file_type);
  EXPECT_EQ(0, metadata.size);

  sync_context_->ShutdownOnUIThread();
  sync_context_ = NULL;

  file_system1.TearDown();
  file_system2.TearDown();
}

TEST_F(LocalFileSyncContextTest, PrepareSync_SyncSuccess_Exclusive) {
  PrepareForSync_Basic(LocalFileSyncContext::SYNC_EXCLUSIVE,
                       SYNC_STATUS_OK);
}

TEST_F(LocalFileSyncContextTest, PrepareSync_SyncSuccess_Snapshot) {
  PrepareForSync_Basic(LocalFileSyncContext::SYNC_SNAPSHOT,
                       SYNC_STATUS_OK);
}

TEST_F(LocalFileSyncContextTest, PrepareSync_SyncFailure_Exclusive) {
  PrepareForSync_Basic(LocalFileSyncContext::SYNC_EXCLUSIVE,
                       SYNC_STATUS_FAILED);
}

TEST_F(LocalFileSyncContextTest, PrepareSync_SyncFailure_Snapshot) {
  PrepareForSync_Basic(LocalFileSyncContext::SYNC_SNAPSHOT,
                       SYNC_STATUS_FAILED);
}

TEST_F(LocalFileSyncContextTest, PrepareSync_WriteDuringSync_Exclusive) {
  PrepareForSync_WriteDuringSync(LocalFileSyncContext::SYNC_EXCLUSIVE);
}

TEST_F(LocalFileSyncContextTest, PrepareSync_WriteDuringSync_Snapshot) {
  PrepareForSync_WriteDuringSync(LocalFileSyncContext::SYNC_SNAPSHOT);
}

// LocalFileSyncContextTest.PrepareSyncWhileWriting is flaky on android.
// http://crbug.com/239793
// It is also flaky on the TSAN v2 bots, and hangs other bots.
// http://crbug.com/305905.
TEST_F(LocalFileSyncContextTest, DISABLED_PrepareSyncWhileWriting) {
  CannedSyncableFileSystem file_system(GURL(kOrigin1),
                                       in_memory_env_.get(),
                                       io_task_runner_.get(),
                                       file_task_runner_.get());
  file_system.SetUp(CannedSyncableFileSystem::QUOTA_ENABLED);
  sync_context_ = new LocalFileSyncContext(
      dir_.path(), in_memory_env_.get(),
      ui_task_runner_.get(), io_task_runner_.get());
  EXPECT_EQ(SYNC_STATUS_OK,
            file_system.MaybeInitializeFileSystemContext(sync_context_.get()));

  EXPECT_EQ(base::File::FILE_OK, file_system.OpenFileSystem());

  const FileSystemURL kURL1(file_system.URL("foo"));

  // Creates a file in file_system.
  EXPECT_EQ(base::File::FILE_OK, file_system.CreateFile(kURL1));

  // Kick file write on IO thread.
  StartModifyFileOnIOThread(&file_system, kURL1);

  // Until the operation finishes PrepareForSync should return BUSY error.
  SyncFileMetadata metadata;
  metadata.file_type = SYNC_FILE_TYPE_UNKNOWN;
  FileChangeList changes;
  EXPECT_EQ(SYNC_STATUS_FILE_BUSY,
            PrepareForSync(file_system.file_system_context(), kURL1,
                           LocalFileSyncContext::SYNC_EXCLUSIVE,
                           &metadata, &changes, NULL));
  EXPECT_EQ(SYNC_FILE_TYPE_FILE, metadata.file_type);

  // Register PrepareForSync method to be invoked when kURL1 becomes
  // syncable. (Actually this may be done after all operations are done
  // on IO thread in this test.)
  metadata.file_type = SYNC_FILE_TYPE_UNKNOWN;
  changes.clear();
  sync_context_->RegisterURLForWaitingSync(
      kURL1, GetPrepareForSyncClosure(file_system.file_system_context(), kURL1,
                                      LocalFileSyncContext::SYNC_EXCLUSIVE,
                                      &metadata, &changes, NULL));

  // Wait for the completion.
  EXPECT_EQ(base::File::FILE_OK, WaitUntilModifyFileIsDone());

  // The PrepareForSync must have been started; wait until DidPrepareForSync
  // is done.
  base::MessageLoop::current()->Run();
  ASSERT_FALSE(has_inflight_prepare_for_sync_);

  // Now PrepareForSync should have run and returned OK.
  EXPECT_EQ(SYNC_STATUS_OK, status_);
  EXPECT_EQ(1U, changes.size());
  EXPECT_TRUE(changes.list().back().IsFile());
  EXPECT_TRUE(changes.list().back().IsAddOrUpdate());
  EXPECT_EQ(SYNC_FILE_TYPE_FILE, metadata.file_type);
  EXPECT_EQ(1, metadata.size);

  sync_context_->ShutdownOnUIThread();
  sync_context_ = NULL;
  file_system.TearDown();
}

TEST_F(LocalFileSyncContextTest, ApplyRemoteChangeForDeletion) {
  CannedSyncableFileSystem file_system(GURL(kOrigin1),
                                       in_memory_env_.get(),
                                       io_task_runner_.get(),
                                       file_task_runner_.get());
  file_system.SetUp(CannedSyncableFileSystem::QUOTA_ENABLED);

  sync_context_ = new LocalFileSyncContext(
      dir_.path(), in_memory_env_.get(),
      ui_task_runner_.get(), io_task_runner_.get());
  ASSERT_EQ(SYNC_STATUS_OK,
            file_system.MaybeInitializeFileSystemContext(sync_context_.get()));
  ASSERT_EQ(base::File::FILE_OK, file_system.OpenFileSystem());

  // Record the initial usage (likely 0).
  int64 initial_usage = -1;
  int64 quota = -1;
  EXPECT_EQ(storage::kQuotaStatusOk,
            file_system.GetUsageAndQuota(&initial_usage, &quota));

  // Create a file and directory in the file_system.
  const FileSystemURL kFile(file_system.URL("file"));
  const FileSystemURL kDir(file_system.URL("dir"));
  const FileSystemURL kChild(file_system.URL("dir/child"));

  EXPECT_EQ(base::File::FILE_OK, file_system.CreateFile(kFile));
  EXPECT_EQ(base::File::FILE_OK, file_system.CreateDirectory(kDir));
  EXPECT_EQ(base::File::FILE_OK, file_system.CreateFile(kChild));

  // file_system's change tracker must have recorded the creation.
  FileSystemURLSet urls;
  file_system.GetChangedURLsInTracker(&urls);
  ASSERT_EQ(3U, urls.size());
  ASSERT_TRUE(ContainsKey(urls, kFile));
  ASSERT_TRUE(ContainsKey(urls, kDir));
  ASSERT_TRUE(ContainsKey(urls, kChild));
  for (FileSystemURLSet::iterator iter = urls.begin();
       iter != urls.end(); ++iter) {
    file_system.ClearChangeForURLInTracker(*iter);
  }

  // At this point the usage must be greater than the initial usage.
  int64 new_usage = -1;
  EXPECT_EQ(storage::kQuotaStatusOk,
            file_system.GetUsageAndQuota(&new_usage, &quota));
  EXPECT_GT(new_usage, initial_usage);

  // Now let's apply remote deletion changes.
  FileChange change(FileChange::FILE_CHANGE_DELETE,
                    SYNC_FILE_TYPE_FILE);
  EXPECT_EQ(SYNC_STATUS_OK,
            ApplyRemoteChange(file_system.file_system_context(),
                              change, base::FilePath(), kFile,
                              SYNC_FILE_TYPE_FILE));

  // The implementation doesn't check file type for deletion, and it must be ok
  // even if we don't know if the deletion change was for a file or a directory.
  change = FileChange(FileChange::FILE_CHANGE_DELETE,
                      SYNC_FILE_TYPE_UNKNOWN);
  EXPECT_EQ(SYNC_STATUS_OK,
            ApplyRemoteChange(file_system.file_system_context(),
                              change, base::FilePath(), kDir,
                              SYNC_FILE_TYPE_DIRECTORY));

  // Check the directory/files are deleted successfully.
  EXPECT_EQ(base::File::FILE_ERROR_NOT_FOUND,
            file_system.FileExists(kFile));
  EXPECT_EQ(base::File::FILE_ERROR_NOT_FOUND,
            file_system.DirectoryExists(kDir));
  EXPECT_EQ(base::File::FILE_ERROR_NOT_FOUND,
            file_system.FileExists(kChild));

  // The changes applied by ApplyRemoteChange should not be recorded in
  // the change tracker.
  urls.clear();
  file_system.GetChangedURLsInTracker(&urls);
  EXPECT_TRUE(urls.empty());

  // The quota usage data must have reflected the deletion.
  EXPECT_EQ(storage::kQuotaStatusOk,
            file_system.GetUsageAndQuota(&new_usage, &quota));
  EXPECT_EQ(new_usage, initial_usage);

  sync_context_->ShutdownOnUIThread();
  sync_context_ = NULL;
  file_system.TearDown();
}

TEST_F(LocalFileSyncContextTest, ApplyRemoteChangeForDeletion_ForRoot) {
  CannedSyncableFileSystem file_system(GURL(kOrigin1),
                                       in_memory_env_.get(),
                                       io_task_runner_.get(),
                                       file_task_runner_.get());
  file_system.SetUp(CannedSyncableFileSystem::QUOTA_ENABLED);

  sync_context_ = new LocalFileSyncContext(
      dir_.path(), in_memory_env_.get(),
      ui_task_runner_.get(), io_task_runner_.get());
  ASSERT_EQ(SYNC_STATUS_OK,
            file_system.MaybeInitializeFileSystemContext(sync_context_.get()));
  ASSERT_EQ(base::File::FILE_OK, file_system.OpenFileSystem());

  // Record the initial usage (likely 0).
  int64 initial_usage = -1;
  int64 quota = -1;
  EXPECT_EQ(storage::kQuotaStatusOk,
            file_system.GetUsageAndQuota(&initial_usage, &quota));

  // Create a file and directory in the file_system.
  const FileSystemURL kFile(file_system.URL("file"));
  const FileSystemURL kDir(file_system.URL("dir"));
  const FileSystemURL kChild(file_system.URL("dir/child"));

  EXPECT_EQ(base::File::FILE_OK, file_system.CreateFile(kFile));
  EXPECT_EQ(base::File::FILE_OK, file_system.CreateDirectory(kDir));
  EXPECT_EQ(base::File::FILE_OK, file_system.CreateFile(kChild));

  // At this point the usage must be greater than the initial usage.
  int64 new_usage = -1;
  EXPECT_EQ(storage::kQuotaStatusOk,
            file_system.GetUsageAndQuota(&new_usage, &quota));
  EXPECT_GT(new_usage, initial_usage);

  const FileSystemURL kRoot(file_system.URL(""));

  // Now let's apply remote deletion changes for the root.
  FileChange change(FileChange::FILE_CHANGE_DELETE, SYNC_FILE_TYPE_DIRECTORY);
  EXPECT_EQ(SYNC_STATUS_OK,
            ApplyRemoteChange(file_system.file_system_context(),
                              change, base::FilePath(), kRoot,
                              SYNC_FILE_TYPE_DIRECTORY));

  // Check the directory/files are deleted successfully.
  EXPECT_EQ(base::File::FILE_ERROR_NOT_FOUND,
            file_system.FileExists(kFile));
  EXPECT_EQ(base::File::FILE_ERROR_NOT_FOUND,
            file_system.DirectoryExists(kDir));
  EXPECT_EQ(base::File::FILE_ERROR_NOT_FOUND,
            file_system.FileExists(kChild));

  // All changes made for the previous creation must have been also reset.
  FileSystemURLSet urls;
  file_system.GetChangedURLsInTracker(&urls);
  EXPECT_TRUE(urls.empty());

  // The quota usage data must have reflected the deletion.
  EXPECT_EQ(storage::kQuotaStatusOk,
            file_system.GetUsageAndQuota(&new_usage, &quota));
  EXPECT_EQ(new_usage, initial_usage);

  sync_context_->ShutdownOnUIThread();
  sync_context_ = NULL;
  file_system.TearDown();
}

TEST_F(LocalFileSyncContextTest, ApplyRemoteChangeForAddOrUpdate) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  CannedSyncableFileSystem file_system(GURL(kOrigin1),
                                       in_memory_env_.get(),
                                       io_task_runner_.get(),
                                       file_task_runner_.get());
  file_system.SetUp(CannedSyncableFileSystem::QUOTA_ENABLED);

  sync_context_ = new LocalFileSyncContext(
      dir_.path(), in_memory_env_.get(),
      ui_task_runner_.get(), io_task_runner_.get());
  ASSERT_EQ(SYNC_STATUS_OK,
            file_system.MaybeInitializeFileSystemContext(sync_context_.get()));
  ASSERT_EQ(base::File::FILE_OK, file_system.OpenFileSystem());

  const FileSystemURL kFile1(file_system.URL("file1"));
  const FileSystemURL kFile2(file_system.URL("file2"));
  const FileSystemURL kDir(file_system.URL("dir"));

  const char kTestFileData0[] = "0123456789";
  const char kTestFileData1[] = "Lorem ipsum!";
  const char kTestFileData2[] = "This is sample test data.";

  // Create kFile1 and populate it with kTestFileData0.
  EXPECT_EQ(base::File::FILE_OK, file_system.CreateFile(kFile1));
  EXPECT_EQ(static_cast<int64>(arraysize(kTestFileData0) - 1),
            file_system.WriteString(kFile1, kTestFileData0));

  // kFile2 and kDir are not there yet.
  EXPECT_EQ(base::File::FILE_ERROR_NOT_FOUND,
            file_system.FileExists(kFile2));
  EXPECT_EQ(base::File::FILE_ERROR_NOT_FOUND,
            file_system.DirectoryExists(kDir));

  // file_system's change tracker must have recorded the creation.
  FileSystemURLSet urls;
  file_system.GetChangedURLsInTracker(&urls);
  ASSERT_EQ(1U, urls.size());
  EXPECT_TRUE(ContainsKey(urls, kFile1));
  file_system.ClearChangeForURLInTracker(*urls.begin());

  // Prepare temporary files which represent the remote file data.
  const base::FilePath kFilePath1(temp_dir.path().Append(FPL("file1")));
  const base::FilePath kFilePath2(temp_dir.path().Append(FPL("file2")));

  ASSERT_EQ(static_cast<int>(arraysize(kTestFileData1) - 1),
            base::WriteFile(kFilePath1, kTestFileData1,
                            arraysize(kTestFileData1) - 1));
  ASSERT_EQ(static_cast<int>(arraysize(kTestFileData2) - 1),
            base::WriteFile(kFilePath2, kTestFileData2,
                            arraysize(kTestFileData2) - 1));

  // Record the usage.
  int64 usage = -1, new_usage = -1;
  int64 quota = -1;
  EXPECT_EQ(storage::kQuotaStatusOk,
            file_system.GetUsageAndQuota(&usage, &quota));

  // Here in the local filesystem we have:
  //  * kFile1 with kTestFileData0
  //
  // In the remote side let's assume we have:
  //  * kFile1 with kTestFileData1
  //  * kFile2 with kTestFileData2
  //  * kDir
  //
  // By calling ApplyChange's:
  //  * kFile1 will be updated to have kTestFileData1
  //  * kFile2 will be created
  //  * kDir will be created

  // Apply the remote change to kFile1 (which will update the file).
  FileChange change(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                    SYNC_FILE_TYPE_FILE);
  EXPECT_EQ(SYNC_STATUS_OK,
            ApplyRemoteChange(file_system.file_system_context(),
                              change, kFilePath1, kFile1,
                              SYNC_FILE_TYPE_FILE));

  // Check if the usage has been increased by (kTestFileData1 - kTestFileData0).
  const int updated_size =
      arraysize(kTestFileData1) - arraysize(kTestFileData0);
  EXPECT_EQ(storage::kQuotaStatusOk,
            file_system.GetUsageAndQuota(&new_usage, &quota));
  EXPECT_EQ(updated_size, new_usage - usage);

  // Apply remote changes to kFile2 and kDir (should create a file and
  // directory respectively).
  // They are non-existent yet so their expected file type (the last
  // parameter of ApplyRemoteChange) are
  // SYNC_FILE_TYPE_UNKNOWN.
  change = FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                      SYNC_FILE_TYPE_FILE);
  EXPECT_EQ(SYNC_STATUS_OK,
            ApplyRemoteChange(file_system.file_system_context(),
                              change, kFilePath2, kFile2,
                              SYNC_FILE_TYPE_UNKNOWN));

  change = FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                      SYNC_FILE_TYPE_DIRECTORY);
  EXPECT_EQ(SYNC_STATUS_OK,
            ApplyRemoteChange(file_system.file_system_context(),
                              change, base::FilePath(), kDir,
                              SYNC_FILE_TYPE_UNKNOWN));

  // Calling ApplyRemoteChange with different file type should be handled as
  // overwrite.
  change =
      FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE, SYNC_FILE_TYPE_FILE);
  EXPECT_EQ(SYNC_STATUS_OK,
            ApplyRemoteChange(file_system.file_system_context(),
                              change,
                              kFilePath1,
                              kDir,
                              SYNC_FILE_TYPE_DIRECTORY));
  EXPECT_EQ(base::File::FILE_OK, file_system.FileExists(kDir));

  change = FileChange(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                      SYNC_FILE_TYPE_DIRECTORY);
  EXPECT_EQ(SYNC_STATUS_OK,
            ApplyRemoteChange(file_system.file_system_context(),
                              change,
                              kFilePath1,
                              kDir,
                              SYNC_FILE_TYPE_FILE));

  // Creating a file/directory must have increased the usage more than
  // the size of kTestFileData2.
  new_usage = usage;
  EXPECT_EQ(storage::kQuotaStatusOk,
            file_system.GetUsageAndQuota(&new_usage, &quota));
  EXPECT_GT(new_usage,
            static_cast<int64>(usage + arraysize(kTestFileData2) - 1));

  // The changes applied by ApplyRemoteChange should not be recorded in
  // the change tracker.
  urls.clear();
  file_system.GetChangedURLsInTracker(&urls);
  EXPECT_TRUE(urls.empty());

  // Make sure all three files/directory exist.
  EXPECT_EQ(base::File::FILE_OK, file_system.FileExists(kFile1));
  EXPECT_EQ(base::File::FILE_OK, file_system.FileExists(kFile2));
  EXPECT_EQ(base::File::FILE_OK, file_system.DirectoryExists(kDir));

  sync_context_->ShutdownOnUIThread();
  file_system.TearDown();
}

TEST_F(LocalFileSyncContextTest, ApplyRemoteChangeForAddOrUpdate_NoParent) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  CannedSyncableFileSystem file_system(GURL(kOrigin1),
                                       in_memory_env_.get(),
                                       io_task_runner_.get(),
                                       file_task_runner_.get());
  file_system.SetUp(CannedSyncableFileSystem::QUOTA_ENABLED);

  sync_context_ = new LocalFileSyncContext(
      dir_.path(), in_memory_env_.get(),
      ui_task_runner_.get(), io_task_runner_.get());
  ASSERT_EQ(SYNC_STATUS_OK,
            file_system.MaybeInitializeFileSystemContext(sync_context_.get()));
  ASSERT_EQ(base::File::FILE_OK, file_system.OpenFileSystem());

  const char kTestFileData[] = "Lorem ipsum!";
  const FileSystemURL kDir(file_system.URL("dir"));
  const FileSystemURL kFile(file_system.URL("dir/file"));

  // Either kDir or kFile not exist yet.
  EXPECT_EQ(base::File::FILE_ERROR_NOT_FOUND, file_system.FileExists(kDir));
  EXPECT_EQ(base::File::FILE_ERROR_NOT_FOUND, file_system.FileExists(kFile));

  // Prepare a temporary file which represents remote file data.
  const base::FilePath kFilePath(temp_dir.path().Append(FPL("file")));
  ASSERT_EQ(static_cast<int>(arraysize(kTestFileData) - 1),
            base::WriteFile(kFilePath, kTestFileData,
                            arraysize(kTestFileData) - 1));

  // Calling ApplyChange's with kFilePath should create
  // kFile along with kDir.
  FileChange change(FileChange::FILE_CHANGE_ADD_OR_UPDATE,
                    SYNC_FILE_TYPE_FILE);
  EXPECT_EQ(SYNC_STATUS_OK,
            ApplyRemoteChange(file_system.file_system_context(),
                              change, kFilePath, kFile,
                              SYNC_FILE_TYPE_UNKNOWN));

  // The changes applied by ApplyRemoteChange should not be recorded in
  // the change tracker.
  FileSystemURLSet urls;
  urls.clear();
  file_system.GetChangedURLsInTracker(&urls);
  EXPECT_TRUE(urls.empty());

  // Make sure kDir and kFile are created by ApplyRemoteChange.
  EXPECT_EQ(base::File::FILE_OK, file_system.FileExists(kFile));
  EXPECT_EQ(base::File::FILE_OK, file_system.DirectoryExists(kDir));

  sync_context_->ShutdownOnUIThread();
  file_system.TearDown();
}

}  // namespace sync_file_system
