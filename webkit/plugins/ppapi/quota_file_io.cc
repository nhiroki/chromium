// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/plugins/ppapi/quota_file_io.h"

#include "base/stl_util.h"
#include "base/message_loop_proxy.h"
#include "base/task.h"
#include "webkit/plugins/ppapi/ppapi_plugin_instance.h"

using base::PlatformFile;
using base::PlatformFileError;
using quota::StorageType;

namespace webkit {
namespace ppapi {

namespace {
StorageType PPFileSystemTypeToQuotaStorageType(PP_FileSystemType type) {
  switch (type) {
    case PP_FILESYSTEMTYPE_LOCALPERSISTENT:
      return quota::kStorageTypePersistent;
    case PP_FILESYSTEMTYPE_LOCALTEMPORARY:
      return quota::kStorageTypeTemporary;
    default:
      return quota::kStorageTypeUnknown;
  }
  NOTREACHED();
}
}  // namespace

class QuotaFileIO::PendingOperationBase {
 public:
  virtual ~PendingOperationBase() {}

  // Either one of Run() or DidFail() is called (the latter is called when
  // there was more than one error during quota queries).
  virtual void Run() = 0;
  virtual void DidFail(PlatformFileError error) = 0;

 protected:
  PendingOperationBase(QuotaFileIO* quota_io, bool is_will_operation)
      : quota_io_(quota_io), is_will_operation_(is_will_operation) {
    DCHECK(quota_io_);
    quota_io_->WillUpdate();
  }

  QuotaFileIO* quota_io_;
  const bool is_will_operation_;
};

class QuotaFileIO::WriteOperation : public PendingOperationBase {
 public:
  WriteOperation(QuotaFileIO* quota_io,
                 bool is_will_operation,
                 int64_t offset,
                 const char* buffer,
                 int32_t bytes_to_write,
                 WriteCallback* callback)
      : PendingOperationBase(quota_io, is_will_operation),
        offset_(offset),
        bytes_to_write_(bytes_to_write),
        callback_(callback),
        finished_(false),
        status_(base::PLATFORM_FILE_OK),
        bytes_written_(0),
        callback_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)),
        runnable_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)) {
    if (!is_will_operation) {
      // TODO(kinuko): check the API convention if we really need to keep a
      // copy of the buffer during the async write operations.
      buffer_.reset(new char[bytes_to_write]);
      memcpy(buffer_.get(), buffer, bytes_to_write);
    }
  }
  virtual ~WriteOperation() {}
  virtual void Run() OVERRIDE {
    DCHECK(quota_io_);
    if (quota_io_->CheckIfExceedsQuota(offset_ + bytes_to_write_)) {
      DidFail(base::PLATFORM_FILE_ERROR_NO_SPACE);
      return;
    }
    if (is_will_operation_) {
      // Assuming the write will succeed.
      base::MessageLoopProxy::CreateForCurrentThread()->PostTask(
          FROM_HERE, runnable_factory_.NewRunnableMethod(
              &WriteOperation::DidFinish,
              base::PLATFORM_FILE_OK, bytes_to_write_));
      return;
    }
    DCHECK(buffer_.get());
    if (!base::FileUtilProxy::Write(
            quota_io_->instance_->delegate()->GetFileThreadMessageLoopProxy(),
            quota_io_->file_, offset_, buffer_.get(), bytes_to_write_,
            callback_factory_.NewCallback(&WriteOperation::DidFinish))) {
      DidFail(base::PLATFORM_FILE_ERROR_FAILED);
      return;
    }
  }

  virtual void DidFail(PlatformFileError error) OVERRIDE {
    base::MessageLoopProxy::CreateForCurrentThread()->PostTask(
        FROM_HERE, runnable_factory_.NewRunnableMethod(
            &WriteOperation::DidFinish, error, 0));
  }

  bool finished() const { return finished_; }

  void RunCallback() {
    DCHECK(callback_.get());
    callback_->Run(status_, bytes_written_);
    callback_.reset();
    delete this;
  }

 private:
  void DidFinish(PlatformFileError status, int bytes_written) {
    finished_ = true;
    status_ = status;
    bytes_written_ = bytes_written;
    int64_t max_offset =
        (status != base::PLATFORM_FILE_OK) ? 0 : offset_ + bytes_written;
    // This may delete itself by calling RunCallback.
    quota_io_->DidWrite(this, max_offset);
  }

  const int64_t offset_;
  scoped_array<char> buffer_;
  const int32_t bytes_to_write_;
  scoped_ptr<WriteCallback> callback_;
  bool finished_;
  PlatformFileError status_;
  int64_t bytes_written_;
  base::ScopedCallbackFactory<QuotaFileIO::WriteOperation> callback_factory_;
  ScopedRunnableMethodFactory<QuotaFileIO::WriteOperation> runnable_factory_;
};

class QuotaFileIO::SetLengthOperation : public PendingOperationBase {
 public:
  SetLengthOperation(QuotaFileIO* quota_io,
                     bool is_will_operation,
                     int64_t length,
                     StatusCallback* callback)
      : PendingOperationBase(quota_io, is_will_operation),
        length_(length),
        callback_(callback),
        callback_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)),
        runnable_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)) {}
  virtual ~SetLengthOperation() {}

  virtual void Run() OVERRIDE {
    DCHECK(quota_io_);
    if (quota_io_->CheckIfExceedsQuota(length_)) {
      DidFail(base::PLATFORM_FILE_ERROR_NO_SPACE);
      return;
    }
    if (is_will_operation_) {
      base::MessageLoopProxy::CreateForCurrentThread()->PostTask(
          FROM_HERE, runnable_factory_.NewRunnableMethod(
              &SetLengthOperation::DidFinish,
              base::PLATFORM_FILE_OK));
      return;
    }
    if (!base::FileUtilProxy::Truncate(
            quota_io_->instance_->delegate()->GetFileThreadMessageLoopProxy(),
            quota_io_->file_, length_,
            callback_factory_.NewCallback(&SetLengthOperation::DidFinish))) {
      DidFail(base::PLATFORM_FILE_ERROR_FAILED);
      return;
    }
  }

  virtual void DidFail(PlatformFileError error) OVERRIDE {
    base::MessageLoopProxy::CreateForCurrentThread()->PostTask(
        FROM_HERE, runnable_factory_.NewRunnableMethod(
            &SetLengthOperation::DidFinish, error));
  }

 private:
  void DidFinish(PlatformFileError status) {
    quota_io_->DidSetLength(status, length_);
    DCHECK(callback_.get());
    callback_->Run(status);
    callback_.reset();
    delete this;
  }

  int64_t length_;
  scoped_ptr<StatusCallback> callback_;
  base::ScopedCallbackFactory<QuotaFileIO::SetLengthOperation>
      callback_factory_;
  ScopedRunnableMethodFactory<QuotaFileIO::SetLengthOperation>
      runnable_factory_;
};

// QuotaFileIO --------------------------------------------------------------

QuotaFileIO::QuotaFileIO(
    PluginInstance* instance,
    PlatformFile file,
    const GURL& file_url,
    PP_FileSystemType type)
    : instance_(instance),
      file_(file),
      file_url_(file_url),
      storage_type_(PPFileSystemTypeToQuotaStorageType(type)),
      cached_file_size_(0),
      cached_available_space_(0),
      outstanding_quota_queries_(0),
      outstanding_errors_(0),
      max_written_offset_(0),
      inflight_operations_(0),
      callback_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)) {
  DCHECK(instance_);
  DCHECK_NE(base::kInvalidPlatformFileValue, file_);
  DCHECK_NE(quota::kStorageTypeUnknown, storage_type_);
}

QuotaFileIO::~QuotaFileIO() {
  // Note that this doesn't dispatch pending callbacks.
  STLDeleteContainerPointers(pending_operations_.begin(),
                             pending_operations_.end());
}

bool QuotaFileIO::Write(
    int64_t offset, const char* buffer, int32_t bytes_to_write,
    WriteCallback* callback) {
  WriteOperation* op = new WriteOperation(
      this, false, offset, buffer, bytes_to_write, callback);
  return RegisterOperationForQuotaChecks(op);
}

bool QuotaFileIO::SetLength(int64_t length, StatusCallback* callback) {
  DCHECK(pending_operations_.empty());
  SetLengthOperation* op = new SetLengthOperation(
      this, false, length, callback);
  return RegisterOperationForQuotaChecks(op);
}

bool QuotaFileIO::WillWrite(
    int64_t offset, int32_t bytes_to_write, WriteCallback* callback) {
  WriteOperation* op = new WriteOperation(
      this, true, offset, NULL, bytes_to_write, callback);
  return RegisterOperationForQuotaChecks(op);
}

bool QuotaFileIO::WillSetLength(int64_t length, StatusCallback* callback) {
  DCHECK(pending_operations_.empty());
  SetLengthOperation* op = new SetLengthOperation(this, true, length, callback);
  return RegisterOperationForQuotaChecks(op);
}

bool QuotaFileIO::RegisterOperationForQuotaChecks(
    PendingOperationBase* op_ptr) {
  scoped_ptr<PendingOperationBase> op(op_ptr);
  if (pending_operations_.empty()) {
    // This is the first pending quota check. Run querying the file size
    // and available space.
    outstanding_quota_queries_ = 0;
    outstanding_errors_ = 0;

    // Query the file size.
    ++outstanding_quota_queries_;
    if (!base::FileUtilProxy::GetFileInfoFromPlatformFile(
            instance_->delegate()->GetFileThreadMessageLoopProxy(), file_,
            callback_factory_.NewCallback(
                &QuotaFileIO::DidQueryInfoForQuota))) {
      // This makes the call fail synchronously; we do not fire the callback
      // here but just delete the operation and return false.
      return false;
    }

    // Query the current available space.
    ++outstanding_quota_queries_;
    instance_->delegate()->QueryAvailableSpace(
        GURL(file_url_.path()).GetOrigin(), storage_type_,
        callback_factory_.NewCallback(&QuotaFileIO::DidQueryAvailableSpace));
  }
  pending_operations_.push_back(op.release());
  return true;
}

void QuotaFileIO::DidQueryInfoForQuota(
    base::PlatformFileError error_code,
    const base::PlatformFileInfo& file_info) {
  if (error_code != base::PLATFORM_FILE_OK)
    ++outstanding_errors_;
  cached_file_size_ = file_info.size;
  DCHECK_GT(outstanding_quota_queries_, 0);
  if (--outstanding_quota_queries_ == 0)
    DidQueryForQuotaCheck();
}

void QuotaFileIO::DidQueryAvailableSpace(int64_t avail_space) {
  cached_available_space_ = avail_space;
  DCHECK_GT(outstanding_quota_queries_, 0);
  if (--outstanding_quota_queries_ == 0)
    DidQueryForQuotaCheck();
}

void QuotaFileIO::DidQueryForQuotaCheck() {
  DCHECK(!pending_operations_.empty());
  DCHECK_GT(inflight_operations_, 0);
  for (std::deque<PendingOperationBase*>::iterator iter =
           pending_operations_.begin();
       iter != pending_operations_.end();
       ++iter) {
    PendingOperationBase* op = *iter;
    if (outstanding_errors_ > 0) {
      op->DidFail(base::PLATFORM_FILE_ERROR_FAILED);
      continue;
    }
    op->Run();
  }
}

bool QuotaFileIO::CheckIfExceedsQuota(int64_t new_file_size) const {
  DCHECK_GE(cached_file_size_, 0);
  DCHECK_GE(cached_available_space_, 0);
  return new_file_size - cached_file_size_ > cached_available_space_;
}

void QuotaFileIO::WillUpdate() {
  if (inflight_operations_++ == 0) {
    instance_->delegate()->WillUpdateFile(file_url_);
    DCHECK_EQ(0, max_written_offset_);
  }
}

void QuotaFileIO::DidWrite(WriteOperation* op,
                           int64_t written_offset_end) {
  max_written_offset_ = std::max(max_written_offset_, written_offset_end);
  DCHECK_GT(inflight_operations_, 0);
  DCHECK(!pending_operations_.empty());
  // Fire callbacks for finished operations.
  while (!pending_operations_.empty()) {
    WriteOperation* op = static_cast<WriteOperation*>(
        pending_operations_.front());
    if (!op->finished())
      break;
    op->RunCallback();
    pending_operations_.pop_front();
  }
  // If we have no more pending writes, notify the browser that we did
  // update the file.
  if (--inflight_operations_ == 0) {
    DCHECK(pending_operations_.empty());
    int64_t growth = max_written_offset_ - cached_file_size_;
    growth = growth < 0 ? 0 : growth;
    instance_->delegate()->DidUpdateFile(file_url_, growth);
    max_written_offset_ = 0;
  }
}

void QuotaFileIO::DidSetLength(PlatformFileError error, int64_t new_file_size) {
  DCHECK_EQ(1, inflight_operations_);
  pending_operations_.pop_front();
  DCHECK(pending_operations_.empty());
  int64_t delta = (error != base::PLATFORM_FILE_OK) ? 0 :
      new_file_size - cached_file_size_;
  instance_->delegate()->DidUpdateFile(file_url_, delta);
  inflight_operations_ = 0;
}

}  // namespace ppapi
}  // namespace webkit
