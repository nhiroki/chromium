// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRI_DRI_WINDOW_H_
#define UI_OZONE_PLATFORM_DRI_DRI_WINDOW_H_

#include "base/memory/scoped_ptr.h"
#include "ui/events/platform/platform_event_dispatcher.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/platform_window/platform_window.h"

namespace ui {

class DriCursor;
class DriWindowDelegate;
class DriWindowManager;
class EventFactoryEvdev;

class DriWindow : public PlatformWindow,
                  public PlatformEventDispatcher {
 public:
  DriWindow(PlatformWindowDelegate* delegate,
            const gfx::Rect& bounds,
            scoped_ptr<DriWindowDelegate> dri_window_delegate,
            EventFactoryEvdev* event_factory,
            DriWindowManager* window_manager,
            DriCursor* cursor);
  virtual ~DriWindow();

  void Initialize();

  // PlatformWindow:
  virtual void Show() OVERRIDE;
  virtual void Hide() OVERRIDE;
  virtual void Close() OVERRIDE;
  virtual void SetBounds(const gfx::Rect& bounds) OVERRIDE;
  virtual gfx::Rect GetBounds() OVERRIDE;
  virtual void SetCapture() OVERRIDE;
  virtual void ReleaseCapture() OVERRIDE;
  virtual void ToggleFullscreen() OVERRIDE;
  virtual void Maximize() OVERRIDE;
  virtual void Minimize() OVERRIDE;
  virtual void Restore() OVERRIDE;
  virtual void SetCursor(PlatformCursor cursor) OVERRIDE;
  virtual void MoveCursorTo(const gfx::Point& location) OVERRIDE;

  // PlatformEventDispatcher:
  virtual bool CanDispatchEvent(const PlatformEvent& event) OVERRIDE;
  virtual uint32_t DispatchEvent(const PlatformEvent& event) OVERRIDE;

 private:
  PlatformWindowDelegate* delegate_;
  gfx::Rect bounds_;
  gfx::AcceleratedWidget widget_;
  DriWindowDelegate* dri_window_delegate_;
  EventFactoryEvdev* event_factory_;
  DriWindowManager* window_manager_;
  DriCursor* cursor_;

  DISALLOW_COPY_AND_ASSIGN(DriWindow);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRI_DRI_WINDOW_H_
