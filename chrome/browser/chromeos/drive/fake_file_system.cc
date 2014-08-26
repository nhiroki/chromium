// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/drive/fake_file_system.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "chrome/browser/chromeos/drive/drive.pb.h"
#include "chrome/browser/chromeos/drive/file_errors.h"
#include "chrome/browser/chromeos/drive/file_system_util.h"
#include "chrome/browser/chromeos/drive/resource_entry_conversion.h"
#include "chrome/browser/drive/drive_service_interface.h"
#include "content/public/browser/browser_thread.h"
#include "google_apis/drive/drive_api_parser.h"

namespace drive {
namespace test_util {

using content::BrowserThread;

FakeFileSystem::FakeFileSystem(DriveServiceInterface* drive_service)
    : drive_service_(drive_service),
      weak_ptr_factory_(this) {
  CHECK(cache_dir_.CreateUniqueTempDir());
}

FakeFileSystem::~FakeFileSystem() {
}

void FakeFileSystem::AddObserver(FileSystemObserver* observer) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::RemoveObserver(FileSystemObserver* observer) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::CheckForUpdates() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::TransferFileFromLocalToRemote(
    const base::FilePath& local_src_file_path,
    const base::FilePath& remote_dest_file_path,
    const FileOperationCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::OpenFile(const base::FilePath& file_path,
                              OpenMode open_mode,
                              const std::string& mime_type,
                              const OpenFileCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::Copy(const base::FilePath& src_file_path,
                          const base::FilePath& dest_file_path,
                          bool preserve_last_modified,
                          const FileOperationCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::Move(const base::FilePath& src_file_path,
                          const base::FilePath& dest_file_path,
                          const FileOperationCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::Remove(const base::FilePath& file_path,
                            bool is_recursive,
                            const FileOperationCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::CreateDirectory(
    const base::FilePath& directory_path,
    bool is_exclusive,
    bool is_recursive,
    const FileOperationCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::CreateFile(const base::FilePath& file_path,
                                bool is_exclusive,
                                const std::string& mime_type,
                                const FileOperationCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::TouchFile(const base::FilePath& file_path,
                               const base::Time& last_access_time,
                               const base::Time& last_modified_time,
                               const FileOperationCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::TruncateFile(const base::FilePath& file_path,
                                  int64 length,
                                  const FileOperationCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::Pin(const base::FilePath& file_path,
                         const FileOperationCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::Unpin(const base::FilePath& file_path,
                           const FileOperationCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::GetFile(const base::FilePath& file_path,
                             const GetFileCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::GetFileForSaving(const base::FilePath& file_path,
                                      const GetFileCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

base::Closure FakeFileSystem::GetFileContent(
    const base::FilePath& file_path,
    const GetFileContentInitializedCallback& initialized_callback,
    const google_apis::GetContentCallback& get_content_callback,
    const FileOperationCallback& completion_callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  GetResourceEntry(
      file_path,
      base::Bind(&FakeFileSystem::GetFileContentAfterGetResourceEntry,
                 weak_ptr_factory_.GetWeakPtr(),
                 initialized_callback, get_content_callback,
                 completion_callback));
  return base::Bind(&base::DoNothing);
}

void FakeFileSystem::GetResourceEntry(
    const base::FilePath& file_path,
    const GetResourceEntryCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  // Now, we only support files under my drive.
  DCHECK(!util::IsUnderDriveMountPoint(file_path));

  if (file_path == util::GetDriveMyDriveRootPath()) {
    // Specialized for the root entry.
    drive_service_->GetAboutResource(
        base::Bind(
            &FakeFileSystem::GetResourceEntryAfterGetAboutResource,
            weak_ptr_factory_.GetWeakPtr(), callback));
    return;
  }

  GetResourceEntry(
      file_path.DirName(),
      base::Bind(
          &FakeFileSystem::GetResourceEntryAfterGetParentEntryInfo,
          weak_ptr_factory_.GetWeakPtr(), file_path.BaseName(), callback));
}

void FakeFileSystem::ReadDirectory(
    const base::FilePath& file_path,
    const ReadDirectoryEntriesCallback& entries_callback,
    const FileOperationCallback& completion_callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::Search(const std::string& search_query,
                            const GURL& next_link,
                            const SearchCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::SearchMetadata(
    const std::string& query,
    int options,
    int at_most_num_matches,
    const SearchMetadataCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::GetAvailableSpace(
    const GetAvailableSpaceCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::GetShareUrl(
    const base::FilePath& file_path,
    const GURL& embed_origin,
    const GetShareUrlCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::GetMetadata(
    const GetFilesystemMetadataCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::MarkCacheFileAsMounted(
    const base::FilePath& drive_file_path,
    const MarkMountedCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::MarkCacheFileAsUnmounted(
    const base::FilePath& cache_file_path,
    const FileOperationCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::AddPermission(const base::FilePath& drive_file_path,
                                   const std::string& email,
                                   google_apis::drive::PermissionRole role,
                                   const FileOperationCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

void FakeFileSystem::Reset(const FileOperationCallback& callback) {
}

void FakeFileSystem::GetPathFromResourceId(
    const std::string& resource_id,
    const GetFilePathCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

// Implementation of GetFileContent.
void FakeFileSystem::GetFileContentAfterGetResourceEntry(
    const GetFileContentInitializedCallback& initialized_callback,
    const google_apis::GetContentCallback& get_content_callback,
    const FileOperationCallback& completion_callback,
    FileError error,
    scoped_ptr<ResourceEntry> entry) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (error != FILE_ERROR_OK) {
    completion_callback.Run(error);
    return;
  }
  DCHECK(entry);

  // We're only interested in a file.
  if (entry->file_info().is_directory()) {
    completion_callback.Run(FILE_ERROR_NOT_A_FILE);
    return;
  }

  // Fetch google_apis::FileResource for its |download_url|.
  drive_service_->GetFileResource(
      entry->resource_id(),
      base::Bind(
          &FakeFileSystem::GetFileContentAfterGetFileResource,
          weak_ptr_factory_.GetWeakPtr(),
          initialized_callback,
          get_content_callback,
          completion_callback));
}

void FakeFileSystem::GetFileContentAfterGetFileResource(
    const GetFileContentInitializedCallback& initialized_callback,
    const google_apis::GetContentCallback& get_content_callback,
    const FileOperationCallback& completion_callback,
    google_apis::GDataErrorCode gdata_error,
    scoped_ptr<google_apis::FileResource> gdata_entry) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  FileError error = GDataToFileError(gdata_error);
  if (error != FILE_ERROR_OK) {
    completion_callback.Run(error);
    return;
  }
  DCHECK(gdata_entry);

  scoped_ptr<ResourceEntry> entry(new ResourceEntry);
  std::string parent_resource_id;
  bool converted = ConvertFileResourceToResourceEntry(
      *gdata_entry, entry.get(), &parent_resource_id);
  DCHECK(converted);
  entry->set_parent_local_id(parent_resource_id);

  base::FilePath cache_path =
      cache_dir_.path().AppendASCII(entry->resource_id());
  if (entry->file_specific_info().is_hosted_document()) {
    // For hosted documents return a dummy cache without server request.
    int result = base::WriteFile(cache_path, "", 0);
    DCHECK_EQ(0, result);
  }
  if (base::PathExists(cache_path)) {
    // Cache file is found.
    initialized_callback.Run(FILE_ERROR_OK, cache_path, entry.Pass());
    completion_callback.Run(FILE_ERROR_OK);
    return;
  }

  initialized_callback.Run(FILE_ERROR_OK, base::FilePath(), entry.Pass());
  drive_service_->DownloadFile(
      cache_path,
      gdata_entry->file_id(),
      base::Bind(&FakeFileSystem::GetFileContentAfterDownloadFile,
                 weak_ptr_factory_.GetWeakPtr(),
                 completion_callback),
      get_content_callback,
      google_apis::ProgressCallback());
}

void FakeFileSystem::GetFileContentAfterDownloadFile(
    const FileOperationCallback& completion_callback,
    google_apis::GDataErrorCode gdata_error,
    const base::FilePath& temp_file) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  completion_callback.Run(GDataToFileError(gdata_error));
}

// Implementation of GetResourceEntry.
void FakeFileSystem::GetResourceEntryAfterGetAboutResource(
    const GetResourceEntryCallback& callback,
    google_apis::GDataErrorCode gdata_error,
    scoped_ptr<google_apis::AboutResource> about_resource) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  FileError error = GDataToFileError(gdata_error);
  if (error != FILE_ERROR_OK) {
    callback.Run(error, scoped_ptr<ResourceEntry>());
    return;
  }

  DCHECK(about_resource);
  scoped_ptr<ResourceEntry> root(new ResourceEntry);
  root->mutable_file_info()->set_is_directory(true);
  root->set_resource_id(about_resource->root_folder_id());
  root->set_title(util::kDriveMyDriveRootDirName);
  callback.Run(error, root.Pass());
}

void FakeFileSystem::GetResourceEntryAfterGetParentEntryInfo(
    const base::FilePath& base_name,
    const GetResourceEntryCallback& callback,
    FileError error,
    scoped_ptr<ResourceEntry> parent_entry) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (error != FILE_ERROR_OK) {
    callback.Run(error, scoped_ptr<ResourceEntry>());
    return;
  }

  DCHECK(parent_entry);
  drive_service_->GetFileListInDirectory(
      parent_entry->resource_id(),
      base::Bind(
          &FakeFileSystem::GetResourceEntryAfterGetFileList,
          weak_ptr_factory_.GetWeakPtr(), base_name, callback));
}

void FakeFileSystem::GetResourceEntryAfterGetFileList(
    const base::FilePath& base_name,
    const GetResourceEntryCallback& callback,
    google_apis::GDataErrorCode gdata_error,
    scoped_ptr<google_apis::FileList> file_list) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  FileError error = GDataToFileError(gdata_error);
  if (error != FILE_ERROR_OK) {
    callback.Run(error, scoped_ptr<ResourceEntry>());
    return;
  }

  DCHECK(file_list);
  const ScopedVector<google_apis::FileResource>& entries = file_list->items();
  for (size_t i = 0; i < entries.size(); ++i) {
    scoped_ptr<ResourceEntry> entry(new ResourceEntry);
    std::string parent_resource_id;
    bool converted = ConvertFileResourceToResourceEntry(
        *entries[i], entry.get(), &parent_resource_id);
    DCHECK(converted);
    entry->set_parent_local_id(parent_resource_id);

    if (entry->base_name() == base_name.AsUTF8Unsafe()) {
      // Found the target entry.
      callback.Run(FILE_ERROR_OK, entry.Pass());
      return;
    }
  }

  callback.Run(FILE_ERROR_NOT_FOUND, scoped_ptr<ResourceEntry>());
}

}  // namespace test_util
}  // namespace drive
