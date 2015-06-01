// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WEB_MODAL_POPUP_MANAGER_H_
#define COMPONENTS_WEB_MODAL_POPUP_MANAGER_H_

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "ui/gfx/native_widget_types.h"

namespace content {
class WebContents;
}

namespace gfx {
class Size;
}

namespace web_modal {

class WebContentsModalDialogHost;

// Per-Browser class to manage popups (bubbles, web-modal dialogs).
class PopupManager {
 public:
  // |host| may be null.
  explicit PopupManager(WebContentsModalDialogHost* host);

  ~PopupManager();

  // Returns the native view which will be the parent of managed popups.
  gfx::NativeView GetHostView() const;

  // Temporary method: Provides compatibility with existing
  // WebContentsModalDialogManager code.
  void ShowModalDialog(gfx::NativeWindow popup,
                       content::WebContents* web_contents);

  // Returns true if a web modal dialog is active and not closed in the
  // given |web_contents|. Note: this is intended for legacy use only; it will
  // be deleted at some point -- new code shouldn't use it.
  bool IsWebModalDialogActive(const content::WebContents* web_contents) const;

  // Called when a native popup we own is about to be closed.
  void WillClose(gfx::NativeWindow popup);

  // Called by views code to re-activate popups anchored to a particular tab
  // when that tab gets focus. Note that depending on the situation, more than
  // one popup may actually be shown (depending on overlappability). The
  // semantics are that the popups that would have been displayed had the tab
  // never lost focus are re-focused when tab focus is regained.
  void WasFocused(const content::WebContents* web_contents);

  // WebContentsUserData-alike API for retrieving the associated window
  // PopupManager from a |web_contents|. Any window which doesn't have a popup
  // manager associated will return null -- popups should not be issued against
  // that window.
  static PopupManager* FromWebContents(content::WebContents* web_contents);

  // Should not be called except by WebContents-owning class; not by clients.
  void RegisterWith(content::WebContents* web_contents);
  void UnregisterWith(content::WebContents* web_contents);

  // DEPRECATED.
  void CloseAllDialogsForTesting(content::WebContents* web_contents);

 private:
  WebContentsModalDialogHost* host_;

  base::WeakPtrFactory<PopupManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PopupManager);
};

}  // namespace web_modal

#endif  // COMPONENTS_WEB_MODAL_POPUP_MANAGER_H_
