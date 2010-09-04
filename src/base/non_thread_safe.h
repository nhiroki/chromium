// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_NON_THREAD_SAFE_H_
#define BASE_NON_THREAD_SAFE_H_
#pragma once

#include "base/platform_thread.h"
#include "base/thread_checker.h"

// A helper class used to help verify that methods of a class are
// called from the same thread.  One can inherit from this class and use
// CalledOnValidThread() to verify.
//
// This is intended to be used with classes that appear to be thread safe, but
// aren't.  For example, a service or a singleton like the preferences system.
//
// Example:
// class MyClass : public NonThreadSafe {
//  public:
//   void Foo() {
//     DCHECK(CalledOnValidThread());
//     ... (do stuff) ...
//   }
// }
//
// In Release mode, CalledOnValidThread will always return true.
//
#ifndef NDEBUG
class NonThreadSafe {
 public:
  ~NonThreadSafe();

  bool CalledOnValidThread() const;

 private:
  ThreadChecker thread_checker_;
};
#else
// Do nothing in release mode.
class NonThreadSafe {
 public:
  bool CalledOnValidThread() const {
    return true;
  }
};
#endif  // NDEBUG

#endif  // BASE_NON_THREAD_SAFE_H_
