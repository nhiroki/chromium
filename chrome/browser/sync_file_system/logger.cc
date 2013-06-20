// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync_file_system/logger.h"

#include "base/file_util.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/drive/event_logger.h"

namespace sync_file_system {
namespace util {
namespace {

static base::LazyInstance<google_apis::EventLogger> g_logger =
    LAZY_INSTANCE_INITIALIZER;

const char* LogSeverityToString(logging::LogSeverity level) {
  switch (level) {
    case logging::LOG_ERROR:
      return "ERROR";
    case logging::LOG_WARNING:
      return "WARNING";
    case logging::LOG_INFO:
      return "INFO";
    case logging::LOG_VERBOSE:
      return "VERBOSE";
  }

  NOTREACHED();
  return "Unknown Log Severity";
}

}  // namespace

void ClearLog() {
  g_logger.Pointer()->SetHistorySize(google_apis::kDefaultHistorySize);
}

void Log(logging::LogSeverity severity,
         const tracked_objects::Location& location,
         const char* format,
         ...) {
  std::string what;

  va_list args;
  va_start(args, format);
  base::StringAppendV(&what, format, args);
  va_end(args);

  // Use same output format as normal console logger.
  base::FilePath path = base::FilePath::FromUTF8Unsafe(location.file_name());
  std::string log_output = base::StringPrintf(
      "[%s: %s(%d)] %s",
      LogSeverityToString(severity),
      path.BaseName().AsUTF8Unsafe().c_str(),
      location.line_number(),
      what.c_str());

  // Log to WebUI regardless of LogSeverity (e.g. ignores command line flags).
  // On thread-safety: LazyInstance guarantees thread-safety for the object
  // creation. EventLogger::Log() internally maintains the lock.
  google_apis::EventLogger* ptr = g_logger.Pointer();
  ptr->Log("%s", log_output.c_str());

  // Log to console if the severity is at or above the min level.
  // LOG_VERBOSE logs are also output if the verbosity of this module
  // (sync_file_system/logger) is >= 1.
  // TODO(kinuko,calvinlo): Reconsider this logging hack, it's not recommended
  // to directly use LogMessage.
  if (severity < logging::GetMinLogLevel() && !VLOG_IS_ON(1))
    return;
  logging::LogMessage(location.file_name(), location.line_number(), severity)
      .stream() << what;
}

std::vector<google_apis::EventLogger::Event> GetLogHistory() {
  google_apis::EventLogger* ptr = g_logger.Pointer();
  return ptr->GetHistory();
}

}  // namespace util
}  // namespace sync_file_system
