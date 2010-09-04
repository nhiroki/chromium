// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GLUE_PLUGINS_CARBON_PLUGIN_WINDOW_TRACKER_MAC_H_
#define WEBKIT_GLUE_PLUGINS_CARBON_PLUGIN_WINDOW_TRACKER_MAC_H_

#include <Carbon/Carbon.h>
#include <map>

#include "base/basictypes.h"

class WebPluginDelegateImpl;

// Creates and tracks the invisible windows that are necessary for
// Carbon-event-model plugins.
//
// Serves as a bridge between plugin delegate instances and the Carbon
// interposing library. The Carbon functions we interpose work in terms of
// WindowRefs, and we need to be able to map from those back to the plugin
// delegates that know what we should claim about the state of the window.
class __attribute__((visibility("default"))) CarbonPluginWindowTracker {
 public:
  CarbonPluginWindowTracker();

  // Returns the shared window tracker instance.
  static CarbonPluginWindowTracker* SharedInstance();

  // Creates a new carbon window associated with |delegate|.
  WindowRef CreateDummyWindowForDelegate(WebPluginDelegateImpl* delegate);

  // Returns the WebPluginDelegate associated with the given dummy window.
  WebPluginDelegateImpl* GetDelegateForDummyWindow(WindowRef window) const;

  // Returns the dummy window associated with |delegate|.
  WindowRef GetDummyWindowForDelegate(WebPluginDelegateImpl* delegate) const;

  // Destroys the dummy window for |delegate|.
  void DestroyDummyWindowForDelegate(WebPluginDelegateImpl* delegate,
                                     WindowRef window);

 private:
  typedef std::map<WindowRef, WebPluginDelegateImpl*> WindowToDelegateMap;
  typedef std::map<WebPluginDelegateImpl*, WindowRef> DelegateToWindowMap;
  WindowToDelegateMap window_to_delegate_map_;
  DelegateToWindowMap delegate_to_window_map_;

  DISALLOW_COPY_AND_ASSIGN(CarbonPluginWindowTracker);
};

#endif  // WEBKIT_GLUE_PLUGINS_CARBON_PLUGIN_WINDOW_TRACKER_MAC_H_
