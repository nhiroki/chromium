// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/fileapi/sandbox_file_stream_writer.h"

#include "base/file_util_proxy.h"
#include "base/platform_file.h"
#include "base/sequenced_task_runner.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "webkit/blob/local_file_stream_reader.h"
#include "webkit/fileapi/file_observers.h"
#include "webkit/fileapi/file_system_context.h"
#include "webkit/fileapi/file_system_operation.h"
#include "webkit/fileapi/file_system_util.h"
#include "webkit/fileapi/local_file_stream_writer.h"
#include "webkit/quota/quota_manager.h"

namespace fileapi {

namespace {

// Adjust the |quota| value in overwriting case (i.e. |file_size| > 0 and
// |file_offset| < |file_size|) to make the remaining quota calculation easier.
// Specifically this widens the quota for overlapping range (so that we can
// simply compare written bytes against the adjusted quota).
int64 AdjustQuotaForOverlap(int64 quota,
                            int64 file_offset,
                            int64 file_size) {
  DCHECK_LE(file_offset, file_size);
  if (quota < 0)
    quota = 0;
  int64 overlap = file_size - file_offset;
  if (kint64max - overlap > quota)
    quota += overlap;
  return quota;
}

}  // namespace

SandboxFileStreamWriter::SandboxFileStreamWriter(
    FileSystemContext* file_system_context,
    const FileSystemURL& url,
    int64 initial_offset,
    const UpdateObserverList& observers)
    : file_system_context_(file_system_context),
      url_(url),
      initial_offset_(initial_offset),
      observers_(observers),
      file_size_(0),
      total_bytes_written_(0),
      allowed_bytes_to_write_(0),
      has_pending_operation_(false),
      default_quota_(kint64max),
      weak_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)) {
  DCHECK(url_.is_valid());
}

SandboxFileStreamWriter::~SandboxFileStreamWriter() {}

int SandboxFileStreamWriter::Write(
    net::IOBuffer* buf, int buf_len,
    const net::CompletionCallback& callback) {
  has_pending_operation_ = true;
  if (local_file_writer_.get())
    return WriteInternal(buf, buf_len, callback);

  base::PlatformFileError error_code;
  FileSystemOperation* operation =
      file_system_context_->CreateFileSystemOperation(url_, &error_code);
  if (error_code != base::PLATFORM_FILE_OK)
    return net::PlatformFileErrorToNetError(error_code);

  DCHECK(operation);
  net::CompletionCallback write_task =
      base::Bind(&SandboxFileStreamWriter::DidInitializeForWrite,
                 weak_factory_.GetWeakPtr(),
                 make_scoped_refptr(buf), buf_len, callback);
  operation->GetMetadata(
      url_, base::Bind(&SandboxFileStreamWriter::DidGetFileInfo,
                       weak_factory_.GetWeakPtr(), write_task));
  return net::ERR_IO_PENDING;
}

int SandboxFileStreamWriter::Cancel(const net::CompletionCallback& callback) {
  if (!has_pending_operation_)
    return net::ERR_UNEXPECTED;

  DCHECK(!callback.is_null());
  cancel_callback_ = callback;
  return net::ERR_IO_PENDING;
}

int SandboxFileStreamWriter::WriteInternal(
    net::IOBuffer* buf, int buf_len,
    const net::CompletionCallback& callback) {
  // allowed_bytes_to_write could be negative if the file size is
  // greater than the current (possibly new) quota.
  DCHECK(total_bytes_written_ <= allowed_bytes_to_write_ ||
         allowed_bytes_to_write_ < 0);
  if (total_bytes_written_ >= allowed_bytes_to_write_) {
    has_pending_operation_ = false;
    return net::ERR_FILE_NO_SPACE;
  }

  if (buf_len > allowed_bytes_to_write_ - total_bytes_written_)
    buf_len = allowed_bytes_to_write_ - total_bytes_written_;

  DCHECK(local_file_writer_.get());
  const int result = local_file_writer_->Write(
      buf, buf_len,
      base::Bind(&SandboxFileStreamWriter::DidWrite, weak_factory_.GetWeakPtr(),
                 callback));
  if (result != net::ERR_IO_PENDING)
    has_pending_operation_ = false;
  return result;
}

void SandboxFileStreamWriter::DidGetFileInfo(
    const net::CompletionCallback& callback,
    base::PlatformFileError file_error,
    const base::PlatformFileInfo& file_info,
    const FilePath& platform_path) {
  if (CancelIfRequested())
    return;
  if (file_error != base::PLATFORM_FILE_OK) {
    callback.Run(net::PlatformFileErrorToNetError(file_error));
    return;
  }
  if (file_info.is_directory) {
    // We should not be writing to a directory.
    callback.Run(net::ERR_ACCESS_DENIED);
    return;
  }
  file_size_ = file_info.size;
  if (initial_offset_ > file_size_) {
    LOG(ERROR) << initial_offset_ << ", " << file_size_;
    // This shouldn't happen as long as we check offset in the renderer.
    NOTREACHED();
    initial_offset_ = file_size_;
  }
  DCHECK(!local_file_writer_.get());
  local_file_writer_.reset(
      new LocalFileStreamWriter(platform_path, initial_offset_));

  quota::QuotaManagerProxy* quota_manager_proxy =
      file_system_context_->quota_manager_proxy();
  if (!quota_manager_proxy) {
    // If we don't have the quota manager or the requested filesystem type
    // does not support quota, we should be able to let it go.
    allowed_bytes_to_write_ = default_quota_;
    callback.Run(net::OK);
    return;
  }

  DCHECK(quota_manager_proxy->quota_manager());
  quota_manager_proxy->quota_manager()->GetUsageAndQuota(
      url_.origin(),
      FileSystemTypeToQuotaStorageType(url_.type()),
      base::Bind(&SandboxFileStreamWriter::DidGetUsageAndQuota,
                 weak_factory_.GetWeakPtr(), callback));
}

void SandboxFileStreamWriter::DidGetUsageAndQuota(
    const net::CompletionCallback& callback,
    quota::QuotaStatusCode status,
    int64 usage, int64 quota) {
  if (CancelIfRequested())
    return;
  if (status != quota::kQuotaStatusOk) {
    LOG(WARNING) << "Got unexpected quota error : " << status;
    callback.Run(net::ERR_FAILED);
    return;
  }

  allowed_bytes_to_write_ = quota - usage;
  callback.Run(net::OK);
}

void SandboxFileStreamWriter::DidInitializeForWrite(
    net::IOBuffer* buf, int buf_len,
    const net::CompletionCallback& callback,
    int init_status) {
  if (CancelIfRequested())
    return;
  if (init_status != net::OK) {
    has_pending_operation_ = false;
    callback.Run(init_status);
    return;
  }
  allowed_bytes_to_write_ = AdjustQuotaForOverlap(
      allowed_bytes_to_write_, initial_offset_, file_size_);
  const int result = WriteInternal(buf, buf_len, callback);
  if (result != net::ERR_IO_PENDING)
    callback.Run(result);
}

void SandboxFileStreamWriter::DidWrite(
    const net::CompletionCallback& callback,
    int write_response) {
  DCHECK(has_pending_operation_);
  has_pending_operation_ = false;

  if (write_response <= 0) {
    if (CancelIfRequested())
      return;
    callback.Run(write_response);
    return;
  }

  if (total_bytes_written_ + write_response + initial_offset_ > file_size_) {
    int overlapped = file_size_ - total_bytes_written_ - initial_offset_;
    if (overlapped < 0)
      overlapped = 0;
    observers_.Notify(&FileUpdateObserver::OnUpdate,
                      MakeTuple(url_, write_response - overlapped));
  }
  total_bytes_written_ += write_response;

  if (CancelIfRequested())
    return;
  callback.Run(write_response);
}

bool SandboxFileStreamWriter::CancelIfRequested() {
  if (cancel_callback_.is_null())
    return false;

  net::CompletionCallback pending_cancel = cancel_callback_;
  has_pending_operation_ = false;
  cancel_callback_.Reset();
  pending_cancel.Run(net::OK);
  return true;
}

int SandboxFileStreamWriter::Flush(const net::CompletionCallback& callback) {
  // For now, Flush is meaningful only for local native file access. It is no-op
  // for sandboxed filesystem files (see the discussion in crbug.com/144790).
  return net::OK;
}

}  // namespace fileapi
