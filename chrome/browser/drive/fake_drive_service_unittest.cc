// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/drive/fake_drive_service.h"

#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/md5.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/drive/test_util.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "google_apis/drive/drive_api_parser.h"
#include "google_apis/drive/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

using google_apis::AboutResource;
using google_apis::AppList;
using google_apis::ChangeList;
using google_apis::ChangeResource;
using google_apis::FileList;
using google_apis::FileResource;
using google_apis::GDATA_NO_CONNECTION;
using google_apis::GDATA_OTHER_ERROR;
using google_apis::GDataErrorCode;
using google_apis::GetContentCallback;
using google_apis::HTTP_CREATED;
using google_apis::HTTP_FORBIDDEN;
using google_apis::HTTP_NOT_FOUND;
using google_apis::HTTP_NO_CONTENT;
using google_apis::HTTP_PRECONDITION;
using google_apis::HTTP_RESUME_INCOMPLETE;
using google_apis::HTTP_SUCCESS;
using google_apis::ProgressCallback;
using google_apis::UploadRangeResponse;

namespace drive {

namespace test_util {

using google_apis::test_util::AppendProgressCallbackResult;
using google_apis::test_util::CreateCopyResultCallback;
using google_apis::test_util::ProgressInfo;
using google_apis::test_util::TestGetContentCallback;
using google_apis::test_util::WriteStringToFile;

}  // namespace test_util

namespace {

class FakeDriveServiceTest : public testing::Test {
 protected:
  // Returns the resource entry that matches |resource_id|.
  scoped_ptr<FileResource> FindEntry(const std::string& resource_id) {
    GDataErrorCode error = GDATA_OTHER_ERROR;
    scoped_ptr<FileResource> entry;
    fake_service_.GetFileResource(
        resource_id, test_util::CreateCopyResultCallback(&error, &entry));
    base::RunLoop().RunUntilIdle();
    return entry.Pass();
  }

  // Returns true if the resource identified by |resource_id| exists.
  bool Exists(const std::string& resource_id) {
    scoped_ptr<FileResource> entry = FindEntry(resource_id);
    return entry && !entry->labels().is_trashed();
  }

  // Adds a new directory at |parent_resource_id| with the given name.
  // Returns true on success.
  bool AddNewDirectory(const std::string& parent_resource_id,
                       const std::string& directory_title) {
    GDataErrorCode error = GDATA_OTHER_ERROR;
    scoped_ptr<FileResource> entry;
    fake_service_.AddNewDirectory(
        parent_resource_id,
        directory_title,
        DriveServiceInterface::AddNewDirectoryOptions(),
        test_util::CreateCopyResultCallback(&error, &entry));
    base::RunLoop().RunUntilIdle();
    return error == HTTP_CREATED;
  }

  // Returns true if the resource identified by |resource_id| has a parent
  // identified by |parent_id|.
  bool HasParent(const std::string& resource_id, const std::string& parent_id) {
    scoped_ptr<FileResource> entry = FindEntry(resource_id);
    if (entry) {
      for (size_t i = 0; i < entry->parents().size(); ++i) {
        if (entry->parents()[i].file_id() == parent_id)
          return true;
      }
    }
    return false;
  }

  int64 GetLargestChangeByAboutResource() {
    GDataErrorCode error;
    scoped_ptr<AboutResource> about_resource;
    fake_service_.GetAboutResource(
        test_util::CreateCopyResultCallback(&error, &about_resource));
    base::RunLoop().RunUntilIdle();
    return about_resource->largest_change_id();
  }

  content::TestBrowserThreadBundle thread_bundle_;
  FakeDriveService fake_service_;
};

TEST_F(FakeDriveServiceTest, GetAllFileList) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.GetAllFileList(
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);
  // Do some sanity check.
  EXPECT_EQ(15U, file_list->items().size());
  EXPECT_EQ(1, fake_service_.file_list_load_count());
}

TEST_F(FakeDriveServiceTest, GetAllFileList_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.GetAllFileList(
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
  EXPECT_FALSE(file_list);
}

TEST_F(FakeDriveServiceTest, GetFileListInDirectory_InRootDirectory) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.GetFileListInDirectory(
      fake_service_.GetRootResourceId(),
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);
  // Do some sanity check. There are 8 entries in the root directory.
  EXPECT_EQ(8U, file_list->items().size());
  EXPECT_EQ(1, fake_service_.directory_load_count());
}

TEST_F(FakeDriveServiceTest, GetFileListInDirectory_InNonRootDirectory) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.GetFileListInDirectory(
      "1_folder_resource_id",
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);
  // Do some sanity check. There is three entries in 1_folder_resource_id
  // directory.
  EXPECT_EQ(3U, file_list->items().size());
  EXPECT_EQ(1, fake_service_.directory_load_count());
}

TEST_F(FakeDriveServiceTest, GetFileListInDirectory_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.GetFileListInDirectory(
      fake_service_.GetRootResourceId(),
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
  EXPECT_FALSE(file_list);
}

TEST_F(FakeDriveServiceTest, Search) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.Search(
      "File",  // search_query
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);
  // Do some sanity check. There are 4 entries that contain "File" in their
  // titles.
  EXPECT_EQ(4U, file_list->items().size());
}

TEST_F(FakeDriveServiceTest, Search_WithAttribute) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.Search(
      "title:1.txt",  // search_query
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);
  // Do some sanity check. There are 4 entries that contain "1.txt" in their
  // titles.
  EXPECT_EQ(4U, file_list->items().size());
}

TEST_F(FakeDriveServiceTest, Search_MultipleQueries) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.Search(
      "Directory 1",  // search_query
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);
  // There are 2 entries that contain both "Directory" and "1" in their titles.
  EXPECT_EQ(2U, file_list->items().size());

  fake_service_.Search(
      "\"Directory 1\"",  // search_query
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);
  // There is 1 entry that contain "Directory 1" in its title.
  EXPECT_EQ(1U, file_list->items().size());
}

TEST_F(FakeDriveServiceTest, Search_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.Search(
      "Directory 1",  // search_query
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
  EXPECT_FALSE(file_list);
}

TEST_F(FakeDriveServiceTest, Search_Deleted) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.DeleteResource("2_file_resource_id",
                               std::string(),  // etag
                               test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(HTTP_NO_CONTENT, error);

  error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.Search(
      "File",  // search_query
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);
  // Do some sanity check. There are 4 entries that contain "File" in their
  // titles and one of them is deleted.
  EXPECT_EQ(3U, file_list->items().size());
}

TEST_F(FakeDriveServiceTest, Search_Trashed) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.TrashResource("2_file_resource_id",
                              test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(HTTP_SUCCESS, error);

  error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.Search(
      "File",  // search_query
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);
  // Do some sanity check. There are 4 entries that contain "File" in their
  // titles and one of them is deleted.
  EXPECT_EQ(3U, file_list->items().size());
}

TEST_F(FakeDriveServiceTest, SearchByTitle) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.SearchByTitle(
      "1.txt",  // title
      fake_service_.GetRootResourceId(),  // directory_resource_id
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);
  // Do some sanity check. There are 2 entries that contain "1.txt" in their
  // titles directly under the root directory.
  EXPECT_EQ(2U, file_list->items().size());
}

TEST_F(FakeDriveServiceTest, SearchByTitle_EmptyDirectoryResourceId) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.SearchByTitle(
      "1.txt",  // title
      "",  // directory resource id
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);
  // Do some sanity check. There are 4 entries that contain "1.txt" in their
  // titles.
  EXPECT_EQ(4U, file_list->items().size());
}

TEST_F(FakeDriveServiceTest, SearchByTitle_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.SearchByTitle(
      "Directory 1",  // title
      fake_service_.GetRootResourceId(),  // directory_resource_id
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
  EXPECT_FALSE(file_list);
}

TEST_F(FakeDriveServiceTest, GetChangeList_NoNewEntries) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<ChangeList> change_list;
  fake_service_.GetChangeList(
      fake_service_.about_resource().largest_change_id() + 1,
      test_util::CreateCopyResultCallback(&error, &change_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(change_list);
  EXPECT_EQ(fake_service_.about_resource().largest_change_id(),
            change_list->largest_change_id());
  // This should be empty as the latest changestamp was passed to
  // GetChangeList(), hence there should be no new entries.
  EXPECT_EQ(0U, change_list->items().size());
  // It's considered loaded even if the result is empty.
  EXPECT_EQ(1, fake_service_.change_list_load_count());
}

TEST_F(FakeDriveServiceTest, GetChangeList_WithNewEntry) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  const int64 old_largest_change_id =
      fake_service_.about_resource().largest_change_id();

  // Add a new directory in the root directory.
  ASSERT_TRUE(AddNewDirectory(
      fake_service_.GetRootResourceId(), "new directory"));

  // Get the resource list newer than old_largest_change_id.
  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<ChangeList> change_list;
  fake_service_.GetChangeList(
      old_largest_change_id + 1,
      test_util::CreateCopyResultCallback(&error, &change_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(change_list);
  EXPECT_EQ(fake_service_.about_resource().largest_change_id(),
            change_list->largest_change_id());
  // The result should only contain the newly created directory.
  ASSERT_EQ(1U, change_list->items().size());
  ASSERT_TRUE(change_list->items()[0]->file());
  EXPECT_EQ("new directory", change_list->items()[0]->file()->title());
  EXPECT_EQ(1, fake_service_.change_list_load_count());
}

TEST_F(FakeDriveServiceTest, GetChangeList_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<ChangeList> change_list;
  fake_service_.GetChangeList(
      654321,  // start_changestamp
      test_util::CreateCopyResultCallback(&error, &change_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
  EXPECT_FALSE(change_list);
}

TEST_F(FakeDriveServiceTest, GetChangeList_DeletedEntry) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  ASSERT_TRUE(Exists("2_file_resource_id"));
  const int64 old_largest_change_id =
      fake_service_.about_resource().largest_change_id();

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.DeleteResource("2_file_resource_id",
                               std::string(),  // etag
                               test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(HTTP_NO_CONTENT, error);
  ASSERT_FALSE(Exists("2_file_resource_id"));

  // Get the resource list newer than old_largest_change_id.
  error = GDATA_OTHER_ERROR;
  scoped_ptr<ChangeList> change_list;
  fake_service_.GetChangeList(
      old_largest_change_id + 1,
      test_util::CreateCopyResultCallback(&error, &change_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(change_list);
  EXPECT_EQ(fake_service_.about_resource().largest_change_id(),
            change_list->largest_change_id());
  // The result should only contain the deleted file.
  ASSERT_EQ(1U, change_list->items().size());
  const ChangeResource& item = *change_list->items()[0];
  EXPECT_EQ("2_file_resource_id", item.file_id());
  EXPECT_FALSE(item.file());
  EXPECT_TRUE(item.is_deleted());
  EXPECT_EQ(1, fake_service_.change_list_load_count());
}

TEST_F(FakeDriveServiceTest, GetChangeList_TrashedEntry) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  ASSERT_TRUE(Exists("2_file_resource_id"));
  const int64 old_largest_change_id =
      fake_service_.about_resource().largest_change_id();

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.TrashResource("2_file_resource_id",
                              test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(HTTP_SUCCESS, error);
  ASSERT_FALSE(Exists("2_file_resource_id"));

  // Get the resource list newer than old_largest_change_id.
  error = GDATA_OTHER_ERROR;
  scoped_ptr<ChangeList> change_list;
  fake_service_.GetChangeList(
      old_largest_change_id + 1,
      test_util::CreateCopyResultCallback(&error, &change_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(change_list);
  EXPECT_EQ(fake_service_.about_resource().largest_change_id(),
            change_list->largest_change_id());
  // The result should only contain the trashed file.
  ASSERT_EQ(1U, change_list->items().size());
  const ChangeResource& item = *change_list->items()[0];
  EXPECT_EQ("2_file_resource_id", item.file_id());
  ASSERT_TRUE(item.file());
  EXPECT_TRUE(item.file()->labels().is_trashed());
  EXPECT_EQ(1, fake_service_.change_list_load_count());
}

TEST_F(FakeDriveServiceTest, GetRemainingFileList_GetAllFileList) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_default_max_results(6);

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.GetAllFileList(
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);

  // Do some sanity check.
  // The number of results is 14 entries. Thus, it should split into three
  // chunks: 6, 6, and then 2.
  EXPECT_EQ(6U, file_list->items().size());
  EXPECT_EQ(1, fake_service_.file_list_load_count());

  // Second page loading.
  // Keep the next url before releasing the |file_list|.
  GURL next_url(file_list->next_link());

  error = GDATA_OTHER_ERROR;
  file_list.reset();
  fake_service_.GetRemainingFileList(
      next_url,
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);

  EXPECT_EQ(6U, file_list->items().size());
  EXPECT_EQ(1, fake_service_.file_list_load_count());

  // Third page loading.
  next_url = file_list->next_link();

  error = GDATA_OTHER_ERROR;
  file_list.reset();
  fake_service_.GetRemainingFileList(
      next_url,
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);

  EXPECT_EQ(3U, file_list->items().size());
  EXPECT_EQ(1, fake_service_.file_list_load_count());
}

TEST_F(FakeDriveServiceTest, GetRemainingFileList_GetFileListInDirectory) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_default_max_results(3);

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.GetFileListInDirectory(
      fake_service_.GetRootResourceId(),
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);

  // Do some sanity check.
  // The number of results is 8 entries. Thus, it should split into three
  // chunks: 3, 3, and then 2.
  EXPECT_EQ(3U, file_list->items().size());
  EXPECT_EQ(1, fake_service_.directory_load_count());

  // Second page loading.
  // Keep the next url before releasing the |file_list|.
  GURL next_url = file_list->next_link();

  error = GDATA_OTHER_ERROR;
  file_list.reset();
  fake_service_.GetRemainingFileList(
      next_url,
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);

  EXPECT_EQ(3U, file_list->items().size());
  EXPECT_EQ(1, fake_service_.directory_load_count());

  // Third page loading.
  next_url = file_list->next_link();

  error = GDATA_OTHER_ERROR;
  file_list.reset();
  fake_service_.GetRemainingFileList(
      next_url,
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);

  EXPECT_EQ(2U, file_list->items().size());
  EXPECT_EQ(1, fake_service_.directory_load_count());
}

TEST_F(FakeDriveServiceTest, GetRemainingFileList_Search) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_default_max_results(2);

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileList> file_list;
  fake_service_.Search(
      "File",  // search_query
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);

  // Do some sanity check.
  // The number of results is 4 entries. Thus, it should split into two
  // chunks: 2, and then 2
  EXPECT_EQ(2U, file_list->items().size());

  // Second page loading.
  // Keep the next url before releasing the |file_list|.
  GURL next_url = file_list->next_link();

  error = GDATA_OTHER_ERROR;
  file_list.reset();
  fake_service_.GetRemainingFileList(
      next_url,
      test_util::CreateCopyResultCallback(&error, &file_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(file_list);

  EXPECT_EQ(2U, file_list->items().size());
}

TEST_F(FakeDriveServiceTest, GetRemainingChangeList_GetChangeList) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_default_max_results(2);
  const int64 old_largest_change_id =
      fake_service_.about_resource().largest_change_id();

  // Add 5 new directory in the root directory.
  for (int i = 0; i < 5; ++i) {
    ASSERT_TRUE(AddNewDirectory(
        fake_service_.GetRootResourceId(),
        base::StringPrintf("new directory %d", i)));
  }

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<ChangeList> change_list;
  fake_service_.GetChangeList(
      old_largest_change_id + 1,  // start_changestamp
      test_util::CreateCopyResultCallback(&error, &change_list));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(change_list);

  // Do some sanity check.
  // The number of results is 5 entries. Thus, it should split into three
  // chunks: 2, 2 and then 1.
  EXPECT_EQ(2U, change_list->items().size());
  EXPECT_EQ(1, fake_service_.change_list_load_count());

  // Second page loading.
  // Keep the next url before releasing the |change_list|.
  GURL next_url = change_list->next_link();

  error = GDATA_OTHER_ERROR;
  change_list.reset();
  fake_service_.GetRemainingChangeList(
      next_url,
      test_util::CreateCopyResultCallback(&error, &change_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(change_list);

  EXPECT_EQ(2U, change_list->items().size());
  EXPECT_EQ(1, fake_service_.change_list_load_count());

  // Third page loading.
  next_url = change_list->next_link();

  error = GDATA_OTHER_ERROR;
  change_list.reset();
  fake_service_.GetRemainingChangeList(
      next_url,
      test_util::CreateCopyResultCallback(&error, &change_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(change_list);

  EXPECT_EQ(1U, change_list->items().size());
  EXPECT_EQ(1, fake_service_.change_list_load_count());
}

TEST_F(FakeDriveServiceTest, GetAboutResource) {
  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<AboutResource> about_resource;
  fake_service_.GetAboutResource(
      test_util::CreateCopyResultCallback(&error, &about_resource));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);

  ASSERT_TRUE(about_resource);
  // Do some sanity check.
  EXPECT_EQ(fake_service_.GetRootResourceId(),
            about_resource->root_folder_id());
  EXPECT_EQ(1, fake_service_.about_resource_load_count());
}

TEST_F(FakeDriveServiceTest, GetAboutResource_Offline) {
  fake_service_.set_offline(true);

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<AboutResource> about_resource;
  fake_service_.GetAboutResource(
      test_util::CreateCopyResultCallback(&error, &about_resource));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
  EXPECT_FALSE(about_resource);
}

TEST_F(FakeDriveServiceTest, GetAppList) {
  ASSERT_TRUE(fake_service_.LoadAppListForDriveApi(
      "drive/applist.json"));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<AppList> app_list;
  fake_service_.GetAppList(
      test_util::CreateCopyResultCallback(&error, &app_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);

  ASSERT_TRUE(app_list);
  EXPECT_EQ(1, fake_service_.app_list_load_count());
}

TEST_F(FakeDriveServiceTest, GetAppList_Offline) {
  ASSERT_TRUE(fake_service_.LoadAppListForDriveApi(
      "drive/applist.json"));
  fake_service_.set_offline(true);

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<AppList> app_list;
  fake_service_.GetAppList(
      test_util::CreateCopyResultCallback(&error, &app_list));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
  EXPECT_FALSE(app_list);
}

TEST_F(FakeDriveServiceTest, GetFileResource_ExistingFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  const std::string kResourceId = "2_file_resource_id";
  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.GetFileResource(
      kResourceId, test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(entry);
  // Do some sanity check.
  EXPECT_EQ(kResourceId, entry->file_id());
}

TEST_F(FakeDriveServiceTest, GetFileResource_NonexistingFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  const std::string kResourceId = "nonexisting_resource_id";
  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.GetFileResource(
      kResourceId, test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NOT_FOUND, error);
  ASSERT_FALSE(entry);
}

TEST_F(FakeDriveServiceTest, GetFileResource_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  const std::string kResourceId = "2_file_resource_id";
  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.GetFileResource(
      kResourceId, test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
  EXPECT_FALSE(entry);
}

TEST_F(FakeDriveServiceTest, GetShareUrl) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  const std::string kResourceId = "2_file_resource_id";
  GDataErrorCode error = GDATA_OTHER_ERROR;
  GURL share_url;
  fake_service_.GetShareUrl(
      kResourceId,
      GURL(),  // embed origin
      test_util::CreateCopyResultCallback(&error, &share_url));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  EXPECT_FALSE(share_url.is_empty());
}

TEST_F(FakeDriveServiceTest, DeleteResource_ExistingFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  // Resource "2_file_resource_id" should now exist.
  ASSERT_TRUE(Exists("2_file_resource_id"));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.DeleteResource("2_file_resource_id",
                               std::string(),  // etag
                               test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NO_CONTENT, error);
  // Resource "2_file_resource_id" should be gone now.
  EXPECT_FALSE(Exists("2_file_resource_id"));

  error = GDATA_OTHER_ERROR;
  fake_service_.DeleteResource("2_file_resource_id",
                               std::string(),  // etag
                               test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(HTTP_NOT_FOUND, error);
  EXPECT_FALSE(Exists("2_file_resource_id"));
}

TEST_F(FakeDriveServiceTest, DeleteResource_NonexistingFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.DeleteResource("nonexisting_resource_id",
                               std::string(),  // etag
                               test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NOT_FOUND, error);
}

TEST_F(FakeDriveServiceTest, DeleteResource_ETagMatch) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  // Resource "2_file_resource_id" should now exist.
  scoped_ptr<FileResource> entry = FindEntry("2_file_resource_id");
  ASSERT_TRUE(entry);
  ASSERT_FALSE(entry->labels().is_trashed());
  ASSERT_FALSE(entry->etag().empty());

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.DeleteResource("2_file_resource_id",
                               entry->etag() + "_mismatch",
                               test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_PRECONDITION, error);
  // Resource "2_file_resource_id" should still exist.
  EXPECT_TRUE(Exists("2_file_resource_id"));

  error = GDATA_OTHER_ERROR;
  fake_service_.DeleteResource("2_file_resource_id",
                               entry->etag(),
                               test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(HTTP_NO_CONTENT, error);
  // Resource "2_file_resource_id" should be gone now.
  EXPECT_FALSE(Exists("2_file_resource_id"));
}

TEST_F(FakeDriveServiceTest, DeleteResource_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.DeleteResource("2_file_resource_id",
                               std::string(),  // etag
                               test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
}

TEST_F(FakeDriveServiceTest, DeleteResource_Forbidden) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  EXPECT_EQ(HTTP_SUCCESS, fake_service_.SetUserPermission(
      "2_file_resource_id", google_apis::drive::PERMISSION_ROLE_READER));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.DeleteResource("2_file_resource_id",
                               std::string(),  // etag
                               test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_FORBIDDEN, error);
}

TEST_F(FakeDriveServiceTest, TrashResource_ExistingFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  // Resource "2_file_resource_id" should now exist.
  ASSERT_TRUE(Exists("2_file_resource_id"));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.TrashResource("2_file_resource_id",
                              test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  // Resource "2_file_resource_id" should be gone now.
  EXPECT_FALSE(Exists("2_file_resource_id"));

  error = GDATA_OTHER_ERROR;
  fake_service_.TrashResource("2_file_resource_id",
                              test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(HTTP_NOT_FOUND, error);
  EXPECT_FALSE(Exists("2_file_resource_id"));
}

TEST_F(FakeDriveServiceTest, TrashResource_NonexistingFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.TrashResource("nonexisting_resource_id",
                              test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NOT_FOUND, error);
}

TEST_F(FakeDriveServiceTest, TrashResource_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.TrashResource("2_file_resource_id",
                              test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
}

TEST_F(FakeDriveServiceTest, TrashResource_Forbidden) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  EXPECT_EQ(HTTP_SUCCESS, fake_service_.SetUserPermission(
      "2_file_resource_id", google_apis::drive::PERMISSION_ROLE_READER));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.TrashResource("2_file_resource_id",
                              test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_FORBIDDEN, error);
}

TEST_F(FakeDriveServiceTest, DownloadFile_ExistingFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  std::vector<test_util::ProgressInfo> download_progress_values;

  const base::FilePath kOutputFilePath =
      temp_dir.path().AppendASCII("whatever.txt");
  GDataErrorCode error = GDATA_OTHER_ERROR;
  base::FilePath output_file_path;
  test_util::TestGetContentCallback get_content_callback;
  fake_service_.DownloadFile(
      kOutputFilePath,
      "2_file_resource_id",
      test_util::CreateCopyResultCallback(&error, &output_file_path),
      get_content_callback.callback(),
      base::Bind(&test_util::AppendProgressCallbackResult,
                 &download_progress_values));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  EXPECT_EQ(output_file_path, kOutputFilePath);
  std::string content;
  ASSERT_TRUE(base::ReadFileToString(output_file_path, &content));
  EXPECT_EQ("This is some test content.", content);
  ASSERT_TRUE(!download_progress_values.empty());
  EXPECT_TRUE(base::STLIsSorted(download_progress_values));
  EXPECT_LE(0, download_progress_values.front().first);
  EXPECT_GE(26, download_progress_values.back().first);
  EXPECT_EQ(content, get_content_callback.GetConcatenatedData());
}

TEST_F(FakeDriveServiceTest, DownloadFile_NonexistingFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  const base::FilePath kOutputFilePath =
      temp_dir.path().AppendASCII("whatever.txt");
  GDataErrorCode error = GDATA_OTHER_ERROR;
  base::FilePath output_file_path;
  fake_service_.DownloadFile(
      kOutputFilePath,
      "non_existent_file_resource_id",
      test_util::CreateCopyResultCallback(&error, &output_file_path),
      GetContentCallback(),
      ProgressCallback());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NOT_FOUND, error);
}

TEST_F(FakeDriveServiceTest, DownloadFile_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  const base::FilePath kOutputFilePath =
      temp_dir.path().AppendASCII("whatever.txt");
  GDataErrorCode error = GDATA_OTHER_ERROR;
  base::FilePath output_file_path;
  fake_service_.DownloadFile(
      kOutputFilePath,
      "2_file_resource_id",
      test_util::CreateCopyResultCallback(&error, &output_file_path),
      GetContentCallback(),
      ProgressCallback());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
}

TEST_F(FakeDriveServiceTest, CopyResource) {
  const base::Time::Exploded kModifiedDate = {2012, 7, 0, 19, 15, 59, 13, 123};

  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  int64 old_largest_change_id = GetLargestChangeByAboutResource();

  const std::string kResourceId = "2_file_resource_id";
  const std::string kParentResourceId = "2_folder_resource_id";
  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.CopyResource(
      kResourceId,
      kParentResourceId,
      "new title",
      base::Time::FromUTCExploded(kModifiedDate),
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(entry);
  // The copied entry should have the new resource ID and the title.
  EXPECT_NE(kResourceId, entry->file_id());
  EXPECT_EQ("new title", entry->title());
  EXPECT_EQ(base::Time::FromUTCExploded(kModifiedDate), entry->modified_date());
  EXPECT_TRUE(HasParent(entry->file_id(), kParentResourceId));
  // Should be incremented as a new hosted document was created.
  EXPECT_EQ(old_largest_change_id + 1,
            fake_service_.about_resource().largest_change_id());
  EXPECT_EQ(old_largest_change_id + 1, GetLargestChangeByAboutResource());
}

TEST_F(FakeDriveServiceTest, CopyResource_NonExisting) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  const std::string kResourceId = "nonexisting_resource_id";
  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.CopyResource(
      kResourceId,
      "1_folder_resource_id",
      "new title",
      base::Time(),
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NOT_FOUND, error);
}

TEST_F(FakeDriveServiceTest, CopyResource_EmptyParentResourceId) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  int64 old_largest_change_id = GetLargestChangeByAboutResource();

  const std::string kResourceId = "2_file_resource_id";
  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.CopyResource(
      kResourceId,
      std::string(),
      "new title",
      base::Time(),
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(entry);
  // The copied entry should have the new resource ID and the title.
  EXPECT_NE(kResourceId, entry->file_id());
  EXPECT_EQ("new title", entry->title());
  EXPECT_TRUE(HasParent(kResourceId, fake_service_.GetRootResourceId()));
  // Should be incremented as a new hosted document was created.
  EXPECT_EQ(old_largest_change_id + 1,
            fake_service_.about_resource().largest_change_id());
  EXPECT_EQ(old_largest_change_id + 1, GetLargestChangeByAboutResource());
}

TEST_F(FakeDriveServiceTest, CopyResource_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  const std::string kResourceId = "2_file_resource_id";
  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.CopyResource(
      kResourceId,
      "1_folder_resource_id",
      "new title",
      base::Time(),
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
  EXPECT_FALSE(entry);
}

TEST_F(FakeDriveServiceTest, UpdateResource) {
  const base::Time::Exploded kModifiedDate = {2012, 7, 0, 19, 15, 59, 13, 123};
  const base::Time::Exploded kViewedDate = {2013, 8, 1, 20, 16, 00, 14, 234};

  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  int64 old_largest_change_id = GetLargestChangeByAboutResource();

  const std::string kResourceId = "2_file_resource_id";
  const std::string kParentResourceId = "2_folder_resource_id";
  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.UpdateResource(
      kResourceId,
      kParentResourceId,
      "new title",
      base::Time::FromUTCExploded(kModifiedDate),
      base::Time::FromUTCExploded(kViewedDate),
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(entry);
  // The updated entry should have the new title.
  EXPECT_EQ(kResourceId, entry->file_id());
  EXPECT_EQ("new title", entry->title());
  EXPECT_EQ(base::Time::FromUTCExploded(kModifiedDate),
            entry->modified_date());
  EXPECT_EQ(base::Time::FromUTCExploded(kViewedDate),
            entry->last_viewed_by_me_date());
  EXPECT_TRUE(HasParent(kResourceId, kParentResourceId));
  // Should be incremented as a new hosted document was created.
  EXPECT_EQ(old_largest_change_id + 1,
            fake_service_.about_resource().largest_change_id());
  EXPECT_EQ(old_largest_change_id + 1, GetLargestChangeByAboutResource());
}

TEST_F(FakeDriveServiceTest, UpdateResource_NonExisting) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  const std::string kResourceId = "nonexisting_resource_id";
  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.UpdateResource(
      kResourceId,
      "1_folder_resource_id",
      "new title",
      base::Time(),
      base::Time(),
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NOT_FOUND, error);
}

TEST_F(FakeDriveServiceTest, UpdateResource_EmptyParentResourceId) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  int64 old_largest_change_id = GetLargestChangeByAboutResource();

  const std::string kResourceId = "2_file_resource_id";

  // Just make sure that the resource is under root.
  ASSERT_TRUE(HasParent(kResourceId, "fake_root"));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.UpdateResource(
      kResourceId,
      std::string(),
      "new title",
      base::Time(),
      base::Time(),
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(entry);
  // The updated entry should have the new title.
  EXPECT_EQ(kResourceId, entry->file_id());
  EXPECT_EQ("new title", entry->title());
  EXPECT_TRUE(HasParent(kResourceId, "fake_root"));
  // Should be incremented as a new hosted document was created.
  EXPECT_EQ(old_largest_change_id + 1,
            fake_service_.about_resource().largest_change_id());
  EXPECT_EQ(old_largest_change_id + 1, GetLargestChangeByAboutResource());
}

TEST_F(FakeDriveServiceTest, UpdateResource_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  const std::string kResourceId = "2_file_resource_id";
  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.UpdateResource(
      kResourceId,
      std::string(),
      "new title",
      base::Time(),
      base::Time(),
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
  EXPECT_FALSE(entry);
}

TEST_F(FakeDriveServiceTest, UpdateResource_Forbidden) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  const std::string kResourceId = "2_file_resource_id";
  EXPECT_EQ(HTTP_SUCCESS, fake_service_.SetUserPermission(
      kResourceId, google_apis::drive::PERMISSION_ROLE_READER));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.UpdateResource(
      kResourceId,
      std::string(),
      "new title",
      base::Time(),
      base::Time(),
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_FORBIDDEN, error);
  EXPECT_FALSE(entry);
}

TEST_F(FakeDriveServiceTest, AddResourceToDirectory_FileInRootDirectory) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  int64 old_largest_change_id = GetLargestChangeByAboutResource();

  const std::string kResourceId = "2_file_resource_id";
  const std::string kOldParentResourceId = fake_service_.GetRootResourceId();
  const std::string kNewParentResourceId = "1_folder_resource_id";

  // Here's the original parent link.
  EXPECT_TRUE(HasParent(kResourceId, kOldParentResourceId));
  EXPECT_FALSE(HasParent(kResourceId, kNewParentResourceId));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.AddResourceToDirectory(
      kNewParentResourceId,
      kResourceId,
      test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);

  // The parent link should now be changed.
  EXPECT_TRUE(HasParent(kResourceId, kOldParentResourceId));
  EXPECT_TRUE(HasParent(kResourceId, kNewParentResourceId));
  // Should be incremented as a file was moved.
  EXPECT_EQ(old_largest_change_id + 1,
            fake_service_.about_resource().largest_change_id());
  EXPECT_EQ(old_largest_change_id + 1, GetLargestChangeByAboutResource());
}

TEST_F(FakeDriveServiceTest, AddResourceToDirectory_FileInNonRootDirectory) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  int64 old_largest_change_id = GetLargestChangeByAboutResource();

  const std::string kResourceId = "subdirectory_file_1_id";
  const std::string kOldParentResourceId = "1_folder_resource_id";
  const std::string kNewParentResourceId = "2_folder_resource_id";

  // Here's the original parent link.
  EXPECT_TRUE(HasParent(kResourceId, kOldParentResourceId));
  EXPECT_FALSE(HasParent(kResourceId, kNewParentResourceId));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.AddResourceToDirectory(
      kNewParentResourceId,
      kResourceId,
      test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);

  // The parent link should now be changed.
  EXPECT_TRUE(HasParent(kResourceId, kOldParentResourceId));
  EXPECT_TRUE(HasParent(kResourceId, kNewParentResourceId));
  // Should be incremented as a file was moved.
  EXPECT_EQ(old_largest_change_id + 1,
            fake_service_.about_resource().largest_change_id());
  EXPECT_EQ(old_largest_change_id + 1, GetLargestChangeByAboutResource());
}

TEST_F(FakeDriveServiceTest, AddResourceToDirectory_NonexistingFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  const std::string kResourceId = "nonexisting_file";
  const std::string kNewParentResourceId = "1_folder_resource_id";

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.AddResourceToDirectory(
      kNewParentResourceId,
      kResourceId,
      test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NOT_FOUND, error);
}

TEST_F(FakeDriveServiceTest, AddResourceToDirectory_OrphanFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  int64 old_largest_change_id = GetLargestChangeByAboutResource();

  const std::string kResourceId = "1_orphanfile_resource_id";
  const std::string kNewParentResourceId = "1_folder_resource_id";

  // The file does not belong to any directory, even to the root.
  EXPECT_FALSE(HasParent(kResourceId, kNewParentResourceId));
  EXPECT_FALSE(HasParent(kResourceId, fake_service_.GetRootResourceId()));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.AddResourceToDirectory(
      kNewParentResourceId,
      kResourceId,
      test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);

  // The parent link should now be changed.
  EXPECT_TRUE(HasParent(kResourceId, kNewParentResourceId));
  EXPECT_FALSE(HasParent(kResourceId, fake_service_.GetRootResourceId()));
  // Should be incremented as a file was moved.
  EXPECT_EQ(old_largest_change_id + 1,
            fake_service_.about_resource().largest_change_id());
  EXPECT_EQ(old_largest_change_id + 1, GetLargestChangeByAboutResource());
}

TEST_F(FakeDriveServiceTest, AddResourceToDirectory_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  const std::string kResourceId = "2_file_resource_id";
  const std::string kNewParentResourceId = "1_folder_resource_id";

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.AddResourceToDirectory(
      kNewParentResourceId,
      kResourceId,
      test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
}

TEST_F(FakeDriveServiceTest, RemoveResourceFromDirectory_ExistingFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  int64 old_largest_change_id = GetLargestChangeByAboutResource();

  const std::string kResourceId = "subdirectory_file_1_id";
  const std::string kParentResourceId = "1_folder_resource_id";

  scoped_ptr<FileResource> entry = FindEntry(kResourceId);
  ASSERT_TRUE(entry);
  // The entry should have a parent now.
  ASSERT_FALSE(entry->parents().empty());

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.RemoveResourceFromDirectory(
      kParentResourceId,
      kResourceId,
      test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NO_CONTENT, error);

  entry = FindEntry(kResourceId);
  ASSERT_TRUE(entry);
  // The entry should have no parent now.
  ASSERT_TRUE(entry->parents().empty());
  // Should be incremented as a file was moved to the root directory.
  EXPECT_EQ(old_largest_change_id + 1,
            fake_service_.about_resource().largest_change_id());
  EXPECT_EQ(old_largest_change_id + 1, GetLargestChangeByAboutResource());
}

TEST_F(FakeDriveServiceTest, RemoveResourceFromDirectory_NonexistingFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  const std::string kResourceId = "nonexisting_file";
  const std::string kParentResourceId = "1_folder_resource_id";

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.RemoveResourceFromDirectory(
      kParentResourceId,
      kResourceId,
      test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NOT_FOUND, error);
}

TEST_F(FakeDriveServiceTest, RemoveResourceFromDirectory_OrphanFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  const std::string kResourceId = "1_orphanfile_resource_id";
  const std::string kParentResourceId = fake_service_.GetRootResourceId();

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.RemoveResourceFromDirectory(
      kParentResourceId,
      kResourceId,
      test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NOT_FOUND, error);
}

TEST_F(FakeDriveServiceTest, RemoveResourceFromDirectory_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  const std::string kResourceId = "subdirectory_file_1_id";
  const std::string kParentResourceId = "1_folder_resource_id";

  GDataErrorCode error = GDATA_OTHER_ERROR;
  fake_service_.RemoveResourceFromDirectory(
      kParentResourceId,
      kResourceId,
      test_util::CreateCopyResultCallback(&error));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
}

TEST_F(FakeDriveServiceTest, AddNewDirectory_EmptyParent) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  int64 old_largest_change_id = GetLargestChangeByAboutResource();

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.AddNewDirectory(
      std::string(),
      "new directory",
      DriveServiceInterface::AddNewDirectoryOptions(),
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_CREATED, error);
  ASSERT_TRUE(entry);
  EXPECT_TRUE(entry->IsDirectory());
  EXPECT_EQ("resource_id_1", entry->file_id());
  EXPECT_EQ("new directory", entry->title());
  EXPECT_TRUE(HasParent(entry->file_id(), fake_service_.GetRootResourceId()));
  // Should be incremented as a new directory was created.
  EXPECT_EQ(old_largest_change_id + 1,
            fake_service_.about_resource().largest_change_id());
  EXPECT_EQ(old_largest_change_id + 1, GetLargestChangeByAboutResource());
}

TEST_F(FakeDriveServiceTest, AddNewDirectory_ToRootDirectory) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  int64 old_largest_change_id = GetLargestChangeByAboutResource();

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.AddNewDirectory(
      fake_service_.GetRootResourceId(),
      "new directory",
      DriveServiceInterface::AddNewDirectoryOptions(),
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_CREATED, error);
  ASSERT_TRUE(entry);
  EXPECT_TRUE(entry->IsDirectory());
  EXPECT_EQ("resource_id_1", entry->file_id());
  EXPECT_EQ("new directory", entry->title());
  EXPECT_TRUE(HasParent(entry->file_id(), fake_service_.GetRootResourceId()));
  // Should be incremented as a new directory was created.
  EXPECT_EQ(old_largest_change_id + 1,
            fake_service_.about_resource().largest_change_id());
  EXPECT_EQ(old_largest_change_id + 1, GetLargestChangeByAboutResource());
}

TEST_F(FakeDriveServiceTest, AddNewDirectory_ToRootDirectoryOnEmptyFileSystem) {
  int64 old_largest_change_id = GetLargestChangeByAboutResource();

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.AddNewDirectory(
      fake_service_.GetRootResourceId(),
      "new directory",
      DriveServiceInterface::AddNewDirectoryOptions(),
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_CREATED, error);
  ASSERT_TRUE(entry);
  EXPECT_TRUE(entry->IsDirectory());
  EXPECT_EQ("resource_id_1", entry->file_id());
  EXPECT_EQ("new directory", entry->title());
  EXPECT_TRUE(HasParent(entry->file_id(), fake_service_.GetRootResourceId()));
  // Should be incremented as a new directory was created.
  EXPECT_EQ(old_largest_change_id + 1,
            fake_service_.about_resource().largest_change_id());
  EXPECT_EQ(old_largest_change_id + 1, GetLargestChangeByAboutResource());
}

TEST_F(FakeDriveServiceTest, AddNewDirectory_ToNonRootDirectory) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  int64 old_largest_change_id = GetLargestChangeByAboutResource();

  const std::string kParentResourceId = "1_folder_resource_id";

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.AddNewDirectory(
      kParentResourceId,
      "new directory",
      DriveServiceInterface::AddNewDirectoryOptions(),
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_CREATED, error);
  ASSERT_TRUE(entry);
  EXPECT_TRUE(entry->IsDirectory());
  EXPECT_EQ("resource_id_1", entry->file_id());
  EXPECT_EQ("new directory", entry->title());
  EXPECT_TRUE(HasParent(entry->file_id(), kParentResourceId));
  // Should be incremented as a new directory was created.
  EXPECT_EQ(old_largest_change_id + 1,
            fake_service_.about_resource().largest_change_id());
  EXPECT_EQ(old_largest_change_id + 1, GetLargestChangeByAboutResource());
}

TEST_F(FakeDriveServiceTest, AddNewDirectory_ToNonexistingDirectory) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  const std::string kParentResourceId = "nonexisting_resource_id";

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.AddNewDirectory(
      kParentResourceId,
      "new directory",
      DriveServiceInterface::AddNewDirectoryOptions(),
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NOT_FOUND, error);
  EXPECT_FALSE(entry);
}

TEST_F(FakeDriveServiceTest, AddNewDirectory_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.AddNewDirectory(
      fake_service_.GetRootResourceId(),
      "new directory",
      DriveServiceInterface::AddNewDirectoryOptions(),
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
  EXPECT_FALSE(entry);
}

TEST_F(FakeDriveServiceTest, InitiateUploadNewFile_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  GDataErrorCode error = GDATA_OTHER_ERROR;
  GURL upload_location;
  fake_service_.InitiateUploadNewFile(
      "test/foo",
      13,
      "1_folder_resource_id",
      "new file.foo",
      FakeDriveService::InitiateUploadNewFileOptions(),
      test_util::CreateCopyResultCallback(&error, &upload_location));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
  EXPECT_TRUE(upload_location.is_empty());
}

TEST_F(FakeDriveServiceTest, InitiateUploadNewFile_NotFound) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  GURL upload_location;
  fake_service_.InitiateUploadNewFile(
      "test/foo",
      13,
      "non_existent",
      "new file.foo",
      FakeDriveService::InitiateUploadNewFileOptions(),
      test_util::CreateCopyResultCallback(&error, &upload_location));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NOT_FOUND, error);
  EXPECT_TRUE(upload_location.is_empty());
}

TEST_F(FakeDriveServiceTest, InitiateUploadNewFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  GURL upload_location;
  fake_service_.InitiateUploadNewFile(
      "test/foo",
      13,
      "1_folder_resource_id",
      "new file.foo",
      FakeDriveService::InitiateUploadNewFileOptions(),
      test_util::CreateCopyResultCallback(&error, &upload_location));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  EXPECT_FALSE(upload_location.is_empty());
  EXPECT_NE(GURL("https://1_folder_resumable_create_media_link?mode=newfile"),
            upload_location);
}

TEST_F(FakeDriveServiceTest, InitiateUploadExistingFile_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  GDataErrorCode error = GDATA_OTHER_ERROR;
  GURL upload_location;
  fake_service_.InitiateUploadExistingFile(
      "test/foo",
      13,
      "2_file_resource_id",
      FakeDriveService::InitiateUploadExistingFileOptions(),
      test_util::CreateCopyResultCallback(&error, &upload_location));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
  EXPECT_TRUE(upload_location.is_empty());
}

TEST_F(FakeDriveServiceTest, InitiateUploadExistingFile_Forbidden) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  EXPECT_EQ(HTTP_SUCCESS, fake_service_.SetUserPermission(
      "2_file_resource_id", google_apis::drive::PERMISSION_ROLE_READER));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  GURL upload_location;
  fake_service_.InitiateUploadExistingFile(
      "test/foo",
      13,
      "2_file_resource_id",
      FakeDriveService::InitiateUploadExistingFileOptions(),
      test_util::CreateCopyResultCallback(&error, &upload_location));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_FORBIDDEN, error);
  EXPECT_TRUE(upload_location.is_empty());
}

TEST_F(FakeDriveServiceTest, InitiateUploadExistingFile_NotFound) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  GURL upload_location;
  fake_service_.InitiateUploadExistingFile(
      "test/foo",
      13,
      "non_existent",
      FakeDriveService::InitiateUploadExistingFileOptions(),
      test_util::CreateCopyResultCallback(&error, &upload_location));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NOT_FOUND, error);
  EXPECT_TRUE(upload_location.is_empty());
}

TEST_F(FakeDriveServiceTest, InitiateUploadExistingFile_WrongETag) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  FakeDriveService::InitiateUploadExistingFileOptions options;
  options.etag = "invalid_etag";

  GDataErrorCode error = GDATA_OTHER_ERROR;
  GURL upload_location;
  fake_service_.InitiateUploadExistingFile(
      "text/plain",
      13,
      "2_file_resource_id",
      options,
      test_util::CreateCopyResultCallback(&error, &upload_location));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_PRECONDITION, error);
  EXPECT_TRUE(upload_location.is_empty());
}

TEST_F(FakeDriveServiceTest, InitiateUpload_ExistingFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  scoped_ptr<FileResource> entry = FindEntry("2_file_resource_id");
  ASSERT_TRUE(entry);

  FakeDriveService::InitiateUploadExistingFileOptions options;
  options.etag = entry->etag();

  GDataErrorCode error = GDATA_OTHER_ERROR;
  GURL upload_location;
  fake_service_.InitiateUploadExistingFile(
      "text/plain",
      13,
      "2_file_resource_id",
      options,
      test_util::CreateCopyResultCallback(&error, &upload_location));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  EXPECT_TRUE(upload_location.is_valid());
}

TEST_F(FakeDriveServiceTest, ResumeUpload_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  GURL upload_location;
  fake_service_.InitiateUploadNewFile(
      "test/foo",
      15,
      "1_folder_resource_id",
      "new file.foo",
      FakeDriveService::InitiateUploadNewFileOptions(),
      test_util::CreateCopyResultCallback(&error, &upload_location));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  EXPECT_FALSE(upload_location.is_empty());
  EXPECT_NE(GURL("https://1_folder_resumable_create_media_link"),
            upload_location);

  fake_service_.set_offline(true);

  UploadRangeResponse response;
  scoped_ptr<FileResource> entry;
  fake_service_.ResumeUpload(
      upload_location,
      0, 13, 15, "test/foo",
      base::FilePath(),
      test_util::CreateCopyResultCallback(&response, &entry),
      ProgressCallback());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, response.code);
  EXPECT_FALSE(entry.get());
}

TEST_F(FakeDriveServiceTest, ResumeUpload_NotFound) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  GURL upload_location;
  fake_service_.InitiateUploadNewFile(
      "test/foo",
      15,
      "1_folder_resource_id",
      "new file.foo",
      FakeDriveService::InitiateUploadNewFileOptions(),
      test_util::CreateCopyResultCallback(&error, &upload_location));
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(HTTP_SUCCESS, error);

  UploadRangeResponse response;
  scoped_ptr<FileResource> entry;
  fake_service_.ResumeUpload(
      GURL("https://foo.com/"),
      0, 13, 15, "test/foo",
      base::FilePath(),
      test_util::CreateCopyResultCallback(&response, &entry),
      ProgressCallback());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NOT_FOUND, response.code);
  EXPECT_FALSE(entry.get());
}

TEST_F(FakeDriveServiceTest, ResumeUpload_ExistingFile) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath local_file_path =
      temp_dir.path().Append(FILE_PATH_LITERAL("File 1.txt"));
  std::string contents("hogefugapiyo");
  ASSERT_TRUE(test_util::WriteStringToFile(local_file_path, contents));

  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  scoped_ptr<FileResource> entry = FindEntry("2_file_resource_id");
  ASSERT_TRUE(entry);

  FakeDriveService::InitiateUploadExistingFileOptions options;
  options.etag = entry->etag();

  GDataErrorCode error = GDATA_OTHER_ERROR;
  GURL upload_location;
  fake_service_.InitiateUploadExistingFile(
      "text/plain",
      contents.size(),
      "2_file_resource_id",
      options,
      test_util::CreateCopyResultCallback(&error, &upload_location));
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(HTTP_SUCCESS, error);

  UploadRangeResponse response;
  entry.reset();
  std::vector<test_util::ProgressInfo> upload_progress_values;
  fake_service_.ResumeUpload(
      upload_location,
      0, contents.size() / 2, contents.size(), "text/plain",
      local_file_path,
      test_util::CreateCopyResultCallback(&response, &entry),
      base::Bind(&test_util::AppendProgressCallbackResult,
                 &upload_progress_values));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_RESUME_INCOMPLETE, response.code);
  EXPECT_FALSE(entry.get());
  ASSERT_TRUE(!upload_progress_values.empty());
  EXPECT_TRUE(base::STLIsSorted(upload_progress_values));
  EXPECT_LE(0, upload_progress_values.front().first);
  EXPECT_GE(static_cast<int64>(contents.size() / 2),
            upload_progress_values.back().first);

  upload_progress_values.clear();
  fake_service_.ResumeUpload(
      upload_location,
      contents.size() / 2, contents.size(), contents.size(), "text/plain",
      local_file_path,
      test_util::CreateCopyResultCallback(&response, &entry),
      base::Bind(&test_util::AppendProgressCallbackResult,
                 &upload_progress_values));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, response.code);
  EXPECT_TRUE(entry.get());
  EXPECT_EQ(static_cast<int64>(contents.size()), entry->file_size());
  EXPECT_TRUE(Exists(entry->file_id()));
  ASSERT_TRUE(!upload_progress_values.empty());
  EXPECT_TRUE(base::STLIsSorted(upload_progress_values));
  EXPECT_LE(0, upload_progress_values.front().first);
  EXPECT_GE(static_cast<int64>(contents.size() - contents.size() / 2),
            upload_progress_values.back().first);
  EXPECT_EQ(base::MD5String(contents), entry->md5_checksum());
}

TEST_F(FakeDriveServiceTest, ResumeUpload_NewFile) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath local_file_path =
      temp_dir.path().Append(FILE_PATH_LITERAL("new file.foo"));
  std::string contents("hogefugapiyo");
  ASSERT_TRUE(test_util::WriteStringToFile(local_file_path, contents));

  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  GURL upload_location;
  fake_service_.InitiateUploadNewFile(
      "test/foo",
      contents.size(),
      "1_folder_resource_id",
      "new file.foo",
      FakeDriveService::InitiateUploadNewFileOptions(),
      test_util::CreateCopyResultCallback(&error, &upload_location));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  EXPECT_FALSE(upload_location.is_empty());
  EXPECT_NE(GURL("https://1_folder_resumable_create_media_link"),
            upload_location);

  UploadRangeResponse response;
  scoped_ptr<FileResource> entry;
  std::vector<test_util::ProgressInfo> upload_progress_values;
  fake_service_.ResumeUpload(
      upload_location,
      0, contents.size() / 2, contents.size(), "test/foo",
      local_file_path,
      test_util::CreateCopyResultCallback(&response, &entry),
      base::Bind(&test_util::AppendProgressCallbackResult,
                 &upload_progress_values));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_RESUME_INCOMPLETE, response.code);
  EXPECT_FALSE(entry.get());
  ASSERT_TRUE(!upload_progress_values.empty());
  EXPECT_TRUE(base::STLIsSorted(upload_progress_values));
  EXPECT_LE(0, upload_progress_values.front().first);
  EXPECT_GE(static_cast<int64>(contents.size() / 2),
            upload_progress_values.back().first);

  upload_progress_values.clear();
  fake_service_.ResumeUpload(
      upload_location,
      contents.size() / 2, contents.size(), contents.size(), "test/foo",
      local_file_path,
      test_util::CreateCopyResultCallback(&response, &entry),
      base::Bind(&test_util::AppendProgressCallbackResult,
                 &upload_progress_values));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_CREATED, response.code);
  EXPECT_TRUE(entry.get());
  EXPECT_EQ(static_cast<int64>(contents.size()), entry->file_size());
  EXPECT_TRUE(Exists(entry->file_id()));
  ASSERT_TRUE(!upload_progress_values.empty());
  EXPECT_TRUE(base::STLIsSorted(upload_progress_values));
  EXPECT_LE(0, upload_progress_values.front().first);
  EXPECT_GE(static_cast<int64>(contents.size() - contents.size() / 2),
            upload_progress_values.back().first);
  EXPECT_EQ(base::MD5String(contents), entry->md5_checksum());
}

TEST_F(FakeDriveServiceTest, AddNewFile_ToRootDirectory) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  int64 old_largest_change_id = GetLargestChangeByAboutResource();

  const std::string kContentType = "text/plain";
  const std::string kContentData = "This is some test content.";
  const std::string kTitle = "new file";

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.AddNewFile(
      kContentType,
      kContentData,
      fake_service_.GetRootResourceId(),
      kTitle,
      false,  // shared_with_me
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_CREATED, error);
  ASSERT_TRUE(entry);
  EXPECT_EQ(kContentType, entry->mime_type());
  EXPECT_EQ(static_cast<int64>(kContentData.size()), entry->file_size());
  EXPECT_EQ("resource_id_1", entry->file_id());
  EXPECT_EQ(kTitle, entry->title());
  EXPECT_TRUE(HasParent(entry->file_id(), fake_service_.GetRootResourceId()));
  // Should be incremented as a new directory was created.
  EXPECT_EQ(old_largest_change_id + 1,
            fake_service_.about_resource().largest_change_id());
  EXPECT_EQ(old_largest_change_id + 1, GetLargestChangeByAboutResource());
  EXPECT_EQ(base::MD5String(kContentData), entry->md5_checksum());
}

TEST_F(FakeDriveServiceTest, AddNewFile_ToRootDirectoryOnEmptyFileSystem) {
  int64 old_largest_change_id = GetLargestChangeByAboutResource();

  const std::string kContentType = "text/plain";
  const std::string kContentData = "This is some test content.";
  const std::string kTitle = "new file";

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.AddNewFile(
      kContentType,
      kContentData,
      fake_service_.GetRootResourceId(),
      kTitle,
      false,  // shared_with_me
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_CREATED, error);
  ASSERT_TRUE(entry);
  EXPECT_EQ(kContentType, entry->mime_type());
  EXPECT_EQ(static_cast<int64>(kContentData.size()), entry->file_size());
  EXPECT_EQ("resource_id_1", entry->file_id());
  EXPECT_EQ(kTitle, entry->title());
  EXPECT_TRUE(HasParent(entry->file_id(), fake_service_.GetRootResourceId()));
  // Should be incremented as a new directory was created.
  EXPECT_EQ(old_largest_change_id + 1,
            fake_service_.about_resource().largest_change_id());
  EXPECT_EQ(old_largest_change_id + 1, GetLargestChangeByAboutResource());
  EXPECT_EQ(base::MD5String(kContentData), entry->md5_checksum());
}

TEST_F(FakeDriveServiceTest, AddNewFile_ToNonRootDirectory) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  int64 old_largest_change_id = GetLargestChangeByAboutResource();

  const std::string kContentType = "text/plain";
  const std::string kContentData = "This is some test content.";
  const std::string kTitle = "new file";
  const std::string kParentResourceId = "1_folder_resource_id";

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.AddNewFile(
      kContentType,
      kContentData,
      kParentResourceId,
      kTitle,
      false,  // shared_with_me
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_CREATED, error);
  ASSERT_TRUE(entry);
  EXPECT_EQ(kContentType, entry->mime_type());
  EXPECT_EQ(static_cast<int64>(kContentData.size()), entry->file_size());
  EXPECT_EQ("resource_id_1", entry->file_id());
  EXPECT_EQ(kTitle, entry->title());
  EXPECT_TRUE(HasParent(entry->file_id(), kParentResourceId));
  // Should be incremented as a new directory was created.
  EXPECT_EQ(old_largest_change_id + 1,
            fake_service_.about_resource().largest_change_id());
  EXPECT_EQ(old_largest_change_id + 1, GetLargestChangeByAboutResource());
  EXPECT_EQ(base::MD5String(kContentData), entry->md5_checksum());
}

TEST_F(FakeDriveServiceTest, AddNewFile_ToNonexistingDirectory) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  const std::string kContentType = "text/plain";
  const std::string kContentData = "This is some test content.";
  const std::string kTitle = "new file";
  const std::string kParentResourceId = "nonexisting_resource_id";

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.AddNewFile(
      kContentType,
      kContentData,
      kParentResourceId,
      kTitle,
      false,  // shared_with_me
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NOT_FOUND, error);
  EXPECT_FALSE(entry);
}

TEST_F(FakeDriveServiceTest, AddNewFile_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  const std::string kContentType = "text/plain";
  const std::string kContentData = "This is some test content.";
  const std::string kTitle = "new file";

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.AddNewFile(
      kContentType,
      kContentData,
      fake_service_.GetRootResourceId(),
      kTitle,
      false,  // shared_with_me
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
  EXPECT_FALSE(entry);
}

TEST_F(FakeDriveServiceTest, AddNewFile_SharedWithMeLabel) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  const std::string kContentType = "text/plain";
  const std::string kContentData = "This is some test content.";
  const std::string kTitle = "new file";

  int64 old_largest_change_id = GetLargestChangeByAboutResource();

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.AddNewFile(
      kContentType,
      kContentData,
      fake_service_.GetRootResourceId(),
      kTitle,
      true,  // shared_with_me
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_CREATED, error);
  ASSERT_TRUE(entry);
  EXPECT_EQ(kContentType, entry->mime_type());
  EXPECT_EQ(static_cast<int64>(kContentData.size()), entry->file_size());
  EXPECT_EQ("resource_id_1", entry->file_id());
  EXPECT_EQ(kTitle, entry->title());
  EXPECT_TRUE(HasParent(entry->file_id(), fake_service_.GetRootResourceId()));
  EXPECT_FALSE(entry->shared_with_me_date().is_null());
  // Should be incremented as a new directory was created.
  EXPECT_EQ(old_largest_change_id + 1,
            fake_service_.about_resource().largest_change_id());
  EXPECT_EQ(old_largest_change_id + 1, GetLargestChangeByAboutResource());
  EXPECT_EQ(base::MD5String(kContentData), entry->md5_checksum());
}

TEST_F(FakeDriveServiceTest, SetLastModifiedTime_ExistingFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  const std::string kResourceId = "2_file_resource_id";
  base::Time time;
  ASSERT_TRUE(base::Time::FromString("1 April 2013 12:34:56", &time));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.SetLastModifiedTime(
      kResourceId,
      time,
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_SUCCESS, error);
  ASSERT_TRUE(entry);
  EXPECT_EQ(time, entry->modified_date());
}

TEST_F(FakeDriveServiceTest, SetLastModifiedTime_NonexistingFile) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));

  const std::string kResourceId = "nonexisting_resource_id";
  base::Time time;
  ASSERT_TRUE(base::Time::FromString("1 April 2013 12:34:56", &time));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.SetLastModifiedTime(
      kResourceId,
      time,
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(HTTP_NOT_FOUND, error);
  EXPECT_FALSE(entry);
}

TEST_F(FakeDriveServiceTest, SetLastModifiedTime_Offline) {
  ASSERT_TRUE(test_util::SetUpTestEntries(&fake_service_));
  fake_service_.set_offline(true);

  const std::string kResourceId = "2_file_resource_id";
  base::Time time;
  ASSERT_TRUE(base::Time::FromString("1 April 2013 12:34:56", &time));

  GDataErrorCode error = GDATA_OTHER_ERROR;
  scoped_ptr<FileResource> entry;
  fake_service_.SetLastModifiedTime(
      kResourceId,
      time,
      test_util::CreateCopyResultCallback(&error, &entry));
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(GDATA_NO_CONNECTION, error);
  EXPECT_FALSE(entry);
}

}  // namespace

}  // namespace drive
