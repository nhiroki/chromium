// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/fake_gcm_app_handler.h"

#include "base/run_loop.h"

namespace gcm {

FakeGCMAppHandler::FakeGCMAppHandler()
    : received_event_(NO_EVENT) {
}

FakeGCMAppHandler::~FakeGCMAppHandler() {
}

void FakeGCMAppHandler::WaitForNotification() {
  run_loop_.reset(new base::RunLoop);
  run_loop_->Run();
  run_loop_.reset();
}

void FakeGCMAppHandler::ShutdownHandler() {
}

void FakeGCMAppHandler::OnMessage(const std::string& app_id,
                                  const GCMClient::IncomingMessage& message) {
  ClearResults();
  received_event_ = MESSAGE_EVENT;
  app_id_ = app_id;
  message_ = message;
  if (run_loop_)
    run_loop_->Quit();
}

void FakeGCMAppHandler::OnMessagesDeleted(const std::string& app_id) {
  ClearResults();
  received_event_ = MESSAGES_DELETED_EVENT;
  app_id_ = app_id;
  if (run_loop_)
    run_loop_->Quit();
}

void FakeGCMAppHandler::OnSendError(
    const std::string& app_id,
    const GCMClient::SendErrorDetails& send_error_details) {
  ClearResults();
  received_event_ = SEND_ERROR_EVENT;
  app_id_ = app_id;
  send_error_details_ = send_error_details;
  if (run_loop_)
    run_loop_->Quit();
}

void FakeGCMAppHandler::OnSendAcknowledged(
    const std::string& app_id,
    const std::string& message_id) {
  ClearResults();
  app_id_ = app_id;
  acked_message_id_ = message_id;
  if (run_loop_)
    run_loop_->Quit();
}

void FakeGCMAppHandler::ClearResults() {
  received_event_ = NO_EVENT;
  app_id_.clear();
  acked_message_id_.clear();
  message_ = GCMClient::IncomingMessage();
  send_error_details_ = GCMClient::SendErrorDetails();
}

}  // namespace gcm
