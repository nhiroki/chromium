// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/proximity_auth/logging/log_buffer.h"

#include "base/lazy_instance.h"

namespace proximity_auth {

namespace {

// The maximum number of logs that can be stored in the buffer.
const size_t kMaxBufferSize = 1000;

// The global instance returned by LogBuffer::GetInstance().
base::LazyInstance<LogBuffer>::Leaky g_log_buffer = LAZY_INSTANCE_INITIALIZER;

}  // namespace

LogBuffer::LogMessage::LogMessage(const std::string& text,
                                  const base::Time& time,
                                  const std::string& file,
                                  const int line,
                                  logging::LogSeverity severity)
    : text(text), time(time), file(file), line(line), severity(severity) {
}

LogBuffer::LogBuffer() {
}

LogBuffer::~LogBuffer() {
}

// static
LogBuffer* LogBuffer::GetInstance() {
  return &g_log_buffer.Get();
}

void LogBuffer::AddLogMessage(const LogMessage& log_message) {
  // Note: We may want to sort the messages by timestamp if there are cases
  // where logs are not added chronologically.
  log_messages_.push_back(log_message);
  if (log_messages_.size() > MaxBufferSize())
    log_messages_.pop_front();
}

void LogBuffer::Clear() {
  log_messages_.clear();
}

size_t LogBuffer::MaxBufferSize() const {
  return kMaxBufferSize;
}

}  // proximity_auth
