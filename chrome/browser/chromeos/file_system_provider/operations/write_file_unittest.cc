// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_system_provider/operations/write_file.h"

#include <string>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/values.h"
#include "chrome/browser/chromeos/file_system_provider/operations/test_util.h"
#include "chrome/common/extensions/api/file_system_provider.h"
#include "chrome/common/extensions/api/file_system_provider_internal.h"
#include "extensions/browser/event_router.h"
#include "net/base/io_buffer.h"
#include "storage/browser/fileapi/async_file_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace file_system_provider {
namespace operations {
namespace {

const char kExtensionId[] = "mbflcebpggnecokmikipoihdbecnjfoj";
const char kFileSystemId[] = "testing-file-system";
const int kRequestId = 2;
const int kFileHandle = 3;
const char kWriteData[] = "Welcome to my world!";
const int kOffset = 10;

}  // namespace

class FileSystemProviderOperationsWriteFileTest : public testing::Test {
 protected:
  FileSystemProviderOperationsWriteFileTest() {}
  virtual ~FileSystemProviderOperationsWriteFileTest() {}

  virtual void SetUp() OVERRIDE {
    file_system_info_ =
        ProvidedFileSystemInfo(kExtensionId,
                               kFileSystemId,
                               "" /* display_name */,
                               true /* writable */,
                               base::FilePath() /* mount_path */);
    io_buffer_ = make_scoped_refptr(new net::StringIOBuffer(kWriteData));
  }

  ProvidedFileSystemInfo file_system_info_;
  scoped_refptr<net::StringIOBuffer> io_buffer_;
};

TEST_F(FileSystemProviderOperationsWriteFileTest, Execute) {
  using extensions::api::file_system_provider::WriteFileRequestedOptions;

  util::LoggingDispatchEventImpl dispatcher(true /* dispatch_reply */);
  util::StatusCallbackLog callback_log;

  WriteFile write_file(NULL,
                       file_system_info_,
                       kFileHandle,
                       io_buffer_.get(),
                       kOffset,
                       io_buffer_->size(),
                       base::Bind(&util::LogStatusCallback, &callback_log));
  write_file.SetDispatchEventImplForTesting(
      base::Bind(&util::LoggingDispatchEventImpl::OnDispatchEventImpl,
                 base::Unretained(&dispatcher)));

  EXPECT_TRUE(write_file.Execute(kRequestId));

  ASSERT_EQ(1u, dispatcher.events().size());
  extensions::Event* event = dispatcher.events()[0];
  EXPECT_EQ(
      extensions::api::file_system_provider::OnWriteFileRequested::kEventName,
      event->event_name);
  base::ListValue* event_args = event->event_args.get();
  ASSERT_EQ(1u, event_args->GetSize());

  const base::DictionaryValue* options_as_value = NULL;
  ASSERT_TRUE(event_args->GetDictionary(0, &options_as_value));

  WriteFileRequestedOptions options;
  ASSERT_TRUE(WriteFileRequestedOptions::Populate(*options_as_value, &options));
  EXPECT_EQ(kFileSystemId, options.file_system_id);
  EXPECT_EQ(kRequestId, options.request_id);
  EXPECT_EQ(kFileHandle, options.open_request_id);
  EXPECT_EQ(kOffset, static_cast<double>(options.offset));
  EXPECT_EQ(std::string(kWriteData), options.data);
}

TEST_F(FileSystemProviderOperationsWriteFileTest, Execute_NoListener) {
  util::LoggingDispatchEventImpl dispatcher(false /* dispatch_reply */);
  util::StatusCallbackLog callback_log;

  WriteFile write_file(NULL,
                       file_system_info_,
                       kFileHandle,
                       io_buffer_.get(),
                       kOffset,
                       io_buffer_->size(),
                       base::Bind(&util::LogStatusCallback, &callback_log));
  write_file.SetDispatchEventImplForTesting(
      base::Bind(&util::LoggingDispatchEventImpl::OnDispatchEventImpl,
                 base::Unretained(&dispatcher)));

  EXPECT_FALSE(write_file.Execute(kRequestId));
}

TEST_F(FileSystemProviderOperationsWriteFileTest, Execute_ReadOnly) {
  util::LoggingDispatchEventImpl dispatcher(true /* dispatch_reply */);
  util::StatusCallbackLog callback_log;

  const ProvidedFileSystemInfo read_only_file_system_info(
        kExtensionId,
        kFileSystemId,
        "" /* file_system_name */,
        false /* writable */,
        base::FilePath() /* mount_path */);

  WriteFile write_file(NULL,
                       read_only_file_system_info,
                       kFileHandle,
                       io_buffer_.get(),
                       kOffset,
                       io_buffer_->size(),
                       base::Bind(&util::LogStatusCallback, &callback_log));
  write_file.SetDispatchEventImplForTesting(
      base::Bind(&util::LoggingDispatchEventImpl::OnDispatchEventImpl,
                 base::Unretained(&dispatcher)));

  EXPECT_FALSE(write_file.Execute(kRequestId));
}

TEST_F(FileSystemProviderOperationsWriteFileTest, OnSuccess) {
  util::LoggingDispatchEventImpl dispatcher(true /* dispatch_reply */);
  util::StatusCallbackLog callback_log;

  WriteFile write_file(NULL,
                       file_system_info_,
                       kFileHandle,
                       io_buffer_.get(),
                       kOffset,
                       io_buffer_->size(),
                       base::Bind(&util::LogStatusCallback, &callback_log));
  write_file.SetDispatchEventImplForTesting(
      base::Bind(&util::LoggingDispatchEventImpl::OnDispatchEventImpl,
                 base::Unretained(&dispatcher)));

  EXPECT_TRUE(write_file.Execute(kRequestId));

  write_file.OnSuccess(kRequestId,
                       scoped_ptr<RequestValue>(new RequestValue()),
                       false /* has_more */);
  ASSERT_EQ(1u, callback_log.size());
  EXPECT_EQ(base::File::FILE_OK, callback_log[0]);
}

TEST_F(FileSystemProviderOperationsWriteFileTest, OnError) {
  util::LoggingDispatchEventImpl dispatcher(true /* dispatch_reply */);
  util::StatusCallbackLog callback_log;

  WriteFile write_file(NULL,
                       file_system_info_,
                       kFileHandle,
                       io_buffer_.get(),
                       kOffset,
                       io_buffer_->size(),
                       base::Bind(&util::LogStatusCallback, &callback_log));
  write_file.SetDispatchEventImplForTesting(
      base::Bind(&util::LoggingDispatchEventImpl::OnDispatchEventImpl,
                 base::Unretained(&dispatcher)));

  EXPECT_TRUE(write_file.Execute(kRequestId));

  write_file.OnError(kRequestId,
                     scoped_ptr<RequestValue>(new RequestValue()),
                     base::File::FILE_ERROR_TOO_MANY_OPENED);

  ASSERT_EQ(1u, callback_log.size());
  EXPECT_EQ(base::File::FILE_ERROR_TOO_MANY_OPENED, callback_log[0]);
}

}  // namespace operations
}  // namespace file_system_provider
}  // namespace chromeos
