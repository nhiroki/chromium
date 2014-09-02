// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <set>

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/hit_test.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/compositor/scoped_animation_duration_scale_mode.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/events/event_processor.h"
#include "ui/events/event_utils.h"
#include "ui/events/test/event_generator.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/point.h"
#include "ui/views/bubble/bubble_delegate.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/test/test_views_delegate.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/views_delegate.h"
#include "ui/views/widget/native_widget_delegate.h"
#include "ui/views/widget/root_view.h"
#include "ui/views/widget/widget_deletion_observer.h"
#include "ui/views/window/dialog_delegate.h"
#include "ui/views/window/native_frame_view.h"

#if defined(OS_WIN)
#include "ui/views/win/hwnd_util.h"
#endif

namespace views {
namespace test {

// A view that keeps track of the events it receives, optionally consuming them.
class EventCountView : public View {
 public:
  // Whether to call SetHandled() on events as they are received. For some event
  // types, this will allow EventCountView to receives future events in the
  // event sequence, such as a drag.
  enum HandleMode {
    PROPAGATE_EVENTS,
    CONSUME_EVENTS
  };

  EventCountView()
      : last_flags_(0),
        handle_mode_(PROPAGATE_EVENTS) {}

  virtual ~EventCountView() {}

  int GetEventCount(ui::EventType type) {
    return event_count_[type];
  }

  void ResetCounts() {
    event_count_.clear();
  }

  int last_flags() const {
    return last_flags_;
  }

  void set_handle_mode(HandleMode handle_mode) {
    handle_mode_ = handle_mode;
  }

 protected:
  // Overridden from View:
  virtual void OnMouseMoved(const ui::MouseEvent& event) OVERRIDE {
    // MouseMove events are not re-dispatched from the RootView.
    ++event_count_[ui::ET_MOUSE_MOVED];
    last_flags_ = 0;
  }

  // Overridden from ui::EventHandler:
  virtual void OnKeyEvent(ui::KeyEvent* event) OVERRIDE {
    RecordEvent(event);
  }
  virtual void OnMouseEvent(ui::MouseEvent* event) OVERRIDE {
    RecordEvent(event);
  }
  virtual void OnScrollEvent(ui::ScrollEvent* event) OVERRIDE {
    RecordEvent(event);
  }
  virtual void OnGestureEvent(ui::GestureEvent* event) OVERRIDE {
    RecordEvent(event);
  }

 private:
  void RecordEvent(ui::Event* event) {
    ++event_count_[event->type()];
    last_flags_ = event->flags();
    if (handle_mode_ == CONSUME_EVENTS)
      event->SetHandled();
  }

  std::map<ui::EventType, int> event_count_;
  int last_flags_;
  HandleMode handle_mode_;

  DISALLOW_COPY_AND_ASSIGN(EventCountView);
};

// A view that keeps track of the events it receives, and consumes all scroll
// gesture events and ui::ET_SCROLL events.
class ScrollableEventCountView : public EventCountView {
 public:
  ScrollableEventCountView() {}
  virtual ~ScrollableEventCountView() {}

 private:
  // Overridden from ui::EventHandler:
  virtual void OnGestureEvent(ui::GestureEvent* event) OVERRIDE {
    EventCountView::OnGestureEvent(event);
    switch (event->type()) {
      case ui::ET_GESTURE_SCROLL_BEGIN:
      case ui::ET_GESTURE_SCROLL_UPDATE:
      case ui::ET_GESTURE_SCROLL_END:
      case ui::ET_SCROLL_FLING_START:
        event->SetHandled();
        break;
      default:
        break;
    }
  }

  virtual void OnScrollEvent(ui::ScrollEvent* event) OVERRIDE {
    EventCountView::OnScrollEvent(event);
    if (event->type() == ui::ET_SCROLL)
      event->SetHandled();
  }

  DISALLOW_COPY_AND_ASSIGN(ScrollableEventCountView);
};

// A view that implements GetMinimumSize.
class MinimumSizeFrameView : public NativeFrameView {
 public:
  explicit MinimumSizeFrameView(Widget* frame): NativeFrameView(frame) {}
  virtual ~MinimumSizeFrameView() {}

 private:
  // Overridden from View:
  virtual gfx::Size GetMinimumSize() const OVERRIDE {
    return gfx::Size(300, 400);
  }

  DISALLOW_COPY_AND_ASSIGN(MinimumSizeFrameView);
};

// An event handler that simply keeps a count of the different types of events
// it receives.
class EventCountHandler : public ui::EventHandler {
 public:
  EventCountHandler() {}
  virtual ~EventCountHandler() {}

  int GetEventCount(ui::EventType type) {
    return event_count_[type];
  }

  void ResetCounts() {
    event_count_.clear();
  }

 protected:
  // Overridden from ui::EventHandler:
  virtual void OnEvent(ui::Event* event) OVERRIDE {
    RecordEvent(*event);
    ui::EventHandler::OnEvent(event);
  }

 private:
  void RecordEvent(const ui::Event& event) {
    ++event_count_[event.type()];
  }

  std::map<ui::EventType, int> event_count_;

  DISALLOW_COPY_AND_ASSIGN(EventCountHandler);
};

// Class that closes the widget (which ends up deleting it immediately) when the
// appropriate event is received.
class CloseWidgetView : public View {
 public:
  explicit CloseWidgetView(ui::EventType event_type)
      : event_type_(event_type) {
  }

  // ui::EventHandler override:
  virtual void OnEvent(ui::Event* event) OVERRIDE {
    if (event->type() == event_type_) {
      // Go through NativeWidgetPrivate to simulate what happens if the OS
      // deletes the NativeWindow out from under us.
      GetWidget()->native_widget_private()->CloseNow();
    } else {
      View::OnEvent(event);
      if (!event->IsTouchEvent())
        event->SetHandled();
    }
  }

 private:
  const ui::EventType event_type_;

  DISALLOW_COPY_AND_ASSIGN(CloseWidgetView);
};

ui::WindowShowState GetWidgetShowState(const Widget* widget) {
  // Use IsMaximized/IsMinimized/IsFullScreen instead of GetWindowPlacement
  // because the former is implemented on all platforms but the latter is not.
  return widget->IsFullscreen() ? ui::SHOW_STATE_FULLSCREEN :
      widget->IsMaximized() ? ui::SHOW_STATE_MAXIMIZED :
      widget->IsMinimized() ? ui::SHOW_STATE_MINIMIZED :
      widget->IsActive() ? ui::SHOW_STATE_NORMAL :
                           ui::SHOW_STATE_INACTIVE;
}

TEST_F(WidgetTest, WidgetInitParams) {
  // Widgets are not transparent by default.
  Widget::InitParams init1;
  EXPECT_EQ(Widget::InitParams::INFER_OPACITY, init1.opacity);
}

////////////////////////////////////////////////////////////////////////////////
// Widget::GetTopLevelWidget tests.

TEST_F(WidgetTest, GetTopLevelWidget_Native) {
  // Create a hierarchy of native widgets.
  Widget* toplevel = CreateTopLevelPlatformWidget();
  gfx::NativeView parent = toplevel->GetNativeView();
  Widget* child = CreateChildPlatformWidget(parent);

  EXPECT_EQ(toplevel, toplevel->GetTopLevelWidget());
  EXPECT_EQ(toplevel, child->GetTopLevelWidget());

  toplevel->CloseNow();
  // |child| should be automatically destroyed with |toplevel|.
}

// Test if a focus manager and an inputmethod work without CHECK failure
// when window activation changes.
TEST_F(WidgetTest, ChangeActivation) {
  Widget* top1 = CreateTopLevelPlatformWidget();
  // CreateInputMethod before activated
  top1->GetInputMethod();
  top1->Show();
  RunPendingMessages();

  Widget* top2 = CreateTopLevelPlatformWidget();
  top2->Show();
  RunPendingMessages();

  top1->Activate();
  RunPendingMessages();

  // Create InputMethod after deactivated.
  top2->GetInputMethod();
  top2->Activate();
  RunPendingMessages();

  top1->Activate();
  RunPendingMessages();

  top1->CloseNow();
  top2->CloseNow();
}

// Tests visibility of child widgets.
TEST_F(WidgetTest, Visibility) {
  Widget* toplevel = CreateTopLevelPlatformWidget();
  gfx::NativeView parent = toplevel->GetNativeView();
  Widget* child = CreateChildPlatformWidget(parent);

  EXPECT_FALSE(toplevel->IsVisible());
  EXPECT_FALSE(child->IsVisible());

  child->Show();

  EXPECT_FALSE(toplevel->IsVisible());
  EXPECT_FALSE(child->IsVisible());

  toplevel->Show();

  EXPECT_TRUE(toplevel->IsVisible());
  EXPECT_TRUE(child->IsVisible());

  toplevel->CloseNow();
  // |child| should be automatically destroyed with |toplevel|.
}

////////////////////////////////////////////////////////////////////////////////
// Widget ownership tests.
//
// Tests various permutations of Widget ownership specified in the
// InitParams::Ownership param.

// A WidgetTest that supplies a toplevel widget for NativeWidget to parent to.
class WidgetOwnershipTest : public WidgetTest {
 public:
  WidgetOwnershipTest() {}
  virtual ~WidgetOwnershipTest() {}

  virtual void SetUp() {
    WidgetTest::SetUp();
    desktop_widget_ = CreateTopLevelPlatformWidget();
  }

  virtual void TearDown() {
    desktop_widget_->CloseNow();
    WidgetTest::TearDown();
  }

 private:
  Widget* desktop_widget_;

  DISALLOW_COPY_AND_ASSIGN(WidgetOwnershipTest);
};

// A bag of state to monitor destructions.
struct OwnershipTestState {
  OwnershipTestState() : widget_deleted(false), native_widget_deleted(false) {}

  bool widget_deleted;
  bool native_widget_deleted;
};

// A platform NativeWidget subclass that updates a bag of state when it is
// destroyed.
class OwnershipTestNativeWidget : public PlatformNativeWidget {
 public:
  OwnershipTestNativeWidget(internal::NativeWidgetDelegate* delegate,
                            OwnershipTestState* state)
      : PlatformNativeWidget(delegate),
        state_(state) {
  }
  virtual ~OwnershipTestNativeWidget() {
    state_->native_widget_deleted = true;
  }

 private:
  OwnershipTestState* state_;

  DISALLOW_COPY_AND_ASSIGN(OwnershipTestNativeWidget);
};

// A views NativeWidget subclass that updates a bag of state when it is
// destroyed.
class OwnershipTestNativeWidgetAura : public NativeWidgetCapture {
 public:
  OwnershipTestNativeWidgetAura(internal::NativeWidgetDelegate* delegate,
                                OwnershipTestState* state)
      : NativeWidgetCapture(delegate),
        state_(state) {
  }
  virtual ~OwnershipTestNativeWidgetAura() {
    state_->native_widget_deleted = true;
  }

 private:
  OwnershipTestState* state_;

  DISALLOW_COPY_AND_ASSIGN(OwnershipTestNativeWidgetAura);
};

// A Widget subclass that updates a bag of state when it is destroyed.
class OwnershipTestWidget : public Widget {
 public:
  explicit OwnershipTestWidget(OwnershipTestState* state) : state_(state) {}
  virtual ~OwnershipTestWidget() {
    state_->widget_deleted = true;
  }

 private:
  OwnershipTestState* state_;

  DISALLOW_COPY_AND_ASSIGN(OwnershipTestWidget);
};

// Widget owns its NativeWidget, part 1: NativeWidget is a platform-native
// widget.
TEST_F(WidgetOwnershipTest, Ownership_WidgetOwnsPlatformNativeWidget) {
  OwnershipTestState state;

  scoped_ptr<Widget> widget(new OwnershipTestWidget(&state));
  Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_POPUP);
  params.native_widget =
      new OwnershipTestNativeWidgetAura(widget.get(), &state);
  params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  widget->Init(params);

  // Now delete the Widget, which should delete the NativeWidget.
  widget.reset();

  EXPECT_TRUE(state.widget_deleted);
  EXPECT_TRUE(state.native_widget_deleted);

  // TODO(beng): write test for this ownership scenario and the NativeWidget
  //             being deleted out from under the Widget.
}

// Widget owns its NativeWidget, part 2: NativeWidget is a NativeWidget.
TEST_F(WidgetOwnershipTest, Ownership_WidgetOwnsViewsNativeWidget) {
  OwnershipTestState state;

  scoped_ptr<Widget> widget(new OwnershipTestWidget(&state));
  Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_POPUP);
  params.native_widget =
      new OwnershipTestNativeWidgetAura(widget.get(), &state);
  params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  widget->Init(params);

  // Now delete the Widget, which should delete the NativeWidget.
  widget.reset();

  EXPECT_TRUE(state.widget_deleted);
  EXPECT_TRUE(state.native_widget_deleted);

  // TODO(beng): write test for this ownership scenario and the NativeWidget
  //             being deleted out from under the Widget.
}

// Widget owns its NativeWidget, part 3: NativeWidget is a NativeWidget,
// destroy the parent view.
TEST_F(WidgetOwnershipTest,
       Ownership_WidgetOwnsViewsNativeWidget_DestroyParentView) {
  OwnershipTestState state;

  Widget* toplevel = CreateTopLevelPlatformWidget();

  scoped_ptr<Widget> widget(new OwnershipTestWidget(&state));
  Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_POPUP);
  params.native_widget =
      new OwnershipTestNativeWidgetAura(widget.get(), &state);
  params.parent = toplevel->GetNativeView();
  params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  widget->Init(params);

  // Now close the toplevel, which deletes the view hierarchy.
  toplevel->CloseNow();

  RunPendingMessages();

  // This shouldn't delete the widget because it shouldn't be deleted
  // from the native side.
  EXPECT_FALSE(state.widget_deleted);
  EXPECT_FALSE(state.native_widget_deleted);

  // Now delete it explicitly.
  widget.reset();

  EXPECT_TRUE(state.widget_deleted);
  EXPECT_TRUE(state.native_widget_deleted);
}

// NativeWidget owns its Widget, part 1: NativeWidget is a platform-native
// widget.
TEST_F(WidgetOwnershipTest, Ownership_PlatformNativeWidgetOwnsWidget) {
  OwnershipTestState state;

  Widget* widget = new OwnershipTestWidget(&state);
  Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_POPUP);
  params.native_widget =
      new OwnershipTestNativeWidgetAura(widget, &state);
  widget->Init(params);

  // Now destroy the native widget.
  widget->CloseNow();

  EXPECT_TRUE(state.widget_deleted);
  EXPECT_TRUE(state.native_widget_deleted);
}

// NativeWidget owns its Widget, part 2: NativeWidget is a NativeWidget.
TEST_F(WidgetOwnershipTest, Ownership_ViewsNativeWidgetOwnsWidget) {
  OwnershipTestState state;

  Widget* toplevel = CreateTopLevelPlatformWidget();

  Widget* widget = new OwnershipTestWidget(&state);
  Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_POPUP);
  params.native_widget =
      new OwnershipTestNativeWidgetAura(widget, &state);
  params.parent = toplevel->GetNativeView();
  widget->Init(params);

  // Now destroy the native widget. This is achieved by closing the toplevel.
  toplevel->CloseNow();

  // The NativeWidget won't be deleted until after a return to the message loop
  // so we have to run pending messages before testing the destruction status.
  RunPendingMessages();

  EXPECT_TRUE(state.widget_deleted);
  EXPECT_TRUE(state.native_widget_deleted);
}

// NativeWidget owns its Widget, part 3: NativeWidget is a platform-native
// widget, destroyed out from under it by the OS.
TEST_F(WidgetOwnershipTest,
       Ownership_PlatformNativeWidgetOwnsWidget_NativeDestroy) {
  OwnershipTestState state;

  Widget* widget = new OwnershipTestWidget(&state);
  Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_POPUP);
  params.native_widget =
      new OwnershipTestNativeWidgetAura(widget, &state);
  widget->Init(params);

  // Now simulate a destroy of the platform native widget from the OS:
  SimulateNativeDestroy(widget);

  EXPECT_TRUE(state.widget_deleted);
  EXPECT_TRUE(state.native_widget_deleted);
}

// NativeWidget owns its Widget, part 4: NativeWidget is a NativeWidget,
// destroyed by the view hierarchy that contains it.
TEST_F(WidgetOwnershipTest,
       Ownership_ViewsNativeWidgetOwnsWidget_NativeDestroy) {
  OwnershipTestState state;

  Widget* toplevel = CreateTopLevelPlatformWidget();

  Widget* widget = new OwnershipTestWidget(&state);
  Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_POPUP);
  params.native_widget =
      new OwnershipTestNativeWidgetAura(widget, &state);
  params.parent = toplevel->GetNativeView();
  widget->Init(params);

  // Destroy the widget (achieved by closing the toplevel).
  toplevel->CloseNow();

  // The NativeWidget won't be deleted until after a return to the message loop
  // so we have to run pending messages before testing the destruction status.
  RunPendingMessages();

  EXPECT_TRUE(state.widget_deleted);
  EXPECT_TRUE(state.native_widget_deleted);
}

// NativeWidget owns its Widget, part 5: NativeWidget is a NativeWidget,
// we close it directly.
TEST_F(WidgetOwnershipTest,
       Ownership_ViewsNativeWidgetOwnsWidget_Close) {
  OwnershipTestState state;

  Widget* toplevel = CreateTopLevelPlatformWidget();

  Widget* widget = new OwnershipTestWidget(&state);
  Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_POPUP);
  params.native_widget =
      new OwnershipTestNativeWidgetAura(widget, &state);
  params.parent = toplevel->GetNativeView();
  widget->Init(params);

  // Destroy the widget.
  widget->Close();
  toplevel->CloseNow();

  // The NativeWidget won't be deleted until after a return to the message loop
  // so we have to run pending messages before testing the destruction status.
  RunPendingMessages();

  EXPECT_TRUE(state.widget_deleted);
  EXPECT_TRUE(state.native_widget_deleted);
}

// Widget owns its NativeWidget and has a WidgetDelegateView as its contents.
TEST_F(WidgetOwnershipTest,
       Ownership_WidgetOwnsNativeWidgetWithWithWidgetDelegateView) {
  OwnershipTestState state;

  WidgetDelegateView* delegate_view = new WidgetDelegateView;

  scoped_ptr<Widget> widget(new OwnershipTestWidget(&state));
  Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_POPUP);
  params.native_widget =
      new OwnershipTestNativeWidgetAura(widget.get(), &state);
  params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.delegate = delegate_view;
  widget->Init(params);
  widget->SetContentsView(delegate_view);

  // Now delete the Widget. There should be no crash or use-after-free.
  widget.reset();

  EXPECT_TRUE(state.widget_deleted);
  EXPECT_TRUE(state.native_widget_deleted);
}

////////////////////////////////////////////////////////////////////////////////
// Test to verify using various Widget methods doesn't crash when the underlying
// NativeView is destroyed.
//
class WidgetWithDestroyedNativeViewTest : public ViewsTestBase {
 public:
  WidgetWithDestroyedNativeViewTest() {}
  virtual ~WidgetWithDestroyedNativeViewTest() {}

  void InvokeWidgetMethods(Widget* widget) {
    widget->GetNativeView();
    widget->GetNativeWindow();
    ui::Accelerator accelerator;
    widget->GetAccelerator(0, &accelerator);
    widget->GetTopLevelWidget();
    widget->GetWindowBoundsInScreen();
    widget->GetClientAreaBoundsInScreen();
    widget->SetBounds(gfx::Rect(0, 0, 100, 80));
    widget->SetSize(gfx::Size(10, 11));
    widget->SetBoundsConstrained(gfx::Rect(0, 0, 120, 140));
    widget->SetVisibilityChangedAnimationsEnabled(false);
    widget->StackAtTop();
    widget->IsClosed();
    widget->Close();
    widget->Hide();
    widget->Activate();
    widget->Deactivate();
    widget->IsActive();
    widget->DisableInactiveRendering();
    widget->SetAlwaysOnTop(true);
    widget->IsAlwaysOnTop();
    widget->Maximize();
    widget->Minimize();
    widget->Restore();
    widget->IsMaximized();
    widget->IsFullscreen();
    widget->SetOpacity(0);
    widget->SetUseDragFrame(true);
    widget->FlashFrame(true);
    widget->IsVisible();
    widget->GetThemeProvider();
    widget->GetNativeTheme();
    widget->GetFocusManager();
    widget->GetInputMethod();
    widget->SchedulePaintInRect(gfx::Rect(0, 0, 1, 2));
    widget->IsMouseEventsEnabled();
    widget->SetNativeWindowProperty("xx", widget);
    widget->GetNativeWindowProperty("xx");
    widget->GetFocusTraversable();
    widget->GetLayer();
    widget->ReorderNativeViews();
    widget->SetCapture(widget->GetRootView());
    widget->ReleaseCapture();
    widget->HasCapture();
    widget->GetWorkAreaBoundsInScreen();
    widget->IsTranslucentWindowOpacitySupported();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(WidgetWithDestroyedNativeViewTest);
};

TEST_F(WidgetWithDestroyedNativeViewTest, Test) {
  {
    Widget widget;
    Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_POPUP);
    params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    widget.Init(params);
    widget.Show();

    widget.native_widget_private()->CloseNow();
    InvokeWidgetMethods(&widget);
  }
#if !defined(OS_CHROMEOS)
  {
    Widget widget;
    Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_POPUP);
    params.native_widget = new PlatformDesktopNativeWidget(&widget);
    params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    widget.Init(params);
    widget.Show();

    widget.native_widget_private()->CloseNow();
    InvokeWidgetMethods(&widget);
  }
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Widget observer tests.
//

class WidgetObserverTest : public WidgetTest, public WidgetObserver {
 public:
  WidgetObserverTest()
      : active_(NULL),
        widget_closed_(NULL),
        widget_activated_(NULL),
        widget_shown_(NULL),
        widget_hidden_(NULL),
        widget_bounds_changed_(NULL) {
  }

  virtual ~WidgetObserverTest() {}

  // Overridden from WidgetObserver:
  virtual void OnWidgetDestroying(Widget* widget) OVERRIDE {
    if (active_ == widget)
      active_ = NULL;
    widget_closed_ = widget;
  }

  virtual void OnWidgetActivationChanged(Widget* widget,
                                         bool active) OVERRIDE {
    if (active) {
      if (widget_activated_)
        widget_activated_->Deactivate();
      widget_activated_ = widget;
      active_ = widget;
    } else {
      if (widget_activated_ == widget)
        widget_activated_ = NULL;
      widget_deactivated_ = widget;
    }
  }

  virtual void OnWidgetVisibilityChanged(Widget* widget,
                                         bool visible) OVERRIDE {
    if (visible)
      widget_shown_ = widget;
    else
      widget_hidden_ = widget;
  }

  virtual void OnWidgetBoundsChanged(Widget* widget,
                                     const gfx::Rect& new_bounds) OVERRIDE {
    widget_bounds_changed_ = widget;
  }

  void reset() {
    active_ = NULL;
    widget_closed_ = NULL;
    widget_activated_ = NULL;
    widget_deactivated_ = NULL;
    widget_shown_ = NULL;
    widget_hidden_ = NULL;
    widget_bounds_changed_ = NULL;
  }

  Widget* NewWidget() {
    Widget* widget = CreateTopLevelNativeWidget();
    widget->AddObserver(this);
    return widget;
  }

  const Widget* active() const { return active_; }
  const Widget* widget_closed() const { return widget_closed_; }
  const Widget* widget_activated() const { return widget_activated_; }
  const Widget* widget_deactivated() const { return widget_deactivated_; }
  const Widget* widget_shown() const { return widget_shown_; }
  const Widget* widget_hidden() const { return widget_hidden_; }
  const Widget* widget_bounds_changed() const { return widget_bounds_changed_; }

 private:
  Widget* active_;

  Widget* widget_closed_;
  Widget* widget_activated_;
  Widget* widget_deactivated_;
  Widget* widget_shown_;
  Widget* widget_hidden_;
  Widget* widget_bounds_changed_;
};

TEST_F(WidgetObserverTest, DISABLED_ActivationChange) {
  Widget* toplevel = CreateTopLevelPlatformWidget();

  Widget* toplevel1 = NewWidget();
  Widget* toplevel2 = NewWidget();

  toplevel1->Show();
  toplevel2->Show();

  reset();

  toplevel1->Activate();

  RunPendingMessages();
  EXPECT_EQ(toplevel1, widget_activated());

  toplevel2->Activate();
  RunPendingMessages();
  EXPECT_EQ(toplevel1, widget_deactivated());
  EXPECT_EQ(toplevel2, widget_activated());
  EXPECT_EQ(toplevel2, active());

  toplevel->CloseNow();
}

TEST_F(WidgetObserverTest, DISABLED_VisibilityChange) {
  Widget* toplevel = CreateTopLevelPlatformWidget();

  Widget* child1 = NewWidget();
  Widget* child2 = NewWidget();

  toplevel->Show();
  child1->Show();
  child2->Show();

  reset();

  child1->Hide();
  EXPECT_EQ(child1, widget_hidden());

  child2->Hide();
  EXPECT_EQ(child2, widget_hidden());

  child1->Show();
  EXPECT_EQ(child1, widget_shown());

  child2->Show();
  EXPECT_EQ(child2, widget_shown());

  toplevel->CloseNow();
}

TEST_F(WidgetObserverTest, DestroyBubble) {
  Widget* anchor = CreateTopLevelPlatformWidget();
  anchor->Show();

  BubbleDelegateView* bubble_delegate =
      new BubbleDelegateView(anchor->client_view(), BubbleBorder::NONE);
  Widget* bubble_widget(BubbleDelegateView::CreateBubble(bubble_delegate));
  bubble_widget->Show();
  bubble_widget->CloseNow();

  anchor->Hide();
  anchor->CloseNow();
}

TEST_F(WidgetObserverTest, WidgetBoundsChanged) {
  Widget* child1 = NewWidget();
  Widget* child2 = NewWidget();

  child1->OnNativeWidgetMove();
  EXPECT_EQ(child1, widget_bounds_changed());

  child2->OnNativeWidgetMove();
  EXPECT_EQ(child2, widget_bounds_changed());

  child1->OnNativeWidgetSizeChanged(gfx::Size());
  EXPECT_EQ(child1, widget_bounds_changed());

  child2->OnNativeWidgetSizeChanged(gfx::Size());
  EXPECT_EQ(child2, widget_bounds_changed());
}

// Tests that SetBounds() and GetWindowBoundsInScreen() is symmetric when the
// widget is visible and not maximized or fullscreen.
TEST_F(WidgetTest, GetWindowBoundsInScreen) {
  // Choose test coordinates away from edges and dimensions that are "small"
  // (but not too small) to ensure the OS doesn't try to adjust them.
  const gfx::Rect kTestBounds(150, 150, 400, 300);
  const gfx::Size kTestSize(200, 180);

  // First test a toplevel widget.
  Widget* widget = CreateTopLevelPlatformWidget();
  widget->Show();

  EXPECT_NE(kTestSize.ToString(),
            widget->GetWindowBoundsInScreen().size().ToString());
  widget->SetSize(kTestSize);
  EXPECT_EQ(kTestSize.ToString(),
            widget->GetWindowBoundsInScreen().size().ToString());

  EXPECT_NE(kTestBounds.ToString(),
            widget->GetWindowBoundsInScreen().ToString());
  widget->SetBounds(kTestBounds);
  EXPECT_EQ(kTestBounds.ToString(),
            widget->GetWindowBoundsInScreen().ToString());

  // Changing just the size should not change the origin.
  widget->SetSize(kTestSize);
  EXPECT_EQ(kTestBounds.origin().ToString(),
            widget->GetWindowBoundsInScreen().origin().ToString());

  widget->CloseNow();

  // Same tests with a frameless window.
  widget = CreateTopLevelFramelessPlatformWidget();
  widget->Show();

  EXPECT_NE(kTestSize.ToString(),
            widget->GetWindowBoundsInScreen().size().ToString());
  widget->SetSize(kTestSize);
  EXPECT_EQ(kTestSize.ToString(),
            widget->GetWindowBoundsInScreen().size().ToString());

  EXPECT_NE(kTestBounds.ToString(),
            widget->GetWindowBoundsInScreen().ToString());
  widget->SetBounds(kTestBounds);
  EXPECT_EQ(kTestBounds.ToString(),
            widget->GetWindowBoundsInScreen().ToString());

  // For a frameless widget, the client bounds should also match.
  EXPECT_EQ(kTestBounds.ToString(),
            widget->GetClientAreaBoundsInScreen().ToString());

  // Verify origin is stable for a frameless window as well.
  widget->SetSize(kTestSize);
  EXPECT_EQ(kTestBounds.origin().ToString(),
            widget->GetWindowBoundsInScreen().origin().ToString());

  widget->CloseNow();
}

#if defined(false)
// Aura needs shell to maximize/fullscreen window.
// NativeWidgetGtk doesn't implement GetRestoredBounds.
TEST_F(WidgetTest, GetRestoredBounds) {
  Widget* toplevel = CreateTopLevelPlatformWidget();
  EXPECT_EQ(toplevel->GetWindowBoundsInScreen().ToString(),
            toplevel->GetRestoredBounds().ToString());
  toplevel->Show();
  toplevel->Maximize();
  RunPendingMessages();
  EXPECT_NE(toplevel->GetWindowBoundsInScreen().ToString(),
            toplevel->GetRestoredBounds().ToString());
  EXPECT_GT(toplevel->GetRestoredBounds().width(), 0);
  EXPECT_GT(toplevel->GetRestoredBounds().height(), 0);

  toplevel->Restore();
  RunPendingMessages();
  EXPECT_EQ(toplevel->GetWindowBoundsInScreen().ToString(),
            toplevel->GetRestoredBounds().ToString());

  toplevel->SetFullscreen(true);
  RunPendingMessages();
  EXPECT_NE(toplevel->GetWindowBoundsInScreen().ToString(),
            toplevel->GetRestoredBounds().ToString());
  EXPECT_GT(toplevel->GetRestoredBounds().width(), 0);
  EXPECT_GT(toplevel->GetRestoredBounds().height(), 0);
}
#endif

// Test that window state is not changed after getting out of full screen.
TEST_F(WidgetTest, ExitFullscreenRestoreState) {
  Widget* toplevel = CreateTopLevelPlatformWidget();

  toplevel->Show();
  RunPendingMessages();

  // This should be a normal state window.
  EXPECT_EQ(ui::SHOW_STATE_NORMAL, GetWidgetShowState(toplevel));

  toplevel->SetFullscreen(true);
  EXPECT_EQ(ui::SHOW_STATE_FULLSCREEN, GetWidgetShowState(toplevel));
  toplevel->SetFullscreen(false);
  EXPECT_NE(ui::SHOW_STATE_FULLSCREEN, GetWidgetShowState(toplevel));

  // And it should still be in normal state after getting out of full screen.
  EXPECT_EQ(ui::SHOW_STATE_NORMAL, GetWidgetShowState(toplevel));

  // Now, make it maximized.
  toplevel->Maximize();
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED, GetWidgetShowState(toplevel));

  toplevel->SetFullscreen(true);
  EXPECT_EQ(ui::SHOW_STATE_FULLSCREEN, GetWidgetShowState(toplevel));
  toplevel->SetFullscreen(false);
  EXPECT_NE(ui::SHOW_STATE_FULLSCREEN, GetWidgetShowState(toplevel));

  // And it stays maximized after getting out of full screen.
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED, GetWidgetShowState(toplevel));

  // Clean up.
  toplevel->Close();
  RunPendingMessages();
}

// The key-event propagation from Widget happens differently on aura and
// non-aura systems because of the difference in IME. So this test works only on
// aura.
TEST_F(WidgetTest, KeyboardInputEvent) {
  Widget* toplevel = CreateTopLevelPlatformWidget();
  View* container = toplevel->client_view();

  Textfield* textfield = new Textfield();
  textfield->SetText(base::ASCIIToUTF16("some text"));
  container->AddChildView(textfield);
  toplevel->Show();
  textfield->RequestFocus();

  // The press gets handled. The release doesn't have an effect.
  ui::KeyEvent backspace_p(ui::ET_KEY_PRESSED, ui::VKEY_DELETE, ui::EF_NONE);
  toplevel->OnKeyEvent(&backspace_p);
  EXPECT_TRUE(backspace_p.stopped_propagation());
  ui::KeyEvent backspace_r(ui::ET_KEY_RELEASED, ui::VKEY_DELETE, ui::EF_NONE);
  toplevel->OnKeyEvent(&backspace_r);
  EXPECT_FALSE(backspace_r.handled());

  toplevel->Close();
}

// Verifies bubbles result in a focus lost when shown.
// TODO(msw): this tests relies on focus, it needs to be in
// interactive_ui_tests.
TEST_F(WidgetTest, DISABLED_FocusChangesOnBubble) {
  // Create a widget, show and activate it and focus the contents view.
  View* contents_view = new View;
  contents_view->SetFocusable(true);
  Widget widget;
  Widget::InitParams init_params =
      CreateParams(Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  init_params.bounds = gfx::Rect(0, 0, 200, 200);
  init_params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
#if !defined(OS_CHROMEOS)
  init_params.native_widget = new PlatformDesktopNativeWidget(&widget);
#endif
  widget.Init(init_params);
  widget.SetContentsView(contents_view);
  widget.Show();
  widget.Activate();
  contents_view->RequestFocus();
  EXPECT_TRUE(contents_view->HasFocus());

  // Show a bubble.
  BubbleDelegateView* bubble_delegate_view =
      new BubbleDelegateView(contents_view, BubbleBorder::TOP_LEFT);
  bubble_delegate_view->SetFocusable(true);
  BubbleDelegateView::CreateBubble(bubble_delegate_view)->Show();
  bubble_delegate_view->RequestFocus();

  // |contents_view_| should no longer have focus.
  EXPECT_FALSE(contents_view->HasFocus());
  EXPECT_TRUE(bubble_delegate_view->HasFocus());

  bubble_delegate_view->GetWidget()->CloseNow();

  // Closing the bubble should result in focus going back to the contents view.
  EXPECT_TRUE(contents_view->HasFocus());
}

class TestBubbleDelegateView : public BubbleDelegateView {
 public:
  TestBubbleDelegateView(View* anchor)
      : BubbleDelegateView(anchor, BubbleBorder::NONE),
        reset_controls_called_(false) {}
  virtual ~TestBubbleDelegateView() {}

  virtual bool ShouldShowCloseButton() const OVERRIDE {
    reset_controls_called_ = true;
    return true;
  }

  mutable bool reset_controls_called_;
};

TEST_F(WidgetTest, BubbleControlsResetOnInit) {
  Widget* anchor = CreateTopLevelPlatformWidget();
  anchor->Show();

  TestBubbleDelegateView* bubble_delegate =
      new TestBubbleDelegateView(anchor->client_view());
  Widget* bubble_widget(BubbleDelegateView::CreateBubble(bubble_delegate));
  EXPECT_TRUE(bubble_delegate->reset_controls_called_);
  bubble_widget->Show();
  bubble_widget->CloseNow();

  anchor->Hide();
  anchor->CloseNow();
}

// Desktop native widget Aura tests are for non Chrome OS platforms.
#if !defined(OS_CHROMEOS)
// Test to ensure that after minimize, view width is set to zero.
TEST_F(WidgetTest, TestViewWidthAfterMinimizingWidget) {
  // Create a widget.
  Widget widget;
  Widget::InitParams init_params =
      CreateParams(Widget::InitParams::TYPE_WINDOW);
  init_params.show_state = ui::SHOW_STATE_NORMAL;
  gfx::Rect initial_bounds(0, 0, 300, 400);
  init_params.bounds = initial_bounds;
  init_params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  init_params.native_widget = new PlatformDesktopNativeWidget(&widget);
  widget.Init(init_params);
  NonClientView* non_client_view = widget.non_client_view();
  NonClientFrameView* frame_view = new MinimumSizeFrameView(&widget);
  non_client_view->SetFrameView(frame_view);
  widget.Show();
  widget.Minimize();
  EXPECT_EQ(0, frame_view->width());
}

// This class validates whether paints are received for a visible Widget.
// To achieve this it overrides the Show and Close methods on the Widget class
// and sets state whether subsequent paints are expected.
class DesktopAuraTestValidPaintWidget : public views::Widget {
 public:
  DesktopAuraTestValidPaintWidget()
    : expect_paint_(true),
      received_paint_while_hidden_(false) {
  }

  virtual ~DesktopAuraTestValidPaintWidget() {
  }

  virtual void Show() OVERRIDE {
    expect_paint_ = true;
    views::Widget::Show();
  }

  virtual void Close() OVERRIDE {
    expect_paint_ = false;
    views::Widget::Close();
  }

  void Hide() {
    expect_paint_ = false;
    views::Widget::Hide();
  }

  virtual void OnNativeWidgetPaint(gfx::Canvas* canvas) OVERRIDE {
    EXPECT_TRUE(expect_paint_);
    if (!expect_paint_)
      received_paint_while_hidden_ = true;
    views::Widget::OnNativeWidgetPaint(canvas);
  }

  bool received_paint_while_hidden() const {
    return received_paint_while_hidden_;
  }

 private:
  bool expect_paint_;
  bool received_paint_while_hidden_;
};

TEST_F(WidgetTest, DesktopNativeWidgetNoPaintAfterCloseTest) {
  View* contents_view = new View;
  contents_view->SetFocusable(true);
  DesktopAuraTestValidPaintWidget widget;
  Widget::InitParams init_params =
      CreateParams(Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  init_params.bounds = gfx::Rect(0, 0, 200, 200);
  init_params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  init_params.native_widget = new PlatformDesktopNativeWidget(&widget);
  widget.Init(init_params);
  widget.SetContentsView(contents_view);
  widget.Show();
  widget.Activate();
  RunPendingMessages();
  widget.SchedulePaintInRect(init_params.bounds);
  widget.Close();
  RunPendingMessages();
  EXPECT_FALSE(widget.received_paint_while_hidden());
}

TEST_F(WidgetTest, DesktopNativeWidgetNoPaintAfterHideTest) {
  View* contents_view = new View;
  contents_view->SetFocusable(true);
  DesktopAuraTestValidPaintWidget widget;
  Widget::InitParams init_params =
      CreateParams(Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  init_params.bounds = gfx::Rect(0, 0, 200, 200);
  init_params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  init_params.native_widget = new PlatformDesktopNativeWidget(&widget);
  widget.Init(init_params);
  widget.SetContentsView(contents_view);
  widget.Show();
  widget.Activate();
  RunPendingMessages();
  widget.SchedulePaintInRect(init_params.bounds);
  widget.Hide();
  RunPendingMessages();
  EXPECT_FALSE(widget.received_paint_while_hidden());
  widget.Close();
}

// Test to ensure that the aura Window's visiblity state is set to visible if
// the underlying widget is hidden and then shown.
TEST_F(WidgetTest, TestWindowVisibilityAfterHide) {
  // Create a widget.
  Widget widget;
  Widget::InitParams init_params =
      CreateParams(Widget::InitParams::TYPE_WINDOW);
  init_params.show_state = ui::SHOW_STATE_NORMAL;
  gfx::Rect initial_bounds(0, 0, 300, 400);
  init_params.bounds = initial_bounds;
  init_params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  init_params.native_widget = new PlatformDesktopNativeWidget(&widget);
  widget.Init(init_params);
  NonClientView* non_client_view = widget.non_client_view();
  NonClientFrameView* frame_view = new MinimumSizeFrameView(&widget);
  non_client_view->SetFrameView(frame_view);

  widget.Hide();
  EXPECT_FALSE(IsNativeWindowVisible(widget.GetNativeWindow()));
  widget.Show();
  EXPECT_TRUE(IsNativeWindowVisible(widget.GetNativeWindow()));
}

// The following code verifies we can correctly destroy a Widget from a mouse
// enter/exit. We could test move/drag/enter/exit but in general we don't run
// nested message loops from such events, nor has the code ever really dealt
// with this situation.

// Generates two moves (first generates enter, second real move), a press, drag
// and release stopping at |last_event_type|.
void GenerateMouseEvents(Widget* widget, ui::EventType last_event_type) {
  const gfx::Rect screen_bounds(widget->GetWindowBoundsInScreen());
  ui::MouseEvent move_event(ui::ET_MOUSE_MOVED, screen_bounds.CenterPoint(),
                            screen_bounds.CenterPoint(), 0, 0);
  ui::EventProcessor* dispatcher = WidgetTest::GetEventProcessor(widget);
  ui::EventDispatchDetails details = dispatcher->OnEventFromSource(&move_event);
  if (last_event_type == ui::ET_MOUSE_ENTERED || details.dispatcher_destroyed)
    return;
  details = dispatcher->OnEventFromSource(&move_event);
  if (last_event_type == ui::ET_MOUSE_MOVED || details.dispatcher_destroyed)
    return;

  ui::MouseEvent press_event(ui::ET_MOUSE_PRESSED, screen_bounds.CenterPoint(),
                             screen_bounds.CenterPoint(), 0, 0);
  details = dispatcher->OnEventFromSource(&press_event);
  if (last_event_type == ui::ET_MOUSE_PRESSED || details.dispatcher_destroyed)
    return;

  gfx::Point end_point(screen_bounds.CenterPoint());
  end_point.Offset(1, 1);
  ui::MouseEvent drag_event(ui::ET_MOUSE_DRAGGED, end_point, end_point, 0, 0);
  details = dispatcher->OnEventFromSource(&drag_event);
  if (last_event_type == ui::ET_MOUSE_DRAGGED || details.dispatcher_destroyed)
    return;

  ui::MouseEvent release_event(ui::ET_MOUSE_RELEASED, end_point, end_point, 0,
                               0);
  details = dispatcher->OnEventFromSource(&release_event);
  if (details.dispatcher_destroyed)
    return;
}

// Creates a widget and invokes GenerateMouseEvents() with |last_event_type|.
void RunCloseWidgetDuringDispatchTest(WidgetTest* test,
                                      ui::EventType last_event_type) {
  // |widget| is deleted by CloseWidgetView.
  Widget* widget = new Widget;
  Widget::InitParams params =
      test->CreateParams(Widget::InitParams::TYPE_POPUP);
  params.native_widget = new PlatformDesktopNativeWidget(widget);
  params.bounds = gfx::Rect(0, 0, 50, 100);
  widget->Init(params);
  widget->SetContentsView(new CloseWidgetView(last_event_type));
  widget->Show();
  GenerateMouseEvents(widget, last_event_type);
}

// Verifies deleting the widget from a mouse pressed event doesn't crash.
TEST_F(WidgetTest, CloseWidgetDuringMousePress) {
  RunCloseWidgetDuringDispatchTest(this, ui::ET_MOUSE_PRESSED);
}

// Verifies deleting the widget from a mouse released event doesn't crash.
TEST_F(WidgetTest, CloseWidgetDuringMouseReleased) {
  RunCloseWidgetDuringDispatchTest(this, ui::ET_MOUSE_RELEASED);
}

#endif  // !defined(OS_CHROMEOS)

// Tests that wheel events generated from scroll events are targetted to the
// views under the cursor when the focused view does not processed them.
TEST_F(WidgetTest, WheelEventsFromScrollEventTarget) {
  EventCountView* cursor_view = new EventCountView;
  cursor_view->SetBounds(60, 0, 50, 40);

  Widget* widget = CreateTopLevelPlatformWidget();
  widget->GetRootView()->AddChildView(cursor_view);

  // Generate a scroll event on the cursor view.
  ui::ScrollEvent scroll(ui::ET_SCROLL,
                         gfx::Point(65, 5),
                         ui::EventTimeForNow(),
                         0,
                         0, 20,
                         0, 20,
                         2);
  widget->OnScrollEvent(&scroll);

  EXPECT_EQ(1, cursor_view->GetEventCount(ui::ET_SCROLL));
  EXPECT_EQ(1, cursor_view->GetEventCount(ui::ET_MOUSEWHEEL));

  cursor_view->ResetCounts();

  ui::ScrollEvent scroll2(ui::ET_SCROLL,
                          gfx::Point(5, 5),
                          ui::EventTimeForNow(),
                          0,
                          0, 20,
                          0, 20,
                          2);
  widget->OnScrollEvent(&scroll2);

  EXPECT_EQ(0, cursor_view->GetEventCount(ui::ET_SCROLL));
  EXPECT_EQ(0, cursor_view->GetEventCount(ui::ET_MOUSEWHEEL));

  widget->CloseNow();
}

// Tests that if a scroll-begin gesture is not handled, then subsequent scroll
// events are not dispatched to any view.
TEST_F(WidgetTest, GestureScrollEventDispatching) {
  EventCountView* noscroll_view = new EventCountView;
  EventCountView* scroll_view = new ScrollableEventCountView;

  noscroll_view->SetBounds(0, 0, 50, 40);
  scroll_view->SetBounds(60, 0, 40, 40);

  Widget* widget = CreateTopLevelPlatformWidget();
  widget->GetRootView()->AddChildView(noscroll_view);
  widget->GetRootView()->AddChildView(scroll_view);

  {
    ui::GestureEvent begin(
        5,
        5,
        0,
        base::TimeDelta(),
        ui::GestureEventDetails(ui::ET_GESTURE_SCROLL_BEGIN, 0, 0));
    widget->OnGestureEvent(&begin);
    ui::GestureEvent update(
        25,
        15,
        0,
        base::TimeDelta(),
        ui::GestureEventDetails(ui::ET_GESTURE_SCROLL_UPDATE, 20, 10));
    widget->OnGestureEvent(&update);
    ui::GestureEvent end(
        25,
        15,
        0,
        base::TimeDelta(),
        ui::GestureEventDetails(ui::ET_GESTURE_SCROLL_END, 0, 0));
    widget->OnGestureEvent(&end);

    EXPECT_EQ(1, noscroll_view->GetEventCount(ui::ET_GESTURE_SCROLL_BEGIN));
    EXPECT_EQ(0, noscroll_view->GetEventCount(ui::ET_GESTURE_SCROLL_UPDATE));
    EXPECT_EQ(0, noscroll_view->GetEventCount(ui::ET_GESTURE_SCROLL_END));
  }

  {
    ui::GestureEvent begin(
        65,
        5,
        0,
        base::TimeDelta(),
        ui::GestureEventDetails(ui::ET_GESTURE_SCROLL_BEGIN, 0, 0));
    widget->OnGestureEvent(&begin);
    ui::GestureEvent update(
        85,
        15,
        0,
        base::TimeDelta(),
        ui::GestureEventDetails(ui::ET_GESTURE_SCROLL_UPDATE, 20, 10));
    widget->OnGestureEvent(&update);
    ui::GestureEvent end(
        85,
        15,
        0,
        base::TimeDelta(),
        ui::GestureEventDetails(ui::ET_GESTURE_SCROLL_END, 0, 0));
    widget->OnGestureEvent(&end);

    EXPECT_EQ(1, scroll_view->GetEventCount(ui::ET_GESTURE_SCROLL_BEGIN));
    EXPECT_EQ(1, scroll_view->GetEventCount(ui::ET_GESTURE_SCROLL_UPDATE));
    EXPECT_EQ(1, scroll_view->GetEventCount(ui::ET_GESTURE_SCROLL_END));
  }

  widget->CloseNow();
}

// Tests that event-handlers installed on the RootView get triggered correctly.
// TODO(tdanderson): Clean up this test as part of crbug.com/355680.
TEST_F(WidgetTest, EventHandlersOnRootView) {
  Widget* widget = CreateTopLevelNativeWidget();
  View* root_view = widget->GetRootView();

  scoped_ptr<EventCountView> view(new EventCountView());
  view->set_owned_by_client();
  view->SetBounds(0, 0, 20, 20);
  root_view->AddChildView(view.get());

  EventCountHandler h1;
  root_view->AddPreTargetHandler(&h1);

  EventCountHandler h2;
  root_view->AddPostTargetHandler(&h2);

  widget->SetBounds(gfx::Rect(0, 0, 100, 100));
  widget->Show();

  // Dispatch a ui::ET_SCROLL event. The event remains unhandled and should
  // bubble up the views hierarchy to be re-dispatched on the root view.
  ui::ScrollEvent scroll(ui::ET_SCROLL,
                         gfx::Point(5, 5),
                         ui::EventTimeForNow(),
                         0,
                         0, 20,
                         0, 20,
                         2);
  widget->OnScrollEvent(&scroll);
  EXPECT_EQ(2, h1.GetEventCount(ui::ET_SCROLL));
  EXPECT_EQ(1, view->GetEventCount(ui::ET_SCROLL));
  EXPECT_EQ(2, h2.GetEventCount(ui::ET_SCROLL));

  // Unhandled scroll events are turned into wheel events and re-dispatched.
  EXPECT_EQ(1, h1.GetEventCount(ui::ET_MOUSEWHEEL));
  EXPECT_EQ(1, view->GetEventCount(ui::ET_MOUSEWHEEL));
  EXPECT_EQ(1, h2.GetEventCount(ui::ET_MOUSEWHEEL));

  h1.ResetCounts();
  view->ResetCounts();
  h2.ResetCounts();

  // Dispatch a ui::ET_SCROLL_FLING_START event. The event remains unhandled and
  // should bubble up the views hierarchy to be re-dispatched on the root view.
  ui::ScrollEvent fling(ui::ET_SCROLL_FLING_START,
                        gfx::Point(5, 5),
                        ui::EventTimeForNow(),
                        0,
                        0, 20,
                        0, 20,
                        2);
  widget->OnScrollEvent(&fling);
  EXPECT_EQ(2, h1.GetEventCount(ui::ET_SCROLL_FLING_START));
  EXPECT_EQ(1, view->GetEventCount(ui::ET_SCROLL_FLING_START));
  EXPECT_EQ(2, h2.GetEventCount(ui::ET_SCROLL_FLING_START));

  // Unhandled scroll events which are not of type ui::ET_SCROLL should not
  // be turned into wheel events and re-dispatched.
  EXPECT_EQ(0, h1.GetEventCount(ui::ET_MOUSEWHEEL));
  EXPECT_EQ(0, view->GetEventCount(ui::ET_MOUSEWHEEL));
  EXPECT_EQ(0, h2.GetEventCount(ui::ET_MOUSEWHEEL));

  h1.ResetCounts();
  view->ResetCounts();
  h2.ResetCounts();

  // Change the handle mode of |view| so that events are marked as handled at
  // the target phase.
  view->set_handle_mode(EventCountView::CONSUME_EVENTS);

  // Dispatch a ui::ET_GESTURE_TAP_DOWN and a ui::ET_GESTURE_TAP_CANCEL event.
  // The events are handled at the target phase and should not reach the
  // post-target handler.
  ui::GestureEvent tap_down(5,
                            5,
                            0,
                            ui::EventTimeForNow(),
                            ui::GestureEventDetails(ui::ET_GESTURE_TAP_DOWN,
                                                    0,
                                                    0));
  widget->OnGestureEvent(&tap_down);
  EXPECT_EQ(1, h1.GetEventCount(ui::ET_GESTURE_TAP_DOWN));
  EXPECT_EQ(1, view->GetEventCount(ui::ET_GESTURE_TAP_DOWN));
  EXPECT_EQ(0, h2.GetEventCount(ui::ET_GESTURE_TAP_DOWN));

  ui::GestureEvent tap_cancel(5,
                              5,
                              0,
                              ui::EventTimeForNow(),
                              ui::GestureEventDetails(ui::ET_GESTURE_TAP_CANCEL,
                                                      0,
                                                      0));
  widget->OnGestureEvent(&tap_cancel);
  EXPECT_EQ(1, h1.GetEventCount(ui::ET_GESTURE_TAP_CANCEL));
  EXPECT_EQ(1, view->GetEventCount(ui::ET_GESTURE_TAP_CANCEL));
  EXPECT_EQ(0, h2.GetEventCount(ui::ET_GESTURE_TAP_CANCEL));

  h1.ResetCounts();
  view->ResetCounts();
  h2.ResetCounts();

  // Dispatch a ui::ET_SCROLL event. The event is handled at the target phase
  // and should not reach the post-target handler.
  ui::ScrollEvent consumed_scroll(ui::ET_SCROLL,
                                  gfx::Point(5, 5),
                                  ui::EventTimeForNow(),
                                  0,
                                  0, 20,
                                  0, 20,
                                  2);
  widget->OnScrollEvent(&consumed_scroll);
  EXPECT_EQ(1, h1.GetEventCount(ui::ET_SCROLL));
  EXPECT_EQ(1, view->GetEventCount(ui::ET_SCROLL));
  EXPECT_EQ(0, h2.GetEventCount(ui::ET_SCROLL));

  // Handled scroll events are not turned into wheel events and re-dispatched.
  EXPECT_EQ(0, h1.GetEventCount(ui::ET_MOUSEWHEEL));
  EXPECT_EQ(0, view->GetEventCount(ui::ET_MOUSEWHEEL));
  EXPECT_EQ(0, h2.GetEventCount(ui::ET_MOUSEWHEEL));

  widget->CloseNow();
}

TEST_F(WidgetTest, SynthesizeMouseMoveEvent) {
  Widget* widget = CreateTopLevelNativeWidget();
  View* root_view = widget->GetRootView();

  EventCountView* v1 = new EventCountView();
  v1->SetBounds(0, 0, 10, 10);
  root_view->AddChildView(v1);
  EventCountView* v2 = new EventCountView();
  v2->SetBounds(0, 10, 10, 10);
  root_view->AddChildView(v2);

  gfx::Point cursor_location(5, 5);
  ui::MouseEvent move(ui::ET_MOUSE_MOVED, cursor_location, cursor_location,
                      ui::EF_NONE, ui::EF_NONE);
  widget->OnMouseEvent(&move);

  EXPECT_EQ(1, v1->GetEventCount(ui::ET_MOUSE_ENTERED));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_MOUSE_ENTERED));

  delete v1;
  v2->SetBounds(0, 0, 10, 10);
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_MOUSE_ENTERED));

  widget->SynthesizeMouseMoveEvent();
  EXPECT_EQ(1, v2->GetEventCount(ui::ET_MOUSE_ENTERED));
}

namespace {

// ui::EventHandler which handles all mouse press events.
class MousePressEventConsumer : public ui::EventHandler {
 public:
  explicit MousePressEventConsumer() {
  }

  virtual ~MousePressEventConsumer() {
  }

 private:
  // ui::EventHandler:
  virtual void OnMouseEvent(ui::MouseEvent* event) OVERRIDE {
    if (event->type() == ui::ET_MOUSE_PRESSED)
      event->SetHandled();
  }

  DISALLOW_COPY_AND_ASSIGN(MousePressEventConsumer);
};

}  // namespace

// Test that mouse presses and mouse releases are dispatched normally when a
// touch is down.
TEST_F(WidgetTest, MouseEventDispatchWhileTouchIsDown) {
  Widget* widget = CreateTopLevelNativeWidget();
  widget->Show();
  widget->SetSize(gfx::Size(300, 300));

  EventCountView* event_count_view = new EventCountView();
  event_count_view->SetBounds(0, 0, 300, 300);
  widget->GetRootView()->AddChildView(event_count_view);

  MousePressEventConsumer consumer;
  event_count_view->AddPostTargetHandler(&consumer);

  ui::test::EventGenerator generator(GetContext(), widget->GetNativeWindow());
  generator.PressTouch();
  generator.ClickLeftButton();

  EXPECT_EQ(1, event_count_view->GetEventCount(ui::ET_MOUSE_PRESSED));
  EXPECT_EQ(1, event_count_view->GetEventCount(ui::ET_MOUSE_RELEASED));

  widget->CloseNow();
}

// Used by SingleWindowClosing to count number of times WindowClosing() has
// been invoked.
class ClosingDelegate : public WidgetDelegate {
 public:
  ClosingDelegate() : count_(0), widget_(NULL) {}

  int count() const { return count_; }

  void set_widget(views::Widget* widget) { widget_ = widget; }

  // WidgetDelegate overrides:
  virtual Widget* GetWidget() OVERRIDE { return widget_; }
  virtual const Widget* GetWidget() const OVERRIDE { return widget_; }
  virtual void WindowClosing() OVERRIDE {
    count_++;
  }

 private:
  int count_;
  views::Widget* widget_;

  DISALLOW_COPY_AND_ASSIGN(ClosingDelegate);
};

// Verifies WindowClosing() is invoked correctly on the delegate when a Widget
// is closed.
TEST_F(WidgetTest, SingleWindowClosing) {
  scoped_ptr<ClosingDelegate> delegate(new ClosingDelegate());
  Widget* widget = new Widget();  // Destroyed by CloseNow() below.
  Widget::InitParams init_params =
      CreateParams(Widget::InitParams::TYPE_WINDOW);
  init_params.bounds = gfx::Rect(0, 0, 200, 200);
  init_params.delegate = delegate.get();
#if !defined(OS_CHROMEOS)
  init_params.native_widget = new PlatformDesktopNativeWidget(widget);
#endif
  widget->Init(init_params);
  EXPECT_EQ(0, delegate->count());
  widget->CloseNow();
  EXPECT_EQ(1, delegate->count());
}

class WidgetWindowTitleTest : public WidgetTest {
 protected:
  void RunTest(bool desktop_native_widget) {
    Widget* widget = new Widget();  // Destroyed by CloseNow() below.
    Widget::InitParams init_params =
        CreateParams(Widget::InitParams::TYPE_WINDOW);
    widget->Init(init_params);

#if !defined(OS_CHROMEOS)
    if (desktop_native_widget)
      init_params.native_widget = new PlatformDesktopNativeWidget(widget);
#else
    DCHECK(!desktop_native_widget)
        << "DesktopNativeWidget does not exist on non-Aura or on ChromeOS.";
#endif

    internal::NativeWidgetPrivate* native_widget =
        widget->native_widget_private();

    base::string16 empty;
    base::string16 s1(base::UTF8ToUTF16("Title1"));
    base::string16 s2(base::UTF8ToUTF16("Title2"));
    base::string16 s3(base::UTF8ToUTF16("TitleLong"));

    // The widget starts with no title, setting empty should not change
    // anything.
    EXPECT_FALSE(native_widget->SetWindowTitle(empty));
    // Setting the title to something non-empty should cause a change.
    EXPECT_TRUE(native_widget->SetWindowTitle(s1));
    // Setting the title to something else with the same length should cause a
    // change.
    EXPECT_TRUE(native_widget->SetWindowTitle(s2));
    // Setting the title to something else with a different length should cause
    // a change.
    EXPECT_TRUE(native_widget->SetWindowTitle(s3));
    // Setting the title to the same thing twice should not cause a change.
    EXPECT_FALSE(native_widget->SetWindowTitle(s3));

    widget->CloseNow();
  }
};

TEST_F(WidgetWindowTitleTest, SetWindowTitleChanged_NativeWidget) {
  // Use the default NativeWidget.
  bool desktop_native_widget = false;
  RunTest(desktop_native_widget);
}

// DesktopNativeWidget does not exist on non-Aura or on ChromeOS.
#if !defined(OS_CHROMEOS)
TEST_F(WidgetWindowTitleTest, SetWindowTitleChanged_DesktopNativeWidget) {
  // Override to use a DesktopNativeWidget.
  bool desktop_native_widget = true;
  RunTest(desktop_native_widget);
}
#endif  // !OS_CHROMEOS

TEST_F(WidgetTest, WidgetDeleted_InOnMousePressed) {
  Widget* widget = new Widget;
  Widget::InitParams params =
      CreateParams(views::Widget::InitParams::TYPE_POPUP);
  widget->Init(params);

  widget->SetContentsView(new CloseWidgetView(ui::ET_MOUSE_PRESSED));

  widget->SetSize(gfx::Size(100, 100));
  widget->Show();

  ui::test::EventGenerator generator(GetContext(), widget->GetNativeWindow());

  WidgetDeletionObserver deletion_observer(widget);
  generator.ClickLeftButton();
  EXPECT_FALSE(deletion_observer.IsWidgetAlive());

  // Yay we did not crash!
}

TEST_F(WidgetTest, WidgetDeleted_InDispatchGestureEvent) {
  Widget* widget = new Widget;
  Widget::InitParams params =
      CreateParams(views::Widget::InitParams::TYPE_POPUP);
  widget->Init(params);

  widget->SetContentsView(new CloseWidgetView(ui::ET_GESTURE_TAP_DOWN));

  widget->SetSize(gfx::Size(100, 100));
  widget->Show();

  ui::test::EventGenerator generator(GetContext(), widget->GetNativeWindow());

  WidgetDeletionObserver deletion_observer(widget);
  generator.GestureTapAt(widget->GetWindowBoundsInScreen().CenterPoint());
  EXPECT_FALSE(deletion_observer.IsWidgetAlive());

  // Yay we did not crash!
}

// See description of RunGetNativeThemeFromDestructor() for details.
class GetNativeThemeFromDestructorView : public WidgetDelegateView {
 public:
  GetNativeThemeFromDestructorView() {}
  virtual ~GetNativeThemeFromDestructorView() {
    VerifyNativeTheme();
  }

  virtual View* GetContentsView() OVERRIDE {
    return this;
  }

 private:
  void VerifyNativeTheme() {
    ASSERT_TRUE(GetNativeTheme() != NULL);
  }

  DISALLOW_COPY_AND_ASSIGN(GetNativeThemeFromDestructorView);
};

// Verifies GetNativeTheme() from the destructor of a WidgetDelegateView doesn't
// crash. |is_first_run| is true if this is the first call. A return value of
// true indicates this should be run again with a value of false.
// First run uses DesktopNativeWidgetAura (if possible). Second run doesn't.
bool RunGetNativeThemeFromDestructor(const Widget::InitParams& in_params,
                                     bool is_first_run) {
  bool needs_second_run = false;
  // Destroyed by CloseNow() below.
  Widget* widget = new Widget;
  Widget::InitParams params(in_params);
  // Deletes itself when the Widget is destroyed.
  params.delegate = new GetNativeThemeFromDestructorView;
#if !defined(OS_CHROMEOS)
  if (is_first_run) {
    params.native_widget = new PlatformDesktopNativeWidget(widget);
    needs_second_run = true;
  }
#endif
  widget->Init(params);
  widget->CloseNow();
  return needs_second_run;
}

// See description of RunGetNativeThemeFromDestructor() for details.
TEST_F(WidgetTest, GetNativeThemeFromDestructor) {
  Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_POPUP);
  if (RunGetNativeThemeFromDestructor(params, true))
    RunGetNativeThemeFromDestructor(params, false);
}

// Used by HideCloseDestroy. Allows setting a boolean when the widget is
// destroyed.
class CloseDestroysWidget : public Widget {
 public:
  explicit CloseDestroysWidget(bool* destroyed)
      : destroyed_(destroyed) {
  }

  virtual ~CloseDestroysWidget() {
    if (destroyed_) {
      *destroyed_ = true;
      base::MessageLoop::current()->QuitNow();
    }
  }

  void Detach() { destroyed_ = NULL; }

 private:
  // If non-null set to true from destructor.
  bool* destroyed_;

  DISALLOW_COPY_AND_ASSIGN(CloseDestroysWidget);
};

// An observer that registers that an animation has ended.
class AnimationEndObserver : public ui::ImplicitAnimationObserver {
 public:
  AnimationEndObserver() : animation_completed_(false) {}
  virtual ~AnimationEndObserver() {}

  bool animation_completed() const { return animation_completed_; }

  // ui::ImplicitAnimationObserver:
  virtual void OnImplicitAnimationsCompleted() OVERRIDE {
    animation_completed_ = true;
  }

 private:
  bool animation_completed_;

  DISALLOW_COPY_AND_ASSIGN(AnimationEndObserver);
};

// An observer that registers the bounds of a widget on destruction.
class WidgetBoundsObserver : public WidgetObserver {
 public:
  WidgetBoundsObserver() {}
  virtual ~WidgetBoundsObserver() {}

  gfx::Rect bounds() { return bounds_; }

  // WidgetObserver:
  virtual void OnWidgetDestroying(Widget* widget) OVERRIDE {
    bounds_ = widget->GetWindowBoundsInScreen();
  }

 private:
  gfx::Rect bounds_;

  DISALLOW_COPY_AND_ASSIGN(WidgetBoundsObserver);
};

// Verifies Close() results in destroying.
TEST_F(WidgetTest, CloseDestroys) {
  bool destroyed = false;
  CloseDestroysWidget* widget = new CloseDestroysWidget(&destroyed);
  Widget::InitParams params =
      CreateParams(views::Widget::InitParams::TYPE_MENU);
  params.opacity = Widget::InitParams::OPAQUE_WINDOW;
#if !defined(OS_CHROMEOS)
  params.native_widget = new PlatformDesktopNativeWidget(widget);
#endif
  widget->Init(params);
  widget->Show();
  widget->Hide();
  widget->Close();
  EXPECT_FALSE(destroyed);
  // Run the message loop as Close() asynchronously deletes.
  base::RunLoop().Run();
  EXPECT_TRUE(destroyed);
  // Close() should destroy the widget. If not we'll cleanup to avoid leaks.
  if (!destroyed) {
    widget->Detach();
    widget->CloseNow();
  }
}

// Tests that killing a widget while animating it does not crash.
TEST_F(WidgetTest, CloseWidgetWhileAnimating) {
  scoped_ptr<Widget> widget(new Widget);
  Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_POPUP);
  params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.bounds = gfx::Rect(50, 50, 250, 250);
  widget->Init(params);
  AnimationEndObserver animation_observer;
  WidgetBoundsObserver widget_observer;
  gfx::Rect bounds(0, 0, 50, 50);
  {
    // Normal animations for tests have ZERO_DURATION, make sure we are actually
    // animating the movement.
    ui::ScopedAnimationDurationScaleMode animation_scale_mode(
        ui::ScopedAnimationDurationScaleMode::NON_ZERO_DURATION);
    ui::ScopedLayerAnimationSettings animation_settings(
        widget->GetLayer()->GetAnimator());
    animation_settings.AddObserver(&animation_observer);
    widget->AddObserver(&widget_observer);
    widget->Show();

    // Animate the bounds change.
    widget->SetBounds(bounds);
    widget.reset();
    EXPECT_FALSE(animation_observer.animation_completed());
  }
  EXPECT_TRUE(animation_observer.animation_completed());
  EXPECT_EQ(widget_observer.bounds(), bounds);
}

// A view that consumes mouse-pressed event and gesture-tap-down events.
class RootViewTestView : public View {
 public:
  RootViewTestView(): View() {}

 private:
  virtual bool OnMousePressed(const ui::MouseEvent& event) OVERRIDE {
    return true;
  }

  virtual void OnGestureEvent(ui::GestureEvent* event) OVERRIDE {
    if (event->type() == ui::ET_GESTURE_TAP_DOWN)
      event->SetHandled();
  }
};

// Checks if RootView::*_handler_ fields are unset when widget is hidden.
// Fails on chromium.webkit Windows bot, see crbug.com/264872.
#if defined(OS_WIN)
#define MAYBE_DisableTestRootViewHandlersWhenHidden\
    DISABLED_TestRootViewHandlersWhenHidden
#else
#define MAYBE_DisableTestRootViewHandlersWhenHidden\
    TestRootViewHandlersWhenHidden
#endif
TEST_F(WidgetTest, MAYBE_DisableTestRootViewHandlersWhenHidden) {
  Widget* widget = CreateTopLevelNativeWidget();
  widget->SetBounds(gfx::Rect(0, 0, 300, 300));
  View* view = new RootViewTestView();
  view->SetBounds(0, 0, 300, 300);
  internal::RootView* root_view =
      static_cast<internal::RootView*>(widget->GetRootView());
  root_view->AddChildView(view);

  // Check RootView::mouse_pressed_handler_.
  widget->Show();
  EXPECT_EQ(NULL, GetMousePressedHandler(root_view));
  gfx::Point click_location(45, 15);
  ui::MouseEvent press(ui::ET_MOUSE_PRESSED, click_location, click_location,
                       ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON);
  widget->OnMouseEvent(&press);
  EXPECT_EQ(view, GetMousePressedHandler(root_view));
  widget->Hide();
  EXPECT_EQ(NULL, GetMousePressedHandler(root_view));

  // Check RootView::mouse_move_handler_.
  widget->Show();
  EXPECT_EQ(NULL, GetMouseMoveHandler(root_view));
  gfx::Point move_location(45, 15);
  ui::MouseEvent move(ui::ET_MOUSE_MOVED, move_location, move_location, 0, 0);
  widget->OnMouseEvent(&move);
  EXPECT_EQ(view, GetMouseMoveHandler(root_view));
  widget->Hide();
  EXPECT_EQ(NULL, GetMouseMoveHandler(root_view));

  // Check RootView::gesture_handler_.
  widget->Show();
  EXPECT_EQ(NULL, GetGestureHandler(root_view));
  ui::GestureEvent tap_down(
      15,
      15,
      0,
      base::TimeDelta(),
      ui::GestureEventDetails(ui::ET_GESTURE_TAP_DOWN, 0, 0));
  widget->OnGestureEvent(&tap_down);
  EXPECT_EQ(view, GetGestureHandler(root_view));
  widget->Hide();
  EXPECT_EQ(NULL, GetGestureHandler(root_view));

  widget->Close();
}

// Convenience to make constructing a GestureEvent simpler.
class GestureEventForTest : public ui::GestureEvent {
 public:
  GestureEventForTest(ui::EventType type, int x, int y)
      : GestureEvent(x,
                     y,
                     0,
                     base::TimeDelta(),
                     ui::GestureEventDetails(type, 0.0f, 0.0f)) {}

  GestureEventForTest(ui::GestureEventDetails details, int x, int y)
      : GestureEvent(x, y, 0, base::TimeDelta(), details) {}
};

// Tests that the |gesture_handler_| member in RootView is always NULL
// after the dispatch of a ui::ET_GESTURE_END event corresponding to
// the release of the final touch point on the screen and that
// ui::ET_GESTURE_END events corresponding to the removal of any other touch
// point are never dispatched to a view. Also verifies that
// ui::ET_GESTURE_BEGIN is never dispatched to a view and does not change the
// value of |gesture_handler_|.
TEST_F(WidgetTest, GestureBeginAndEndEvents) {
  Widget* widget = CreateTopLevelNativeWidget();
  widget->SetBounds(gfx::Rect(0, 0, 300, 300));
  EventCountView* view = new EventCountView();
  view->SetBounds(0, 0, 300, 300);
  internal::RootView* root_view =
      static_cast<internal::RootView*>(widget->GetRootView());
  root_view->AddChildView(view);
  widget->Show();

  // If no gesture handler is set, dispatching a ui::ET_GESTURE_END or
  // ui::ET_GESTURE_BEGIN event should not set the gesture handler and
  // the events should remain unhandled because the handle mode of |view|
  // indicates that events should not be consumed.
  EXPECT_EQ(NULL, GetGestureHandler(root_view));
  GestureEventForTest end(ui::ET_GESTURE_END, 15, 15);
  widget->OnGestureEvent(&end);
  EXPECT_FALSE(end.handled());
  EXPECT_EQ(NULL, GetGestureHandler(root_view));

  GestureEventForTest begin(ui::ET_GESTURE_BEGIN, 15, 15);
  widget->OnGestureEvent(&begin);
  EXPECT_FALSE(begin.handled());
  EXPECT_EQ(NULL, GetGestureHandler(root_view));

  // Change the handle mode of |view| to indicate that it would like
  // to handle all events.
  view->set_handle_mode(EventCountView::CONSUME_EVENTS);

  // If no gesture handler is set, dispatching only a ui::ET_GESTURE_BEGIN
  // should not set the gesture handler and should not be marked as handled
  // because it is never dispatched.
  begin = GestureEventForTest(ui::ET_GESTURE_BEGIN, 15, 15);
  widget->OnGestureEvent(&begin);
  EXPECT_FALSE(begin.handled());
  EXPECT_EQ(NULL, GetGestureHandler(root_view));

  // If no gesture handler is set, dispatching only a ui::ET_GESTURE_BEGIN
  // corresponding to a second touch point should not set the gesture handler
  // and should not be marked as handled because it is never dispatched.
  ui::GestureEventDetails details(ui::ET_GESTURE_END, 15, 15);
  details.set_touch_points(2);
  GestureEventForTest end_second_touch_point(details, 15, 15);
  widget->OnGestureEvent(&end_second_touch_point);
  EXPECT_FALSE(end_second_touch_point.handled());
  EXPECT_EQ(NULL, GetGestureHandler(root_view));

  // If no gesture handler is set, dispatching only a ui::ET_GESTURE_END
  // event corresponding to the final touch point should not set the gesture
  // handler. Furthermore, it should not be marked as handled because it was
  // not dispatched (GESTURE_END events are only dispatched in cases where
  // a gesture handler is already set).
  end = GestureEventForTest(ui::ET_GESTURE_END, 15, 15);
  widget->OnGestureEvent(&end);
  EXPECT_FALSE(end.handled());
  EXPECT_EQ(NULL, GetGestureHandler(root_view));

  // If the gesture handler has been set by a previous gesture, then it should
  // remain unchanged on a ui::ET_GESTURE_BEGIN or a ui::ET_GESTURE_END
  // corresponding to a second touch point. It should be reset to NULL by a
  // ui::ET_GESTURE_END corresponding to the final touch point.
  GestureEventForTest tap(ui::ET_GESTURE_TAP, 15, 15);
  widget->OnGestureEvent(&tap);
  EXPECT_TRUE(tap.handled());
  EXPECT_EQ(view, GetGestureHandler(root_view));

  begin = GestureEventForTest(ui::ET_GESTURE_BEGIN, 15, 15);
  widget->OnGestureEvent(&begin);
  EXPECT_FALSE(begin.handled());
  EXPECT_EQ(view, GetGestureHandler(root_view));

  end_second_touch_point = GestureEventForTest(details, 15, 15);
  widget->OnGestureEvent(&end_second_touch_point);
  EXPECT_FALSE(end_second_touch_point.handled());
  EXPECT_EQ(view, GetGestureHandler(root_view));

  end = GestureEventForTest(ui::ET_GESTURE_END, 15, 15);
  widget->OnGestureEvent(&end);
  EXPECT_TRUE(end.handled());
  EXPECT_EQ(NULL, GetGestureHandler(root_view));

  // If the gesture handler has been set by a previous gesture, then
  // it should remain unchanged on a ui::ET_GESTURE_BEGIN or a
  // ui::ET_GESTURE_END corresponding to a second touch point and be reset
  // to NULL by a ui::ET_GESTURE_END corresponding to the final touch point,
  // even when the gesture handler has indicated that it would not like to
  // handle any further events.
  tap = GestureEventForTest(ui::ET_GESTURE_TAP, 15, 15);
  widget->OnGestureEvent(&tap);
  EXPECT_TRUE(tap.handled());
  EXPECT_EQ(view, GetGestureHandler(root_view));

  // Change the handle mode of |view| to indicate that it does not want
  // to handle any further events.
  view->set_handle_mode(EventCountView::PROPAGATE_EVENTS);

  begin = GestureEventForTest(ui::ET_GESTURE_BEGIN, 15, 15);
  widget->OnGestureEvent(&begin);
  EXPECT_FALSE(begin.handled());
  EXPECT_EQ(view, GetGestureHandler(root_view));

  end_second_touch_point = GestureEventForTest(details, 15, 15);
  widget->OnGestureEvent(&end_second_touch_point);
  EXPECT_FALSE(end_second_touch_point.handled());
  EXPECT_EQ(view, GetGestureHandler(root_view));

  end = GestureEventForTest(ui::ET_GESTURE_END, 15, 15);
  widget->OnGestureEvent(&end);
  EXPECT_FALSE(end.handled());
  EXPECT_EQ(NULL, GetGestureHandler(root_view));

  widget->Close();
}

// Tests that a (non-scroll) gesture event is dispatched to the correct views
// in a view hierarchy and that the default gesture handler in RootView is set
// correctly.
TEST_F(WidgetTest, GestureEventDispatch) {
  Widget* widget = CreateTopLevelNativeWidget();
  widget->SetBounds(gfx::Rect(0, 0, 300, 300));

  // Define a hierarchy of four views (coordinates are in
  // their parent coordinate space).
  // v1 (0, 0, 300, 300)
  //   v2 (0, 0, 100, 100)
  //     v3 (0, 0, 50, 50)
  //       v4(0, 0, 10, 10)
  EventCountView* v1 = new EventCountView();
  v1->SetBounds(0, 0, 300, 300);
  EventCountView* v2 = new EventCountView();
  v2->SetBounds(0, 0, 100, 100);
  EventCountView* v3 = new EventCountView();
  v3->SetBounds(0, 0, 50, 50);
  EventCountView* v4 = new EventCountView();
  v4->SetBounds(0, 0, 10, 10);
  internal::RootView* root_view =
      static_cast<internal::RootView*>(widget->GetRootView());
  root_view->AddChildView(v1);
  v1->AddChildView(v2);
  v2->AddChildView(v3);
  v3->AddChildView(v4);

  widget->Show();

  // No gesture handler is set in the root view and none of the views in the
  // view hierarchy handle a ui::ET_GESTURE_TAP event. In this case the tap
  // event should be dispatched to all views in the hierarchy, the gesture
  // handler should remain unset, and the event should remain unhandled.
  GestureEventForTest tap(ui::ET_GESTURE_TAP, 5, 5);
  EXPECT_EQ(NULL, GetGestureHandler(root_view));
  widget->OnGestureEvent(&tap);
  EXPECT_EQ(1, v1->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(1, v2->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(1, v3->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(1, v4->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(NULL, GetGestureHandler(root_view));
  EXPECT_FALSE(tap.handled());

  // No gesture handler is set in the root view and |v1|, |v2|, and |v3| all
  // handle a ui::ET_GESTURE_TAP event. In this case the tap event should be
  // dispatched to |v4| and |v3|, the gesture handler should be set to |v3|,
  // and the event should be marked as handled.
  v1->ResetCounts();
  v2->ResetCounts();
  v3->ResetCounts();
  v4->ResetCounts();
  v1->set_handle_mode(EventCountView::CONSUME_EVENTS);
  v2->set_handle_mode(EventCountView::CONSUME_EVENTS);
  v3->set_handle_mode(EventCountView::CONSUME_EVENTS);
  tap = GestureEventForTest(ui::ET_GESTURE_TAP, 5, 5);
  widget->OnGestureEvent(&tap);
  EXPECT_EQ(0, v1->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(1, v3->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(1, v4->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(v3, GetGestureHandler(root_view));
  EXPECT_TRUE(tap.handled());

  // The gesture handler is set to |v3| and all views handle all gesture event
  // types. In this case subsequent gesture events should only be dispatched to
  // |v3| and marked as handled. The gesture handler should remain as |v3|.
  v1->ResetCounts();
  v2->ResetCounts();
  v3->ResetCounts();
  v4->ResetCounts();
  v4->set_handle_mode(EventCountView::CONSUME_EVENTS);
  tap = GestureEventForTest(ui::ET_GESTURE_TAP, 5, 5);
  widget->OnGestureEvent(&tap);
  EXPECT_TRUE(tap.handled());
  GestureEventForTest show_press(ui::ET_GESTURE_SHOW_PRESS, 5, 5);
  widget->OnGestureEvent(&show_press);
  tap = GestureEventForTest(ui::ET_GESTURE_TAP, 5, 5);
  widget->OnGestureEvent(&tap);
  EXPECT_EQ(0, v1->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(2, v3->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(0, v4->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(0, v1->GetEventCount(ui::ET_GESTURE_SHOW_PRESS));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_GESTURE_SHOW_PRESS));
  EXPECT_EQ(1, v3->GetEventCount(ui::ET_GESTURE_SHOW_PRESS));
  EXPECT_EQ(0, v4->GetEventCount(ui::ET_GESTURE_SHOW_PRESS));
  EXPECT_TRUE(tap.handled());
  EXPECT_TRUE(show_press.handled());
  EXPECT_EQ(v3, GetGestureHandler(root_view));

  // The gesture handler is set to |v3|, but |v3| does not handle
  // ui::ET_GESTURE_TAP events. In this case a tap gesture should be dispatched
  // only to |v3|, but the event should remain unhandled. The gesture handler
  // should remain as |v3|.
  v1->ResetCounts();
  v2->ResetCounts();
  v3->ResetCounts();
  v4->ResetCounts();
  v3->set_handle_mode(EventCountView::PROPAGATE_EVENTS);
  tap = GestureEventForTest(ui::ET_GESTURE_TAP, 5, 5);
  widget->OnGestureEvent(&tap);
  EXPECT_EQ(0, v1->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(1, v3->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(0, v4->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_FALSE(tap.handled());
  EXPECT_EQ(v3, GetGestureHandler(root_view));

  widget->Close();
}

// Tests that gesture scroll events will change the default gesture handler in
// RootView if the current handler to which they are dispatched does not handle
// gesture scroll events.
TEST_F(WidgetTest, ScrollGestureEventDispatch) {
  Widget* widget = CreateTopLevelNativeWidget();
  widget->SetBounds(gfx::Rect(0, 0, 300, 300));

  // Define a hierarchy of four views (coordinates are in
  // their parent coordinate space).
  // v1 (0, 0, 300, 300)
  //   v2 (0, 0, 100, 100)
  //     v3 (0, 0, 50, 50)
  //       v4(0, 0, 10, 10)
  EventCountView* v1 = new EventCountView();
  v1->SetBounds(0, 0, 300, 300);
  EventCountView* v2 = new EventCountView();
  v2->SetBounds(0, 0, 100, 100);
  EventCountView* v3 = new EventCountView();
  v3->SetBounds(0, 0, 50, 50);
  EventCountView* v4 = new EventCountView();
  v4->SetBounds(0, 0, 10, 10);
  internal::RootView* root_view =
      static_cast<internal::RootView*>(widget->GetRootView());
  root_view->AddChildView(v1);
  v1->AddChildView(v2);
  v2->AddChildView(v3);
  v3->AddChildView(v4);

  widget->Show();

  // Change the handle mode of |v3| to indicate that it would like to handle
  // gesture events.
  v3->set_handle_mode(EventCountView::CONSUME_EVENTS);

  // When no gesture handler is set, dispatching a ui::ET_GESTURE_TAP_DOWN
  // should bubble up the views hierarchy until it reaches the first view
  // that will handle it (|v3|) and then sets the handler to |v3|.
  EXPECT_EQ(NULL, GetGestureHandler(root_view));
  GestureEventForTest tap_down(ui::ET_GESTURE_TAP_DOWN, 5, 5);
  widget->OnGestureEvent(&tap_down);
  EXPECT_EQ(0, v1->GetEventCount(ui::ET_GESTURE_TAP_DOWN));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_GESTURE_TAP_DOWN));
  EXPECT_EQ(1, v3->GetEventCount(ui::ET_GESTURE_TAP_DOWN));
  EXPECT_EQ(1, v4->GetEventCount(ui::ET_GESTURE_TAP_DOWN));
  EXPECT_EQ(v3, GetGestureHandler(root_view));
  EXPECT_TRUE(tap_down.handled());
  v1->ResetCounts();
  v2->ResetCounts();
  v3->ResetCounts();
  v4->ResetCounts();

  // A ui::ET_GESTURE_TAP_CANCEL event should be dispatched to |v3| directly.
  GestureEventForTest tap_cancel(ui::ET_GESTURE_TAP_CANCEL, 5, 5);
  widget->OnGestureEvent(&tap_cancel);
  EXPECT_EQ(0, v1->GetEventCount(ui::ET_GESTURE_TAP_CANCEL));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_GESTURE_TAP_CANCEL));
  EXPECT_EQ(1, v3->GetEventCount(ui::ET_GESTURE_TAP_CANCEL));
  EXPECT_EQ(0, v4->GetEventCount(ui::ET_GESTURE_TAP_CANCEL));
  EXPECT_EQ(v3, GetGestureHandler(root_view));
  EXPECT_TRUE(tap_cancel.handled());
  v1->ResetCounts();
  v2->ResetCounts();
  v3->ResetCounts();
  v4->ResetCounts();

  // Change the handle mode of |v3| to indicate that it would no longer like
  // to handle events, and change the mode of |v1| to indicate that it would
  // like to handle events.
  v3->set_handle_mode(EventCountView::PROPAGATE_EVENTS);
  v1->set_handle_mode(EventCountView::CONSUME_EVENTS);

  // Dispatch a ui::ET_GESTURE_SCROLL_BEGIN event. Because the current gesture
  // handler (|v3|) does not handle scroll events, the event should bubble up
  // the views hierarchy until it reaches the first view that will handle
  // it (|v1|) and then sets the handler to |v1|.
  GestureEventForTest scroll_begin(ui::ET_GESTURE_SCROLL_BEGIN, 5, 5);
  widget->OnGestureEvent(&scroll_begin);
  EXPECT_EQ(1, v1->GetEventCount(ui::ET_GESTURE_SCROLL_BEGIN));
  EXPECT_EQ(1, v2->GetEventCount(ui::ET_GESTURE_SCROLL_BEGIN));
  EXPECT_EQ(1, v3->GetEventCount(ui::ET_GESTURE_SCROLL_BEGIN));
  EXPECT_EQ(0, v4->GetEventCount(ui::ET_GESTURE_SCROLL_BEGIN));
  EXPECT_EQ(v1, GetGestureHandler(root_view));
  EXPECT_TRUE(scroll_begin.handled());
  v1->ResetCounts();
  v2->ResetCounts();
  v3->ResetCounts();
  v4->ResetCounts();

  // A ui::ET_GESTURE_SCROLL_UPDATE event should be dispatched to |v1|
  // directly.
  GestureEventForTest scroll_update(ui::ET_GESTURE_SCROLL_UPDATE, 5, 5);
  widget->OnGestureEvent(&scroll_update);
  EXPECT_EQ(1, v1->GetEventCount(ui::ET_GESTURE_SCROLL_UPDATE));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_GESTURE_SCROLL_UPDATE));
  EXPECT_EQ(0, v3->GetEventCount(ui::ET_GESTURE_SCROLL_UPDATE));
  EXPECT_EQ(0, v4->GetEventCount(ui::ET_GESTURE_SCROLL_UPDATE));
  EXPECT_EQ(v1, GetGestureHandler(root_view));
  EXPECT_TRUE(scroll_update.handled());
  v1->ResetCounts();
  v2->ResetCounts();
  v3->ResetCounts();
  v4->ResetCounts();

  // A ui::ET_GESTURE_SCROLL_END event should be dispatched to |v1|
  // directly and should not reset the gesture handler.
  GestureEventForTest scroll_end(ui::ET_GESTURE_SCROLL_END, 5, 5);
  widget->OnGestureEvent(&scroll_end);
  EXPECT_EQ(1, v1->GetEventCount(ui::ET_GESTURE_SCROLL_END));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_GESTURE_SCROLL_END));
  EXPECT_EQ(0, v3->GetEventCount(ui::ET_GESTURE_SCROLL_END));
  EXPECT_EQ(0, v4->GetEventCount(ui::ET_GESTURE_SCROLL_END));
  EXPECT_EQ(v1, GetGestureHandler(root_view));
  EXPECT_TRUE(scroll_end.handled());
  v1->ResetCounts();
  v2->ResetCounts();
  v3->ResetCounts();
  v4->ResetCounts();

  // A ui::ET_GESTURE_PINCH_BEGIN event (which is a non-scroll event) should
  // still be dispatched to |v1| directly.
  GestureEventForTest pinch_begin(ui::ET_GESTURE_PINCH_BEGIN, 5, 5);
  widget->OnGestureEvent(&pinch_begin);
  EXPECT_EQ(1, v1->GetEventCount(ui::ET_GESTURE_PINCH_BEGIN));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_GESTURE_PINCH_BEGIN));
  EXPECT_EQ(0, v3->GetEventCount(ui::ET_GESTURE_PINCH_BEGIN));
  EXPECT_EQ(0, v4->GetEventCount(ui::ET_GESTURE_PINCH_BEGIN));
  EXPECT_EQ(v1, GetGestureHandler(root_view));
  EXPECT_TRUE(pinch_begin.handled());
  v1->ResetCounts();
  v2->ResetCounts();
  v3->ResetCounts();
  v4->ResetCounts();

  // A ui::ET_GESTURE_END event should be dispatched to |v1| and should
  // set the gesture handler to NULL.
  GestureEventForTest end(ui::ET_GESTURE_END, 5, 5);
  widget->OnGestureEvent(&end);
  EXPECT_EQ(1, v1->GetEventCount(ui::ET_GESTURE_END));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_GESTURE_END));
  EXPECT_EQ(0, v3->GetEventCount(ui::ET_GESTURE_END));
  EXPECT_EQ(0, v4->GetEventCount(ui::ET_GESTURE_END));
  EXPECT_EQ(NULL, GetGestureHandler(root_view));
  EXPECT_TRUE(end.handled());

  widget->Close();
}

// Verifies that disabled views are permitted to be set as the default gesture
// handler in RootView. Also verifies that gesture events targeted to a disabled
// view are not actually dispatched to the view, but are still marked as
// handled.
TEST_F(WidgetTest, DisabledGestureEventTarget) {
  Widget* widget = CreateTopLevelNativeWidget();
  widget->SetBounds(gfx::Rect(0, 0, 300, 300));

  // Define a hierarchy of four views (coordinates are in
  // their parent coordinate space).
  // v1 (0, 0, 300, 300)
  //   v2 (0, 0, 100, 100)
  //     v3 (0, 0, 50, 50)
  //       v4(0, 0, 10, 10)
  EventCountView* v1 = new EventCountView();
  v1->SetBounds(0, 0, 300, 300);
  EventCountView* v2 = new EventCountView();
  v2->SetBounds(0, 0, 100, 100);
  EventCountView* v3 = new EventCountView();
  v3->SetBounds(0, 0, 50, 50);
  EventCountView* v4 = new EventCountView();
  v4->SetBounds(0, 0, 10, 10);
  internal::RootView* root_view =
      static_cast<internal::RootView*>(widget->GetRootView());
  root_view->AddChildView(v1);
  v1->AddChildView(v2);
  v2->AddChildView(v3);
  v3->AddChildView(v4);

  widget->Show();

  // |v1|, |v2|, and |v3| all handle gesture events but |v3| is marked as
  // disabled.
  v1->set_handle_mode(EventCountView::CONSUME_EVENTS);
  v2->set_handle_mode(EventCountView::CONSUME_EVENTS);
  v3->set_handle_mode(EventCountView::CONSUME_EVENTS);
  v3->SetEnabled(false);

  // No gesture handler is set in the root view, so it should remain unset
  // after a GESTURE_END. GESTURE_END events are not dispatched unless
  // a gesture handler is already set in the root view, so none of the
  // views should see this event and it should not be marked as handled.
  GestureEventForTest end(ui::ET_GESTURE_END, 5, 5);
  widget->OnGestureEvent(&end);
  EXPECT_EQ(0, v1->GetEventCount(ui::ET_GESTURE_END));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_GESTURE_END));
  EXPECT_EQ(0, v3->GetEventCount(ui::ET_GESTURE_END));
  EXPECT_EQ(0, v4->GetEventCount(ui::ET_GESTURE_END));
  EXPECT_EQ(NULL, GetGestureHandler(root_view));
  EXPECT_FALSE(end.handled());
  v1->ResetCounts();
  v2->ResetCounts();
  v3->ResetCounts();
  v4->ResetCounts();

  // No gesture handler is set in the root view. In this case the tap event
  // should be dispatched only to |v4|, the gesture handler should be set to
  // |v3|, and the event should be marked as handled.
  GestureEventForTest tap(ui::ET_GESTURE_TAP, 5, 5);
  widget->OnGestureEvent(&tap);
  EXPECT_EQ(0, v1->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(0, v3->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(1, v4->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(v3, GetGestureHandler(root_view));
  EXPECT_TRUE(tap.handled());
  v1->ResetCounts();
  v2->ResetCounts();
  v3->ResetCounts();
  v4->ResetCounts();

  // A subsequent gesture event should be marked as handled but not dispatched.
  tap = GestureEventForTest(ui::ET_GESTURE_TAP, 5, 5);
  widget->OnGestureEvent(&tap);
  EXPECT_EQ(0, v1->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(0, v3->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(0, v4->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(v3, GetGestureHandler(root_view));
  EXPECT_TRUE(tap.handled());
  v1->ResetCounts();
  v2->ResetCounts();
  v3->ResetCounts();
  v4->ResetCounts();

  // A GESTURE_END should reset the default gesture handler to NULL. It should
  // also not be dispatched to |v3| but still marked as handled.
  end = GestureEventForTest(ui::ET_GESTURE_END, 5, 5);
  widget->OnGestureEvent(&end);
  EXPECT_EQ(0, v1->GetEventCount(ui::ET_GESTURE_END));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_GESTURE_END));
  EXPECT_EQ(0, v3->GetEventCount(ui::ET_GESTURE_END));
  EXPECT_EQ(0, v4->GetEventCount(ui::ET_GESTURE_END));
  EXPECT_EQ(NULL, GetGestureHandler(root_view));
  EXPECT_TRUE(end.handled());
  v1->ResetCounts();
  v2->ResetCounts();
  v3->ResetCounts();
  v4->ResetCounts();

  // Change the handle mode of |v3| to indicate that it would no longer like
  // to handle events which are dispatched to it.
  v3->set_handle_mode(EventCountView::PROPAGATE_EVENTS);

  // No gesture handler is set in the root view. In this case the tap event
  // should be dispatched only to |v4| and the event should be marked as
  // handled. Furthermore, the gesture handler should be set to
  // |v3|; even though |v3| does not explicitly handle events, it is a
  // valid target for the tap event because it is disabled.
  tap = GestureEventForTest(ui::ET_GESTURE_TAP, 5, 5);
  widget->OnGestureEvent(&tap);
  EXPECT_EQ(0, v1->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(0, v3->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(1, v4->GetEventCount(ui::ET_GESTURE_TAP));
  EXPECT_EQ(v3, GetGestureHandler(root_view));
  EXPECT_TRUE(tap.handled());
  v1->ResetCounts();
  v2->ResetCounts();
  v3->ResetCounts();
  v4->ResetCounts();

  // A GESTURE_END should reset the default gesture handler to NULL. It should
  // also not be dispatched to |v3| but still marked as handled.
  end = GestureEventForTest(ui::ET_GESTURE_END, 5, 5);
  widget->OnGestureEvent(&end);
  EXPECT_EQ(0, v1->GetEventCount(ui::ET_GESTURE_END));
  EXPECT_EQ(0, v2->GetEventCount(ui::ET_GESTURE_END));
  EXPECT_EQ(0, v3->GetEventCount(ui::ET_GESTURE_END));
  EXPECT_EQ(0, v4->GetEventCount(ui::ET_GESTURE_END));
  EXPECT_EQ(NULL, GetGestureHandler(root_view));
  EXPECT_TRUE(end.handled());

  widget->Close();
}

// Test the result of Widget::GetAllChildWidgets().
TEST_F(WidgetTest, GetAllChildWidgets) {
  // Create the following widget hierarchy:
  //
  // toplevel
  // +-- w1
  //     +-- w11
  // +-- w2
  //     +-- w21
  //     +-- w22
  Widget* toplevel = CreateTopLevelPlatformWidget();
  Widget* w1 = CreateChildPlatformWidget(toplevel->GetNativeView());
  Widget* w11 = CreateChildPlatformWidget(w1->GetNativeView());
  Widget* w2 = CreateChildPlatformWidget(toplevel->GetNativeView());
  Widget* w21 = CreateChildPlatformWidget(w2->GetNativeView());
  Widget* w22 = CreateChildPlatformWidget(w2->GetNativeView());

  std::set<Widget*> expected;
  expected.insert(toplevel);
  expected.insert(w1);
  expected.insert(w11);
  expected.insert(w2);
  expected.insert(w21);
  expected.insert(w22);

  std::set<Widget*> widgets;
  Widget::GetAllChildWidgets(toplevel->GetNativeView(), &widgets);

  EXPECT_EQ(expected.size(), widgets.size());
  EXPECT_TRUE(std::equal(expected.begin(), expected.end(), widgets.begin()));
}

// Used by DestroyChildWidgetsInOrder. On destruction adds the supplied name to
// a vector.
class DestroyedTrackingView : public View {
 public:
  DestroyedTrackingView(const std::string& name,
                        std::vector<std::string>* add_to)
      : name_(name),
        add_to_(add_to) {
  }

  virtual ~DestroyedTrackingView() {
    add_to_->push_back(name_);
  }

 private:
  const std::string name_;
  std::vector<std::string>* add_to_;

  DISALLOW_COPY_AND_ASSIGN(DestroyedTrackingView);
};

class WidgetChildDestructionTest : public WidgetTest {
 public:
  WidgetChildDestructionTest() {}

  // Creates a top level and a child, destroys the child and verifies the views
  // of the child are destroyed before the views of the parent.
  void RunDestroyChildWidgetsTest(bool top_level_has_desktop_native_widget_aura,
                                  bool child_has_desktop_native_widget_aura) {
    // When a View is destroyed its name is added here.
    std::vector<std::string> destroyed;

    Widget* top_level = new Widget;
    Widget::InitParams params =
        CreateParams(views::Widget::InitParams::TYPE_WINDOW);
#if !defined(OS_CHROMEOS)
    if (top_level_has_desktop_native_widget_aura)
      params.native_widget = new PlatformDesktopNativeWidget(top_level);
#endif
    top_level->Init(params);
    top_level->GetRootView()->AddChildView(
        new DestroyedTrackingView("parent", &destroyed));
    top_level->Show();

    Widget* child = new Widget;
    Widget::InitParams child_params =
        CreateParams(views::Widget::InitParams::TYPE_POPUP);
    child_params.parent = top_level->GetNativeView();
#if !defined(OS_CHROMEOS)
    if (child_has_desktop_native_widget_aura)
      child_params.native_widget = new PlatformDesktopNativeWidget(child);
#endif
    child->Init(child_params);
    child->GetRootView()->AddChildView(
        new DestroyedTrackingView("child", &destroyed));
    child->Show();

    // Should trigger destruction of the child too.
    top_level->native_widget_private()->CloseNow();

    // Child should be destroyed first.
    ASSERT_EQ(2u, destroyed.size());
    EXPECT_EQ("child", destroyed[0]);
    EXPECT_EQ("parent", destroyed[1]);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(WidgetChildDestructionTest);
};

#if !defined(OS_CHROMEOS)
// See description of RunDestroyChildWidgetsTest(). Parent uses
// DesktopNativeWidgetAura.
TEST_F(WidgetChildDestructionTest,
       DestroyChildWidgetsInOrderWithDesktopNativeWidget) {
  RunDestroyChildWidgetsTest(true, false);
}

// See description of RunDestroyChildWidgetsTest(). Both parent and child use
// DesktopNativeWidgetAura.
TEST_F(WidgetChildDestructionTest,
       DestroyChildWidgetsInOrderWithDesktopNativeWidgetForBoth) {
  RunDestroyChildWidgetsTest(true, true);
}
#endif  // !defined(OS_CHROMEOS)

// See description of RunDestroyChildWidgetsTest().
TEST_F(WidgetChildDestructionTest, DestroyChildWidgetsInOrder) {
  RunDestroyChildWidgetsTest(false, false);
}

#if !defined(OS_CHROMEOS)
// Provides functionality to create a window modal dialog.
class ModalDialogDelegate : public DialogDelegateView {
 public:
  ModalDialogDelegate() {}
  virtual ~ModalDialogDelegate() {}

  // WidgetDelegate overrides.
  virtual ui::ModalType GetModalType() const OVERRIDE {
    return ui::MODAL_TYPE_WINDOW;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ModalDialogDelegate);
};

// This test verifies that whether mouse events when a modal dialog is
// displayed are eaten or recieved by the dialog.
TEST_F(WidgetTest, WindowMouseModalityTest) {
  // Create a top level widget.
  Widget top_level_widget;
  Widget::InitParams init_params =
      CreateParams(Widget::InitParams::TYPE_WINDOW);
  init_params.show_state = ui::SHOW_STATE_NORMAL;
  gfx::Rect initial_bounds(0, 0, 500, 500);
  init_params.bounds = initial_bounds;
  init_params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  init_params.native_widget =
      new PlatformDesktopNativeWidget(&top_level_widget);
  top_level_widget.Init(init_params);
  top_level_widget.Show();
  EXPECT_TRUE(top_level_widget.IsVisible());

  // Create a view and validate that a mouse moves makes it to the view.
  EventCountView* widget_view = new EventCountView();
  widget_view->SetBounds(0, 0, 10, 10);
  top_level_widget.GetRootView()->AddChildView(widget_view);

  gfx::Point cursor_location_main(5, 5);
  ui::MouseEvent move_main(ui::ET_MOUSE_MOVED,
                           cursor_location_main,
                           cursor_location_main,
                           ui::EF_NONE,
                           ui::EF_NONE);
  ui::EventDispatchDetails details =
      GetEventProcessor(&top_level_widget)->OnEventFromSource(&move_main);
  ASSERT_FALSE(details.dispatcher_destroyed);

  EXPECT_EQ(1, widget_view->GetEventCount(ui::ET_MOUSE_ENTERED));
  widget_view->ResetCounts();

  // Create a modal dialog and validate that a mouse down message makes it to
  // the main view within the dialog.

  // This instance will be destroyed when the dialog is destroyed.
  ModalDialogDelegate* dialog_delegate = new ModalDialogDelegate;

  Widget* modal_dialog_widget = views::DialogDelegate::CreateDialogWidget(
      dialog_delegate, NULL, top_level_widget.GetNativeView());
  modal_dialog_widget->SetBounds(gfx::Rect(100, 100, 200, 200));
  EventCountView* dialog_widget_view = new EventCountView();
  dialog_widget_view->SetBounds(0, 0, 50, 50);
  modal_dialog_widget->GetRootView()->AddChildView(dialog_widget_view);
  modal_dialog_widget->Show();
  EXPECT_TRUE(modal_dialog_widget->IsVisible());

  gfx::Point cursor_location_dialog(100, 100);
  ui::MouseEvent mouse_down_dialog(ui::ET_MOUSE_PRESSED,
                                   cursor_location_dialog,
                                   cursor_location_dialog,
                                   ui::EF_NONE,
                                   ui::EF_NONE);
  details = GetEventProcessor(&top_level_widget)->OnEventFromSource(
      &mouse_down_dialog);
  ASSERT_FALSE(details.dispatcher_destroyed);
  EXPECT_EQ(1, dialog_widget_view->GetEventCount(ui::ET_MOUSE_PRESSED));

  // Send a mouse move message to the main window. It should not be received by
  // the main window as the modal dialog is still active.
  gfx::Point cursor_location_main2(6, 6);
  ui::MouseEvent mouse_down_main(ui::ET_MOUSE_MOVED,
                                 cursor_location_main2,
                                 cursor_location_main2,
                                 ui::EF_NONE,
                                 ui::EF_NONE);
  details = GetEventProcessor(&top_level_widget)->OnEventFromSource(
      &mouse_down_main);
  ASSERT_FALSE(details.dispatcher_destroyed);
  EXPECT_EQ(0, widget_view->GetEventCount(ui::ET_MOUSE_MOVED));

  modal_dialog_widget->CloseNow();
  top_level_widget.CloseNow();
}

// Verifies nativeview visbility matches that of Widget visibility when
// SetFullscreen is invoked.
TEST_F(WidgetTest, FullscreenStatePropagated) {
  Widget::InitParams init_params =
      CreateParams(Widget::InitParams::TYPE_WINDOW);
  init_params.show_state = ui::SHOW_STATE_NORMAL;
  init_params.bounds = gfx::Rect(0, 0, 500, 500);
  init_params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;

  {
    Widget top_level_widget;
    top_level_widget.Init(init_params);
    top_level_widget.SetFullscreen(true);
    EXPECT_EQ(top_level_widget.IsVisible(),
              IsNativeWindowVisible(top_level_widget.GetNativeWindow()));
    top_level_widget.CloseNow();
  }
#if !defined(OS_CHROMEOS)
  {
    Widget top_level_widget;
    init_params.native_widget =
        new PlatformDesktopNativeWidget(&top_level_widget);
    top_level_widget.Init(init_params);
    top_level_widget.SetFullscreen(true);
    EXPECT_EQ(top_level_widget.IsVisible(),
              IsNativeWindowVisible(top_level_widget.GetNativeWindow()));
    top_level_widget.CloseNow();
  }
#endif
}
#if defined(OS_WIN)

// Provides functionality to test widget activation via an activation flag
// which can be set by an accessor.
class ModalWindowTestWidgetDelegate : public WidgetDelegate {
 public:
  ModalWindowTestWidgetDelegate()
      : widget_(NULL),
        can_activate_(true) {}

  virtual ~ModalWindowTestWidgetDelegate() {}

  // Overridden from WidgetDelegate:
  virtual void DeleteDelegate() OVERRIDE {
    delete this;
  }
  virtual Widget* GetWidget() OVERRIDE {
    return widget_;
  }
  virtual const Widget* GetWidget() const OVERRIDE {
    return widget_;
  }
  virtual bool CanActivate() const OVERRIDE {
    return can_activate_;
  }
  virtual bool ShouldAdvanceFocusToTopLevelWidget() const OVERRIDE {
    return true;
  }

  void set_can_activate(bool can_activate) {
    can_activate_ = can_activate;
  }

  void set_widget(Widget* widget) {
    widget_ = widget;
  }

 private:
  Widget* widget_;
  bool can_activate_;

  DISALLOW_COPY_AND_ASSIGN(ModalWindowTestWidgetDelegate);
};

// Tests whether we can activate the top level widget when a modal dialog is
// active.
TEST_F(WidgetTest, WindowModalityActivationTest) {
  // Destroyed when the top level widget created below is destroyed.
  ModalWindowTestWidgetDelegate* widget_delegate =
      new ModalWindowTestWidgetDelegate;
  // Create a top level widget.
  Widget top_level_widget;
  Widget::InitParams init_params =
      CreateParams(Widget::InitParams::TYPE_WINDOW);
  init_params.show_state = ui::SHOW_STATE_NORMAL;
  gfx::Rect initial_bounds(0, 0, 500, 500);
  init_params.bounds = initial_bounds;
  init_params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  init_params.native_widget = new DesktopNativeWidgetAura(&top_level_widget);
  init_params.delegate = widget_delegate;
  top_level_widget.Init(init_params);
  widget_delegate->set_widget(&top_level_widget);
  top_level_widget.Show();
  EXPECT_TRUE(top_level_widget.IsVisible());

  HWND win32_window = views::HWNDForWidget(&top_level_widget);
  EXPECT_TRUE(::IsWindow(win32_window));

  // This instance will be destroyed when the dialog is destroyed.
  ModalDialogDelegate* dialog_delegate = new ModalDialogDelegate;

  // We should be able to activate the window even if the WidgetDelegate
  // says no, when a modal dialog is active.
  widget_delegate->set_can_activate(false);

  Widget* modal_dialog_widget = views::DialogDelegate::CreateDialogWidget(
      dialog_delegate, NULL, top_level_widget.GetNativeWindow());
  modal_dialog_widget->SetBounds(gfx::Rect(100, 100, 200, 200));
  modal_dialog_widget->Show();
  EXPECT_TRUE(modal_dialog_widget->IsVisible());

  LRESULT activate_result = ::SendMessage(
      win32_window,
      WM_MOUSEACTIVATE,
      reinterpret_cast<WPARAM>(win32_window),
      MAKELPARAM(WM_LBUTTONDOWN, HTCLIENT));
  EXPECT_EQ(activate_result, MA_ACTIVATE);

  modal_dialog_widget->CloseNow();
  top_level_widget.CloseNow();
}
#endif  // defined(OS_WIN)
#endif  // !defined(OS_CHROMEOS)

TEST_F(WidgetTest, ShowCreatesActiveWindow) {
  Widget* widget = CreateTopLevelPlatformWidget();

  widget->Show();
  EXPECT_EQ(GetWidgetShowState(widget), ui::SHOW_STATE_NORMAL);

  widget->CloseNow();
}

// OSX does not have a per-application "active" window such as provided by
// ::GetActiveWindow() on Windows. There is only a system-wide "keyWindow" which
// is updated asynchronously.
#if defined(OS_MACOSX)
#define MAYBE_ShowInactive DISABLED_ShowInactive
#else
#define MAYBE_ShowInactive ShowInactive
#endif
TEST_F(WidgetTest, MAYBE_ShowInactive) {
  Widget* widget = CreateTopLevelPlatformWidget();

  widget->ShowInactive();
  EXPECT_EQ(GetWidgetShowState(widget), ui::SHOW_STATE_INACTIVE);

  widget->CloseNow();
}

TEST_F(WidgetTest, InactiveBeforeShow) {
  Widget* widget = CreateTopLevelPlatformWidget();

  EXPECT_FALSE(widget->IsActive());
  EXPECT_FALSE(widget->IsVisible());

  widget->Show();

  EXPECT_TRUE(widget->IsActive());
  EXPECT_TRUE(widget->IsVisible());

  widget->CloseNow();
}

TEST_F(WidgetTest, ShowInactiveAfterShow) {
  // Create 2 widgets to ensure window layering does not change.
  Widget* widget = CreateTopLevelPlatformWidget();
  Widget* widget2 = CreateTopLevelPlatformWidget();

  widget2->Show();
  EXPECT_FALSE(widget->IsActive());
  EXPECT_TRUE(widget2->IsVisible());
  EXPECT_TRUE(widget2->IsActive());

  widget->Show();
  EXPECT_TRUE(widget->IsActive());
  EXPECT_FALSE(widget2->IsActive());
  widget->ShowInactive();
  EXPECT_TRUE(widget->IsActive());
  EXPECT_FALSE(widget2->IsActive());
  EXPECT_EQ(GetWidgetShowState(widget), ui::SHOW_STATE_NORMAL);

  widget2->CloseNow();
  widget->CloseNow();
}

TEST_F(WidgetTest, ShowAfterShowInactive) {
  Widget* widget = CreateTopLevelPlatformWidget();

  widget->ShowInactive();
  widget->Show();
  EXPECT_EQ(GetWidgetShowState(widget), ui::SHOW_STATE_NORMAL);

  widget->CloseNow();
}

#if !defined(OS_CHROMEOS)
TEST_F(WidgetTest, InactiveWidgetDoesNotGrabActivation) {
  Widget* widget = CreateTopLevelPlatformWidget();
  widget->Show();
  EXPECT_EQ(GetWidgetShowState(widget), ui::SHOW_STATE_NORMAL);

  Widget widget2;
  Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_POPUP);
  params.native_widget = new PlatformDesktopNativeWidget(&widget2);
  params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  widget2.Init(params);
  widget2.Show();

  EXPECT_EQ(GetWidgetShowState(&widget2), ui::SHOW_STATE_INACTIVE);
  EXPECT_EQ(GetWidgetShowState(widget), ui::SHOW_STATE_NORMAL);

  widget->CloseNow();
  widget2.CloseNow();
}
#endif  // !defined(OS_CHROMEOS)

namespace {

class FullscreenAwareFrame : public views::NonClientFrameView {
 public:
  explicit FullscreenAwareFrame(views::Widget* widget)
      : widget_(widget), fullscreen_layout_called_(false) {}
  virtual ~FullscreenAwareFrame() {}

  // views::NonClientFrameView overrides:
  virtual gfx::Rect GetBoundsForClientView() const OVERRIDE {
    return gfx::Rect();
  }
  virtual gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const OVERRIDE {
    return gfx::Rect();
  }
  virtual int NonClientHitTest(const gfx::Point& point) OVERRIDE {
    return HTNOWHERE;
  }
  virtual void GetWindowMask(const gfx::Size& size,
                             gfx::Path* window_mask) OVERRIDE {}
  virtual void ResetWindowControls() OVERRIDE {}
  virtual void UpdateWindowIcon() OVERRIDE {}
  virtual void UpdateWindowTitle() OVERRIDE {}

  // views::View overrides:
  virtual void Layout() OVERRIDE {
    if (widget_->IsFullscreen())
      fullscreen_layout_called_ = true;
  }

  bool fullscreen_layout_called() const { return fullscreen_layout_called_; }

 private:
  views::Widget* widget_;
  bool fullscreen_layout_called_;

  DISALLOW_COPY_AND_ASSIGN(FullscreenAwareFrame);
};

}  // namespace

// Tests that frame Layout is called when a widget goes fullscreen without
// changing its size or title.
TEST_F(WidgetTest, FullscreenFrameLayout) {
  Widget* widget = CreateTopLevelPlatformWidget();
  FullscreenAwareFrame* frame = new FullscreenAwareFrame(widget);
  widget->non_client_view()->SetFrameView(frame);  // Owns |frame|.

  widget->Maximize();
  RunPendingMessages();

  EXPECT_FALSE(frame->fullscreen_layout_called());
  widget->SetFullscreen(true);
  widget->Show();
  RunPendingMessages();
  EXPECT_TRUE(frame->fullscreen_layout_called());

  widget->CloseNow();
}

#if !defined(OS_CHROMEOS)
namespace {

// Trivial WidgetObserverTest that invokes Widget::IsActive() from
// OnWindowDestroying.
class IsActiveFromDestroyObserver : public WidgetObserver {
 public:
  IsActiveFromDestroyObserver() {}
  virtual ~IsActiveFromDestroyObserver() {}
  virtual void OnWidgetDestroying(Widget* widget) OVERRIDE {
    widget->IsActive();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(IsActiveFromDestroyObserver);
};

}  // namespace

// Verifies Widget::IsActive() invoked from
// WidgetObserver::OnWidgetDestroying() in a child widget doesn't crash.
TEST_F(WidgetTest, IsActiveFromDestroy) {
  // Create two widgets, one a child of the other.
  IsActiveFromDestroyObserver observer;
  Widget parent_widget;
  Widget::InitParams parent_params =
      CreateParams(Widget::InitParams::TYPE_POPUP);
  parent_params.native_widget = new PlatformDesktopNativeWidget(&parent_widget);
  parent_params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  parent_widget.Init(parent_params);
  parent_widget.Show();

  Widget child_widget;
  Widget::InitParams child_params =
      CreateParams(Widget::InitParams::TYPE_POPUP);
  child_params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  child_params.context = parent_widget.GetNativeWindow();
  child_widget.Init(child_params);
  child_widget.AddObserver(&observer);
  child_widget.Show();

  parent_widget.CloseNow();
}
#endif  // !defined(OS_CHROMEOS)

// Tests that events propagate through from the dispatcher with the correct
// event type, and that the different platforms behave the same.
TEST_F(WidgetTest, MouseEventTypesViaGenerator) {
  EventCountView* view = new EventCountView;
  view->set_handle_mode(EventCountView::CONSUME_EVENTS);
  view->SetBounds(10, 10, 50, 40);

  Widget* widget = CreateTopLevelFramelessPlatformWidget();
  widget->GetRootView()->AddChildView(view);

  widget->SetBounds(gfx::Rect(0, 0, 100, 80));
  widget->Show();

  ui::test::EventGenerator generator(GetContext(), widget->GetNativeWindow());
  generator.set_current_location(gfx::Point(20, 20));

  generator.ClickLeftButton();
  EXPECT_EQ(1, view->GetEventCount(ui::ET_MOUSE_PRESSED));
  EXPECT_EQ(1, view->GetEventCount(ui::ET_MOUSE_RELEASED));
  EXPECT_EQ(ui::EF_LEFT_MOUSE_BUTTON, view->last_flags());

  generator.PressRightButton();
  EXPECT_EQ(2, view->GetEventCount(ui::ET_MOUSE_PRESSED));
  EXPECT_EQ(1, view->GetEventCount(ui::ET_MOUSE_RELEASED));
  EXPECT_EQ(ui::EF_RIGHT_MOUSE_BUTTON, view->last_flags());

  generator.ReleaseRightButton();
  EXPECT_EQ(2, view->GetEventCount(ui::ET_MOUSE_PRESSED));
  EXPECT_EQ(2, view->GetEventCount(ui::ET_MOUSE_RELEASED));
  EXPECT_EQ(ui::EF_RIGHT_MOUSE_BUTTON, view->last_flags());

  // Test mouse move events.
  EXPECT_EQ(0, view->GetEventCount(ui::ET_MOUSE_MOVED));
  EXPECT_EQ(0, view->GetEventCount(ui::ET_MOUSE_ENTERED));

  // Move the mouse within the view (20, 20) -> (30, 30).
  generator.MoveMouseTo(gfx::Point(30, 30));
  EXPECT_EQ(1, view->GetEventCount(ui::ET_MOUSE_MOVED));
  EXPECT_EQ(1, view->GetEventCount(ui::ET_MOUSE_ENTERED));
  EXPECT_EQ(ui::EF_NONE, view->last_flags());

  // Move it again - entered count shouldn't change.
  generator.MoveMouseTo(gfx::Point(31, 31));
  EXPECT_EQ(2, view->GetEventCount(ui::ET_MOUSE_MOVED));
  EXPECT_EQ(1, view->GetEventCount(ui::ET_MOUSE_ENTERED));
  EXPECT_EQ(0, view->GetEventCount(ui::ET_MOUSE_EXITED));

  // Move it off the view.
  generator.MoveMouseTo(gfx::Point(5, 5));
  EXPECT_EQ(2, view->GetEventCount(ui::ET_MOUSE_MOVED));
  EXPECT_EQ(1, view->GetEventCount(ui::ET_MOUSE_ENTERED));
  EXPECT_EQ(1, view->GetEventCount(ui::ET_MOUSE_EXITED));

  // Move it back on.
  generator.MoveMouseTo(gfx::Point(20, 20));
  EXPECT_EQ(3, view->GetEventCount(ui::ET_MOUSE_MOVED));
  EXPECT_EQ(2, view->GetEventCount(ui::ET_MOUSE_ENTERED));
  EXPECT_EQ(1, view->GetEventCount(ui::ET_MOUSE_EXITED));

  // Drargging. Cover HasCapture() and NativeWidgetPrivate::IsMouseButtonDown().
  generator.DragMouseTo(gfx::Point(40, 40));
  EXPECT_EQ(3, view->GetEventCount(ui::ET_MOUSE_PRESSED));
  EXPECT_EQ(3, view->GetEventCount(ui::ET_MOUSE_RELEASED));
  EXPECT_EQ(1, view->GetEventCount(ui::ET_MOUSE_DRAGGED));
  EXPECT_EQ(ui::EF_LEFT_MOUSE_BUTTON, view->last_flags());

  widget->CloseNow();
}

}  // namespace test
}  // namespace views
