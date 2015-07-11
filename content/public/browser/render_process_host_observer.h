// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_RENDER_PROCESS_HOST_OBSERVER_H_
#define CONTENT_PUBLIC_BROWSER_RENDER_PROCESS_HOST_OBSERVER_H_

#include "base/process/kill.h"
#include "base/process/process_handle.h"
#include "content/common/content_export.h"

namespace content {

class RenderProcessHost;

// An observer API implemented by classes which are interested
// in RenderProcessHost lifecycle events.
class CONTENT_EXPORT RenderProcessHostObserver {
 public:
  // This method is invoked when the process of the observed RenderProcessHost
  // exits (either normally or with a crash). To determine if the process closed
  // normally or crashed, examine the |status| parameter.
  //
  // This will cause a call to WebContentsObserver::RenderProcessGone() for the
  // active renderer process for the top-level frame; for code that needs to be
  // a WebContentsObserver anyway, consider whether that API might be a better
  // choice.
  virtual void RenderProcessExited(RenderProcessHost* host,
                                   base::TerminationStatus status,
                                   int exit_code) {}

  // This method is invoked when the observed RenderProcessHost itself is
  // destroyed. This is guaranteed to be the last call made to the observer.
  virtual void RenderProcessHostDestroyed(RenderProcessHost* host) {}

 protected:
  virtual ~RenderProcessHostObserver() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_RENDER_PROCESS_HOST_OBSERVER_H_
