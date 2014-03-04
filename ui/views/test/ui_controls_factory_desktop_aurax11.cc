// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <X11/keysym.h>
#include <X11/Xlib.h>

// X macro fail.
#if defined(RootWindow)
#undef RootWindow
#endif

#include "base/logging.h"
#include "ui/aura/client/screen_position_client.h"
#include "ui/aura/env.h"
#include "ui/aura/test/ui_controls_factory_aura.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/test/ui_controls_aura.h"
#include "ui/base/x/x11_util.h"
#include "ui/compositor/dip_util.h"
#include "ui/events/keycodes/keyboard_code_conversion_x.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_x11.h"

namespace views {
namespace test {
namespace {

using ui_controls::DOWN;
using ui_controls::LEFT;
using ui_controls::MIDDLE;
using ui_controls::MouseButton;
using ui_controls::RIGHT;
using ui_controls::UIControlsAura;
using ui_controls::UP;

// Mask of the buttons currently down.
unsigned button_down_mask = 0;

// Event waiter executes the specified closure|when a matching event
// is found.
// TODO(oshima): Move this to base.
class EventWaiter : public base::MessageLoopForUI::Observer {
 public:
  typedef bool (*EventWaiterMatcher)(const base::NativeEvent& event);

  EventWaiter(const base::Closure& closure, EventWaiterMatcher matcher)
      : closure_(closure),
        matcher_(matcher) {
    base::MessageLoopForUI::current()->AddObserver(this);
  }

  virtual ~EventWaiter() {
    base::MessageLoopForUI::current()->RemoveObserver(this);
  }

  // MessageLoop::Observer implementation:
  virtual base::EventStatus WillProcessEvent(
      const base::NativeEvent& event) OVERRIDE {
    if ((*matcher_)(event)) {
      base::MessageLoop::current()->PostTask(FROM_HERE, closure_);
      delete this;
    }
    return base::EVENT_CONTINUE;
  }

  virtual void DidProcessEvent(const base::NativeEvent& event) OVERRIDE {
  }

 private:
  base::Closure closure_;
  EventWaiterMatcher matcher_;
  DISALLOW_COPY_AND_ASSIGN(EventWaiter);
};

// Returns atom that indidates that the XEvent is marker event.
Atom MarkerEventAtom() {
  return XInternAtom(gfx::GetXDisplay(), "marker_event", False);
}

// Returns true when the event is a marker event.
bool Matcher(const base::NativeEvent& event) {
  return event->xany.type == ClientMessage &&
      event->xclient.message_type == MarkerEventAtom();
}

class UIControlsDesktopX11 : public UIControlsAura {
 public:
  UIControlsDesktopX11()
      : x_display_(gfx::GetXDisplay()),
        x_root_window_(DefaultRootWindow(x_display_)),
        x_window_(XCreateWindow(
            x_display_, x_root_window_,
            -100, -100, 10, 10,  // x, y, width, height
            0,                   // border width
            CopyFromParent,      // depth
            InputOnly,
            CopyFromParent,      // visual
            0,
            NULL)) {
    XStoreName(x_display_, x_window_, "Chromium UIControlsDesktopX11 Window");
  }

  virtual ~UIControlsDesktopX11() {
    XDestroyWindow(x_display_, x_window_);
  }

  virtual bool SendKeyPress(gfx::NativeWindow window,
                            ui::KeyboardCode key,
                            bool control,
                            bool shift,
                            bool alt,
                            bool command) OVERRIDE {
    DCHECK(!command);  // No command key on Aura
    return SendKeyPressNotifyWhenDone(
        window, key, control, shift, alt, command, base::Closure());
  }

  virtual bool SendKeyPressNotifyWhenDone(
      gfx::NativeWindow window,
      ui::KeyboardCode key,
      bool control,
      bool shift,
      bool alt,
      bool command,
      const base::Closure& closure) OVERRIDE {
    DCHECK(!command);  // No command key on Aura

    aura::WindowTreeHost* host = window->GetHost();

    XEvent xevent = {0};
    xevent.xkey.type = KeyPress;
    if (control) {
      SetKeycodeAndSendThenMask(host, &xevent, XK_Control_L, ControlMask);
    }
    if (shift)
      SetKeycodeAndSendThenMask(host, &xevent, XK_Shift_L, ShiftMask);
    if (alt)
      SetKeycodeAndSendThenMask(host, &xevent, XK_Alt_L, Mod1Mask);
    xevent.xkey.keycode =
        XKeysymToKeycode(x_display_,
                         ui::XKeysymForWindowsKeyCode(key, shift));
    host->PostNativeEvent(&xevent);

    // Send key release events.
    xevent.xkey.type = KeyRelease;
    host->PostNativeEvent(&xevent);
    if (alt)
      UnmaskAndSetKeycodeThenSend(host, &xevent, Mod1Mask, XK_Alt_L);
    if (shift)
      UnmaskAndSetKeycodeThenSend(host, &xevent, ShiftMask, XK_Shift_L);
    if (control) {
      UnmaskAndSetKeycodeThenSend(host, &xevent, ControlMask, XK_Control_L);
    }
    DCHECK(!xevent.xkey.state);
    RunClosureAfterAllPendingUIEvents(closure);
    return true;
  }

  virtual bool SendMouseMove(long screen_x, long screen_y) OVERRIDE {
    return SendMouseMoveNotifyWhenDone(screen_x, screen_y, base::Closure());
  }
  virtual bool SendMouseMoveNotifyWhenDone(
      long screen_x,
      long screen_y,
      const base::Closure& closure) OVERRIDE {
    gfx::Point screen_location(screen_x, screen_y);
    gfx::Point root_location = screen_location;
    aura::Window* root_window = RootWindowForPoint(screen_location);

    aura::client::ScreenPositionClient* screen_position_client =
          aura::client::GetScreenPositionClient(root_window);
    if (screen_position_client) {
      screen_position_client->ConvertPointFromScreen(root_window,
                                                     &root_location);
    }

    aura::WindowTreeHost* host = root_window->GetHost();
    gfx::Point root_current_location;
    host->QueryMouseLocation(&root_current_location);
    host->ConvertPointFromHost(&root_current_location);

    if (root_location != root_current_location && button_down_mask == 0) {
      // Move the cursor because EnterNotify/LeaveNotify are generated with the
      // current mouse position as a result of XGrabPointer()
      root_window->MoveCursorTo(root_location);
    } else {
      XEvent xevent = {0};
      XMotionEvent* xmotion = &xevent.xmotion;
      xmotion->type = MotionNotify;
      xmotion->x = root_location.x();
      xmotion->y = root_location.y();
      xmotion->state = button_down_mask;
      xmotion->same_screen = True;
      // RootWindow will take care of other necessary fields.
      host->PostNativeEvent(&xevent);
    }
    RunClosureAfterAllPendingUIEvents(closure);
    return true;
  }
  virtual bool SendMouseEvents(MouseButton type, int state) OVERRIDE {
    return SendMouseEventsNotifyWhenDone(type, state, base::Closure());
  }
  virtual bool SendMouseEventsNotifyWhenDone(
      MouseButton type,
      int state,
      const base::Closure& closure) OVERRIDE {
    XEvent xevent = {0};
    XButtonEvent* xbutton = &xevent.xbutton;
    gfx::Point mouse_loc = aura::Env::GetInstance()->last_mouse_location();
    aura::Window* root_window = RootWindowForPoint(mouse_loc);
    aura::client::ScreenPositionClient* screen_position_client =
          aura::client::GetScreenPositionClient(root_window);
    if (screen_position_client)
      screen_position_client->ConvertPointFromScreen(root_window, &mouse_loc);
    xbutton->x = mouse_loc.x();
    xbutton->y = mouse_loc.y();
    xbutton->same_screen = True;
    switch (type) {
      case LEFT:
        xbutton->button = Button1;
        xbutton->state = Button1Mask;
        break;
      case MIDDLE:
        xbutton->button = Button2;
        xbutton->state = Button2Mask;
        break;
      case RIGHT:
        xbutton->button = Button3;
        xbutton->state = Button3Mask;
        break;
    }
    // RootWindow will take care of other necessary fields.
    if (state & DOWN) {
      xevent.xbutton.type = ButtonPress;
      root_window->GetHost()->PostNativeEvent(&xevent);
      button_down_mask |= xbutton->state;
    }
    if (state & UP) {
      xevent.xbutton.type = ButtonRelease;
      root_window->GetHost()->PostNativeEvent(&xevent);
      button_down_mask = (button_down_mask | xbutton->state) ^ xbutton->state;
    }
    RunClosureAfterAllPendingUIEvents(closure);
    return true;
  }
  virtual bool SendMouseClick(MouseButton type) OVERRIDE {
    return SendMouseEvents(type, UP | DOWN);
  }
  virtual void RunClosureAfterAllPendingUIEvents(
      const base::Closure& closure) OVERRIDE {
    if (closure.is_null())
      return;
    static XEvent* marker_event = NULL;
    if (!marker_event) {
      marker_event = new XEvent();
      marker_event->xclient.type = ClientMessage;
      marker_event->xclient.display = x_display_;
      marker_event->xclient.window = x_window_;
      marker_event->xclient.format = 8;
    }
    marker_event->xclient.message_type = MarkerEventAtom();
    XSendEvent(x_display_, x_window_, False, 0, marker_event);
    new EventWaiter(closure, &Matcher);
  }
 private:
  aura::Window* RootWindowForPoint(const gfx::Point& point) {
    // Most interactive_ui_tests run inside of the aura_test_helper
    // environment. This means that we can't rely on gfx::Screen and several
    // other things to work properly. Therefore we hack around this by
    // iterating across the windows owned DesktopWindowTreeHostX11 since this
    // doesn't rely on having a DesktopScreenX11.
    std::vector<aura::Window*> windows =
        DesktopWindowTreeHostX11::GetAllOpenWindows();
    for (std::vector<aura::Window*>::const_iterator it = windows.begin();
         it != windows.end(); ++it) {
      if ((*it)->GetBoundsInScreen().Contains(point)) {
        return (*it)->GetRootWindow();
      }
    }

    NOTREACHED() << "Coulding find RW for " << point.ToString() << " among "
                 << windows.size() << " RWs.";
    return NULL;
  }

  void SetKeycodeAndSendThenMask(aura::WindowTreeHost* host,
                                 XEvent* xevent,
                                 KeySym keysym,
                                 unsigned int mask) {
    xevent->xkey.keycode = XKeysymToKeycode(x_display_, keysym);
    host->PostNativeEvent(xevent);
    xevent->xkey.state |= mask;
  }

  void UnmaskAndSetKeycodeThenSend(aura::WindowTreeHost* host,
                                   XEvent* xevent,
                                   unsigned int mask,
                                   KeySym keysym) {
    xevent->xkey.state ^= mask;
    xevent->xkey.keycode = XKeysymToKeycode(x_display_, keysym);
    host->PostNativeEvent(xevent);
  }

  // Our X11 state.
  Display* x_display_;
  ::Window x_root_window_;

  // Input-only window used for events.
  ::Window x_window_;

  DISALLOW_COPY_AND_ASSIGN(UIControlsDesktopX11);
};

}  // namespace

UIControlsAura* CreateUIControlsDesktopAura() {
  return new UIControlsDesktopX11();
}

}  // namespace test
}  // namespace views
