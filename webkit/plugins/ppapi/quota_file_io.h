// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_PLUGINS_PPAPI_QUOTA_FILE_IO_H_
#define WEBKIT_PLUGINS_PPAPI_QUOTA_FILE_IO_H_

#include <deque>

#include "base/file_util_proxy.h"
#include "base/memory/scoped_callback_factory.h"
#include "base/platform_file.h"
#include "googleurl/src/gurl.h"
#include "ppapi/c/pp_file_info.h"
#include "webkit/quota/quota_types.h"

namespace webkit {
namespace ppapi {

class PluginInstance;

// This class is created per PPB_FileIO_Impl instance and provides
// write operations for quota-managed files (i.e. files of
// PP_FILESYSTEMTYPE_LOCAL{PERSISTENT,TEMPORARY}).
class QuotaFileIO {
 public:
  typedef base::FileUtilProxy::WriteCallback WriteCallback;
  typedef base::FileUtilProxy::StatusCallback StatusCallback;

  QuotaFileIO(PluginInstance* instance,
              base::PlatformFile file,
              const GURL& path_url,
              PP_FileSystemType type);
  ~QuotaFileIO();

  // Performs write or setlength operation with quota checks.
  // Returns true when the operation is successfully dispatched.
  // |callback| will be dispatched when the operation completes.
  // Otherwise it returns false and |callback| will not be dispatched.
  // |callback| will not be dispatched either when this instance is
  // destroyed before the operation completes.
  // SetLength/WillSetLength cannot be called while there're any inflight
  // operations.  For Write/WillWrite it is guaranteed that |callback| are
  // always dispatched in the same order as Write being called.
  bool Write(int64_t offset,
             const char* buffer,
             int32_t bytes_to_write,
             WriteCallback* callback);
  bool WillWrite(int64_t offset,
                 int32_t bytes_to_write,
                 WriteCallback* callback);

  bool SetLength(int64_t length, StatusCallback* callback);
  bool WillSetLength(int64_t length, StatusCallback* callback);

 private:
  class PendingOperationBase;
  class WriteOperation;
  class SetLengthOperation;

  bool CheckIfExceedsQuota(int64_t new_file_size) const;
  void WillUpdate();
  void DidWrite(WriteOperation* op, int64_t written_offset_end);
  void DidSetLength(base::PlatformFileError error, int64_t new_file_size);

  bool RegisterOperationForQuotaChecks(PendingOperationBase* op);
  void DidQueryInfoForQuota(base::PlatformFileError error_code,
                            const base::PlatformFileInfo& file_info);
  void DidQueryAvailableSpace(int64_t avail_space);
  void DidQueryForQuotaCheck();

  // The plugin instance that owns this (via PPB_FileIO_Impl).
  PluginInstance* instance_;

  // The file information associated to this instance.
  base::PlatformFile file_;
  GURL file_url_;
  quota::StorageType storage_type_;

  // Operations waiting for a quota check to finish.
  std::deque<PendingOperationBase*> pending_operations_;

  // Valid only while there're pending quota checks.
  int64_t cached_file_size_;
  int64_t cached_available_space_;

  // Quota-related queries and errors occured during inflight quota checks.
  int outstanding_quota_queries_;
  int outstanding_errors_;

  // For parallel writes bookkeeping.
  int64_t max_written_offset_;
  int inflight_operations_;

  base::ScopedCallbackFactory<QuotaFileIO> callback_factory_;
  DISALLOW_COPY_AND_ASSIGN(QuotaFileIO);
};

}  // namespace ppapi
}  // namespace webkit

#endif  // WEBKIT_PLUGINS_PPAPI_QUOTA_FILE_IO_H_
