// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SCHEDULER_CANCELABLE_CLOSURE_HOLDER_H_
#define CONTENT_RENDERER_SCHEDULER_CANCELABLE_CLOSURE_HOLDER_H_

#include "base/cancelable_callback.h"

namespace content {

// A CancelableClosureHolder is a CancelableCallback which resets its wrapped
// callback with a cached closure whenever it is canceled.
class CancelableClosureHolder {
 public:
  CancelableClosureHolder();
  ~CancelableClosureHolder();

  // Resets the closure to be wrapped by the cancelable callback.  Cancels any
  // outstanding callbacks.
  void Reset(const base::Closure& callback);

  // Cancels any outstanding closures returned by callback().
  void Cancel();

  // Returns a callback that will be disabled by calling Cancel(). Callback
  // must have been set using Reset() before calling this function.
  const base::Closure& callback() const;

 private:
  base::Closure callback_;
  base::CancelableClosure cancelable_callback_;

  DISALLOW_COPY_AND_ASSIGN(CancelableClosureHolder);
};

}  // namespace content

#endif  // CONTENT_RENDERER_SCHEDULER_CANCELABLE_CLOSURE_HOLDER_H_
