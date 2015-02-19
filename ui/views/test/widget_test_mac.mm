// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/test/widget_test.h"

#include <Cocoa/Cocoa.h>

#import "base/mac/scoped_nsobject.h"
#import "base/mac/scoped_objc_class_swizzler.h"
#include "ui/views/widget/root_view.h"

@interface IsKeyWindowDonor : NSObject
@end

@implementation IsKeyWindowDonor
- (BOOL)isKeyWindow {
  return YES;
}
@end

namespace views {
namespace test {

namespace {

class FakeActivationMac : public WidgetTest::FakeActivation {
 public:
  FakeActivationMac()
      : swizzler_([NSWindow class],
                  [IsKeyWindowDonor class],
                  @selector(isKeyWindow)) {}

 private:
  base::mac::ScopedObjCClassSwizzler swizzler_;

  DISALLOW_COPY_AND_ASSIGN(FakeActivationMac);
};

}  // namespace

// static
void WidgetTest::SimulateNativeDestroy(Widget* widget) {
  // Retain the window while closing it, otherwise the window may lose its last
  // owner before -[NSWindow close] completes (this offends AppKit). Usually
  // this reference will exist on an event delivered to the runloop.
  base::scoped_nsobject<NSWindow> window([widget->GetNativeWindow() retain]);
  [window close];
}

// static
bool WidgetTest::IsNativeWindowVisible(gfx::NativeWindow window) {
  return [window isVisible];
}

// static
bool WidgetTest::IsWindowStackedAbove(Widget* above, Widget* below) {
  EXPECT_TRUE(above->IsVisible());
  EXPECT_TRUE(below->IsVisible());

  // -[NSApplication orderedWindows] are ordered front-to-back.
  NSWindow* first = above->GetNativeWindow();
  NSWindow* second = below->GetNativeWindow();

  for (NSWindow* window in [NSApp orderedWindows]) {
    if (window == second)
      return !first;

    if (window == first)
      first = nil;
  }
  return false;
}

// static
ui::EventProcessor* WidgetTest::GetEventProcessor(Widget* widget) {
  return static_cast<internal::RootView*>(widget->GetRootView());
}

// static
scoped_ptr<WidgetTest::FakeActivation> WidgetTest::FakeWidgetIsActiveAlways() {
  return make_scoped_ptr(new FakeActivationMac);
}

}  // namespace test
}  // namespace views
