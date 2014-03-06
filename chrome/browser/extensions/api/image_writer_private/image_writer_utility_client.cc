// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/api/image_writer_private/image_writer_utility_client.h"
#include "chrome/common/chrome_utility_messages.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

ImageWriterUtilityClient::ImageWriterUtilityClient()
    : message_loop_proxy_(base::MessageLoopProxy::current()) {}
ImageWriterUtilityClient::~ImageWriterUtilityClient() {}

void ImageWriterUtilityClient::Write(const ProgressCallback& progress_callback,
                                     const SuccessCallback& success_callback,
                                     const ErrorCallback& error_callback,
                                     const base::FilePath& source,
                                     const base::FilePath& target) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  StartHost();

  progress_callback_ = progress_callback;
  success_callback_ = success_callback;
  error_callback_ = error_callback;

  if (!Send(new ChromeUtilityMsg_ImageWriter_Write(source, target))) {
    DLOG(ERROR) << "Unable to send Write message to Utility Process.";
    message_loop_proxy_->PostTask(
        FROM_HERE, base::Bind(error_callback_, "IPC communication failed"));
  }
}

void ImageWriterUtilityClient::Verify(const ProgressCallback& progress_callback,
                                      const SuccessCallback& success_callback,
                                      const ErrorCallback& error_callback,
                                      const base::FilePath& source,
                                      const base::FilePath& target) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  StartHost();

  progress_callback_ = progress_callback;
  success_callback_ = success_callback;
  error_callback_ = error_callback;

  if (!Send(new ChromeUtilityMsg_ImageWriter_Verify(source, target))) {
    DLOG(ERROR) << "Unable to send Verify message to Utility Process.";
    message_loop_proxy_->PostTask(
        FROM_HERE, base::Bind(error_callback_, "IPC communication failed"));
  }
}

void ImageWriterUtilityClient::Cancel(const CancelCallback& cancel_callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (!utility_process_host_) {
    // If we haven't connected, there is nothing to cancel.
    message_loop_proxy_->PostTask(FROM_HERE, cancel_callback);
    return;
  }

  cancel_callback_ = cancel_callback;

  if (!Send(new ChromeUtilityMsg_ImageWriter_Cancel())) {
    DLOG(ERROR) << "Unable to send Cancel message to Utility Process.";
  }
}

void ImageWriterUtilityClient::Shutdown() {
  if (utility_process_host_ &&
      Send(new ChromeUtilityMsg_ImageWriter_Cancel())) {
    utility_process_host_->EndBatchMode();
  }

  // Clear handlers to not hold any reference to the caller.
  success_callback_ = base::Closure();
  progress_callback_ = base::Callback<void(int64)>();
  error_callback_ = base::Callback<void(const std::string&)>();
  cancel_callback_ = base::Closure();
}

void ImageWriterUtilityClient::StartHost() {
  if (!utility_process_host_) {
    scoped_refptr<base::SequencedTaskRunner> task_runner =
        base::MessageLoop::current()->message_loop_proxy();
    utility_process_host_ = content::UtilityProcessHost::Create(
                                this, task_runner.get())->AsWeakPtr();

#if defined(OS_WIN)
    utility_process_host_->ElevatePrivileges();
#else
    utility_process_host_->DisableSandbox();
#endif
    utility_process_host_->StartBatchMode();
    utility_process_host_->DisableSandbox();
  }
}

void ImageWriterUtilityClient::OnProcessCrashed(int exit_code) {
  message_loop_proxy_->PostTask(
      FROM_HERE, base::Bind(error_callback_, "Utility process crashed."));
}

void ImageWriterUtilityClient::OnProcessLaunchFailed() {
  message_loop_proxy_->PostTask(
      FROM_HERE, base::Bind(error_callback_, "Process launch failed."));
}

bool ImageWriterUtilityClient::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ImageWriterUtilityClient, message)
  IPC_MESSAGE_HANDLER(ChromeUtilityHostMsg_ImageWriter_Succeeded,
                      OnWriteImageSucceeded)
  IPC_MESSAGE_HANDLER(ChromeUtilityHostMsg_ImageWriter_Cancelled,
                      OnWriteImageCancelled)
  IPC_MESSAGE_HANDLER(ChromeUtilityHostMsg_ImageWriter_Failed,
                      OnWriteImageFailed)
  IPC_MESSAGE_HANDLER(ChromeUtilityHostMsg_ImageWriter_Progress,
                      OnWriteImageProgress)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

bool ImageWriterUtilityClient::Send(IPC::Message* msg) {
  return utility_process_host_ && utility_process_host_->Send(msg);
}

void ImageWriterUtilityClient::OnWriteImageSucceeded() {
  if (!success_callback_.is_null()) {
    message_loop_proxy_->PostTask(FROM_HERE, success_callback_);
  }
}

void ImageWriterUtilityClient::OnWriteImageCancelled() {
  if (!cancel_callback_.is_null()) {
    message_loop_proxy_->PostTask(FROM_HERE, cancel_callback_);
  }
}

void ImageWriterUtilityClient::OnWriteImageFailed(const std::string& message) {
  if (!error_callback_.is_null()) {
    message_loop_proxy_->PostTask(FROM_HERE,
                                  base::Bind(error_callback_, message));
  }
}

void ImageWriterUtilityClient::OnWriteImageProgress(int64 progress) {
  if (!progress_callback_.is_null()) {
    message_loop_proxy_->PostTask(FROM_HERE,
                                  base::Bind(progress_callback_, progress));
  }
}
