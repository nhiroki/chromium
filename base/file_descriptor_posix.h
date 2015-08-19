// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_FILE_DESCRIPTOR_POSIX_H_
#define BASE_FILE_DESCRIPTOR_POSIX_H_

#include "base/files/file.h"
#include "base/files/scoped_file.h"

namespace base {

// -----------------------------------------------------------------------------
// We introduct a special structure for file descriptors in order that we are
// able to use template specialisation to special-case their handling.
//
// IMPORTANT: This is primarily intended for use when sending file descriptors
// over IPC. Even if |auto_close| is true, base::FileDescriptor does NOT close()
// |fd| when going out of scope. Instead, a consumer of a base::FileDescriptor
// must invoke close() on |fd| if |auto_close| is true.
//
// In the case of IPC, the the IPC subsystem knows to close() |fd| after sending
// a message that contains a base::FileDescriptor if auto_close == true. On the
// other end, the receiver must make sure to close() |fd| after it has finished
// processing the IPC message. See the IPC::ParamTraits<> specialization in
// ipc/ipc_message_utils.h for all the details.
// -----------------------------------------------------------------------------
struct FileDescriptor {
  FileDescriptor() : fd(-1), auto_close(false) {}

  FileDescriptor(int ifd, bool iauto_close) : fd(ifd), auto_close(iauto_close) {
  }

  FileDescriptor(File file) : fd(file.TakePlatformFile()), auto_close(true) {}
  explicit FileDescriptor(ScopedFD fd) : fd(fd.release()), auto_close(true) {}

  bool operator==(const FileDescriptor& other) const {
    return (fd == other.fd && auto_close == other.auto_close);
  }

  bool operator!=(const FileDescriptor& other) const {
    return !operator==(other);
  }

  // A comparison operator so that we can use these as keys in a std::map.
  bool operator<(const FileDescriptor& other) const {
    return other.fd < fd;
  }

  int fd;
  // If true, this file descriptor should be closed after it has been used. For
  // example an IPC system might interpret this flag as indicating that the
  // file descriptor it has been given should be closed after use.
  bool auto_close;
};

}  // namespace base

#endif  // BASE_FILE_DESCRIPTOR_POSIX_H_
