// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_BROWSER_FILEAPI_FILE_SYSTEM_FILE_UTIL_H_
#define WEBKIT_BROWSER_FILEAPI_FILE_SYSTEM_FILE_UTIL_H_

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "webkit/browser/fileapi/file_system_operation.h"
#include "webkit/browser/storage_browser_export.h"
#include "webkit/common/blob/scoped_file.h"

namespace base {
class Time;
}

namespace storage {

class FileSystemOperationContext;
class FileSystemURL;

// A file utility interface that provides basic file utility methods for
// FileSystem API.
//
// Layering structure of the FileSystemFileUtil was split out.
// See http://crbug.com/128136 if you need it.
class STORAGE_EXPORT FileSystemFileUtil {
 public:
  typedef FileSystemOperation::CopyOrMoveOption CopyOrMoveOption;

  // It will be implemented by each subclass such as FileSystemFileEnumerator.
  class STORAGE_EXPORT AbstractFileEnumerator {
   public:
    virtual ~AbstractFileEnumerator() {}

    // Returns an empty string if there are no more results.
    virtual base::FilePath Next() = 0;

    // These methods return metadata for the file most recently returned by
    // Next(). If Next() has never been called, or if Next() most recently
    // returned an empty string, then return the default values of 0,
    // "null time", and false, respectively.
    virtual int64 Size() = 0;
    virtual base::Time LastModifiedTime() = 0;
    virtual bool IsDirectory() = 0;
  };

  class STORAGE_EXPORT EmptyFileEnumerator
      : public AbstractFileEnumerator {
    virtual base::FilePath Next() OVERRIDE;
    virtual int64 Size() OVERRIDE;
    virtual base::Time LastModifiedTime() OVERRIDE;
    virtual bool IsDirectory() OVERRIDE;
  };

  virtual ~FileSystemFileUtil() {}

  // Creates or opens a file with the given flags.
  // See header comments for AsyncFileUtil::CreateOrOpen() for more details.
  // This is used only by Pepper/NaCl File API.
  virtual base::File CreateOrOpen(
      FileSystemOperationContext* context,
      const FileSystemURL& url,
      int file_flags) = 0;

  // Ensures that the given |url| exist.  This creates a empty new file
  // at |url| if the |url| does not exist.
  // See header comments for AsyncFileUtil::EnsureFileExists() for more details.
  virtual base::File::Error EnsureFileExists(
      FileSystemOperationContext* context,
      const FileSystemURL& url, bool* created) = 0;

  // Creates directory at given url.
  // See header comments for AsyncFileUtil::CreateDirectory() for more details.
  virtual base::File::Error CreateDirectory(
      FileSystemOperationContext* context,
      const FileSystemURL& url,
      bool exclusive,
      bool recursive) = 0;

  // Retrieves the information about a file.
  // See header comments for AsyncFileUtil::GetFileInfo() for more details.
  virtual base::File::Error GetFileInfo(
      FileSystemOperationContext* context,
      const FileSystemURL& url,
      base::File::Info* file_info,
      base::FilePath* platform_path) = 0;

  // Returns a pointer to a new instance of AbstractFileEnumerator which is
  // implemented for each FileSystemFileUtil subclass. The instance needs to be
  // freed by the caller, and its lifetime should not extend past when the
  // current call returns to the main FILE message loop.
  //
  // The supplied context must remain valid at least lifetime of the enumerator
  // instance.
  virtual scoped_ptr<AbstractFileEnumerator> CreateFileEnumerator(
      FileSystemOperationContext* context,
      const FileSystemURL& root_url) = 0;

  // Maps |file_system_url| given |context| into |local_file_path|
  // which represents physical file location on the host OS.
  // This may not always make sense for all subclasses.
  virtual base::File::Error GetLocalFilePath(
      FileSystemOperationContext* context,
      const FileSystemURL& file_system_url,
      base::FilePath* local_file_path) = 0;

  // Updates the file metadata information.
  // See header comments for AsyncFileUtil::Touch() for more details.
  virtual base::File::Error Touch(
      FileSystemOperationContext* context,
      const FileSystemURL& url,
      const base::Time& last_access_time,
      const base::Time& last_modified_time) = 0;

  // Truncates a file to the given length.
  // See header comments for AsyncFileUtil::Truncate() for more details.
  virtual base::File::Error Truncate(
      FileSystemOperationContext* context,
      const FileSystemURL& url,
      int64 length) = 0;

  // Copies or moves a single file from |src_url| to |dest_url|.
  // The filesystem type of |src_url| and |dest_url| MUST be same.
  // For |option|, please see file_system_operation.h
  //
  // This returns:
  // - File::FILE_ERROR_NOT_FOUND if |src_url|
  //   or the parent directory of |dest_url| does not exist.
  // - File::FILE_ERROR_NOT_A_FILE if |src_url| exists but is not a file.
  // - File::FILE_ERROR_INVALID_OPERATION if |dest_url| exists and
  //   is not a file.
  // - File::FILE_ERROR_FAILED if |dest_url| does not exist and
  //   its parent path is a file.
  //
  virtual base::File::Error CopyOrMoveFile(
      FileSystemOperationContext* context,
      const FileSystemURL& src_url,
      const FileSystemURL& dest_url,
      CopyOrMoveOption option,
      bool copy) = 0;

  // Copies in a single file from a different filesystem.
  // See header comments for AsyncFileUtil::CopyInForeignFile() for
  // more details.
  virtual base::File::Error CopyInForeignFile(
        FileSystemOperationContext* context,
        const base::FilePath& src_file_path,
        const FileSystemURL& dest_url) = 0;

  // Deletes a single file.
  // See header comments for AsyncFileUtil::DeleteFile() for more details.
  virtual base::File::Error DeleteFile(
      FileSystemOperationContext* context,
      const FileSystemURL& url) = 0;

  // Deletes a single empty directory.
  // See header comments for AsyncFileUtil::DeleteDirectory() for more details.
  virtual base::File::Error DeleteDirectory(
      FileSystemOperationContext* context,
      const FileSystemURL& url) = 0;

  // Creates a local snapshot file for a given |url| and returns the
  // metadata and platform path of the snapshot file via |callback|.
  //
  // See header comments for AsyncFileUtil::CreateSnapshotFile() for
  // more details.
  virtual storage::ScopedFile CreateSnapshotFile(
      FileSystemOperationContext* context,
      const FileSystemURL& url,
      base::File::Error* error,
      base::File::Info* file_info,
      base::FilePath* platform_path) = 0;

 protected:
  FileSystemFileUtil() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FileSystemFileUtil);
};

}  // namespace storage

#endif  // WEBKIT_BROWSER_FILEAPI_FILE_SYSTEM_FILE_UTIL_H_
