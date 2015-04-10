// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTES_OBSERVER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTES_OBSERVER_H_

#include <vector>

#include "base/macros.h"
#include "chrome/browser/media/router/media_route.h"

namespace media_router {

class MediaRouter;

// Base class for observing when the set of MediaRoutes and their associated
// MediaSinks have been updated.
class MediaRoutesObserver {
 public:
  explicit MediaRoutesObserver(MediaRouter* router);
  virtual ~MediaRoutesObserver();

  // This function is invoked when the list of routes and their associated
  // sinks have been updated. Routes included in the list are created either
  // locally or remotely.
  virtual void OnRoutesUpdated(const std::vector<MediaRoute>& routes) {}

 private:
  MediaRouter* router_;

  DISALLOW_COPY_AND_ASSIGN(MediaRoutesObserver);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MEDIA_ROUTES_OBSERVER_H_
