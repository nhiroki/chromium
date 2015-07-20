// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/keyboard/keyboard_controller.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/client/focus_client.h"
#include "ui/aura/layout_manager.h"
#include "ui/aura/test/aura_test_helper.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/ime/dummy_text_input_client.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/input_method_factory.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/layer_type.h"
#include "ui/compositor/scoped_animation_duration_scale_mode.h"
#include "ui/compositor/test/context_factories_for_test.h"
#include "ui/compositor/test/layer_animator_test_controller.h"
#include "ui/events/event_utils.h"
#include "ui/events/test/event_generator.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/keyboard/keyboard_controller_observer.h"
#include "ui/keyboard/keyboard_controller_proxy.h"
#include "ui/keyboard/keyboard_util.h"
#include "ui/wm/core/default_activation_client.h"

namespace keyboard {
namespace {

const int kDefaultVirtualKeyboardHeight = 100;

// Verify if the |keyboard| window covers the |container| window completely.
void VerifyKeyboardWindowSize(aura::Window* container, aura::Window* keyboard) {
  ASSERT_EQ(gfx::Rect(0, 0, container->bounds().width(),
                      container->bounds().height()),
            keyboard->bounds());
}

// Steps a layer animation until it is completed. Animations must be enabled.
void RunAnimationForLayer(ui::Layer* layer) {
  // Animations must be enabled for stepping to work.
  ASSERT_NE(ui::ScopedAnimationDurationScaleMode::duration_scale_mode(),
            ui::ScopedAnimationDurationScaleMode::ZERO_DURATION);

  ui::LayerAnimatorTestController controller(layer->GetAnimator());
  // Multiple steps are required to complete complex animations.
  // TODO(vollick): This should not be necessary. crbug.com/154017
  while (controller.animator()->is_animating()) {
    controller.StartThreadedAnimationsIfNeeded();
    base::TimeTicks step_time = controller.animator()->last_step_time();
    controller.animator()->Step(step_time +
                                base::TimeDelta::FromMilliseconds(1000));
  }
}

// An event handler that focuses a window when it is clicked/touched on. This is
// used to match the focus manger behaviour in ash and views.
class TestFocusController : public ui::EventHandler {
 public:
  explicit TestFocusController(aura::Window* root)
      : root_(root) {
    root_->AddPreTargetHandler(this);
  }

  ~TestFocusController() override { root_->RemovePreTargetHandler(this); }

 private:
  // Overridden from ui::EventHandler:
  void OnEvent(ui::Event* event) override {
    aura::Window* target = static_cast<aura::Window*>(event->target());
    if (event->type() == ui::ET_MOUSE_PRESSED ||
        event->type() == ui::ET_TOUCH_PRESSED) {
      aura::client::GetFocusClient(target)->FocusWindow(target);
    }
  }

  aura::Window* root_;
  DISALLOW_COPY_AND_ASSIGN(TestFocusController);
};

class TestKeyboardControllerProxy : public KeyboardControllerProxy {
 public:
  TestKeyboardControllerProxy(ui::InputMethod* input_method)
      : KeyboardControllerProxy(nullptr),
        input_method_(input_method) {}

  ~TestKeyboardControllerProxy() override {
    // Destroy the window before the delegate.
    window_.reset();
  }

  // Overridden from KeyboardControllerProxy:
  bool HasKeyboardWindow() const override { return window_; }
  aura::Window* GetKeyboardWindow() override {
    if (!window_) {
      window_.reset(new aura::Window(&delegate_));
      window_->Init(ui::LAYER_NOT_DRAWN);
      window_->set_owned_by_parent(false);
    }
    return window_.get();
  }
  ui::InputMethod* GetInputMethod() override { return input_method_; }
  void RequestAudioInput(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback) override {
    return;
  }
  void LoadSystemKeyboard() override{};
  void ReloadKeyboardIfNeeded() override{};

 private:
  scoped_ptr<aura::Window> window_;
  aura::test::TestWindowDelegate delegate_;
  ui::InputMethod* input_method_;

  DISALLOW_COPY_AND_ASSIGN(TestKeyboardControllerProxy);
};

// Keeps a count of all the events a window receives.
class EventObserver : public ui::EventHandler {
 public:
  EventObserver() {}
  ~EventObserver() override {}

  int GetEventCount(ui::EventType type) {
    return event_counts_[type];
  }

 private:
  // Overridden from ui::EventHandler:
  void OnEvent(ui::Event* event) override {
    ui::EventHandler::OnEvent(event);
    event_counts_[event->type()]++;
  }

  std::map<ui::EventType, int> event_counts_;
  DISALLOW_COPY_AND_ASSIGN(EventObserver);
};

class KeyboardContainerObserver : public aura::WindowObserver {
 public:
  explicit KeyboardContainerObserver(aura::Window* window) : window_(window) {
    window_->AddObserver(this);
  }
  ~KeyboardContainerObserver() override { window_->RemoveObserver(this); }

 private:
  void OnWindowVisibilityChanged(aura::Window* window, bool visible) override {
    if (!visible)
      base::MessageLoop::current()->Quit();
  }

  aura::Window* window_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardContainerObserver);
};

}  // namespace

class KeyboardControllerTest : public testing::Test,
                               public KeyboardControllerObserver {
 public:
  KeyboardControllerTest() : number_of_calls_(0) {}
  ~KeyboardControllerTest() override {}

  void SetUp() override {
    // The ContextFactory must exist before any Compositors are created.
    bool enable_pixel_output = false;
    ui::ContextFactory* context_factory =
        ui::InitializeContextFactoryForTests(enable_pixel_output);

    ui::SetUpInputMethodFactoryForTesting();
    aura_test_helper_.reset(new aura::test::AuraTestHelper(&message_loop_));
    aura_test_helper_->SetUp(context_factory);
    new wm::DefaultActivationClient(aura_test_helper_->root_window());
    focus_controller_.reset(new TestFocusController(root_window()));
    proxy_ = new TestKeyboardControllerProxy(
        aura_test_helper_->host()->GetInputMethod());
    controller_.reset(new KeyboardController(proxy_));
    controller()->AddObserver(this);
  }

  void TearDown() override {
    controller()->RemoveObserver(this);
    controller_.reset();
    focus_controller_.reset();
    aura_test_helper_->TearDown();
    ui::TerminateContextFactoryForTests();
  }

  aura::Window* root_window() { return aura_test_helper_->root_window(); }
  KeyboardControllerProxy* proxy() { return proxy_; }
  KeyboardController* controller() { return controller_.get(); }

  void ShowKeyboard() {
    test_text_input_client_.reset(
        new ui::DummyTextInputClient(ui::TEXT_INPUT_TYPE_TEXT));
    SetFocus(test_text_input_client_.get());
  }

  void MockRotateScreen() {
    const gfx::Rect root_bounds = root_window()->bounds();
    controller_->OnWindowBoundsChanged(root_window(), gfx::Rect(),
                                       gfx::Rect(0,
                                                 0,
                                                 root_bounds.height(),
                                                 root_bounds.width()));
  }

 protected:
  // KeyboardControllerObserver overrides
  void OnKeyboardBoundsChanging(const gfx::Rect& new_bounds) override {
    notified_bounds_ = new_bounds;
    number_of_calls_++;
  }

  int number_of_calls() const { return number_of_calls_; }

  const gfx::Rect& notified_bounds() { return notified_bounds_; }

  void SetFocus(ui::TextInputClient* client) {
    ui::InputMethod* input_method = proxy()->GetInputMethod();
    input_method->SetFocusedTextInputClient(client);
    if (client && client->GetTextInputType() != ui::TEXT_INPUT_TYPE_NONE) {
      input_method->ShowImeIfNeeded();
      if (proxy_->GetKeyboardWindow()->bounds().height() == 0) {
        // Set initial bounds for test keyboard window.
        proxy_->GetKeyboardWindow()->SetBounds(
            FullWidthKeyboardBoundsFromRootBounds(
                root_window()->bounds(), kDefaultVirtualKeyboardHeight));
      }
    }
  }

  bool WillHideKeyboard() {
    return controller_->WillHideKeyboard();
  }

  bool ShouldEnableInsets(aura::Window* window) {
    return controller_->ShouldEnableInsets(window);
  }

  base::MessageLoopForUI message_loop_;
  scoped_ptr<aura::test::AuraTestHelper> aura_test_helper_;
  scoped_ptr<TestFocusController> focus_controller_;

 private:
  int number_of_calls_;
  gfx::Rect notified_bounds_;
  KeyboardControllerProxy* proxy_;
  scoped_ptr<KeyboardController> controller_;
  scoped_ptr<ui::TextInputClient> test_text_input_client_;
  DISALLOW_COPY_AND_ASSIGN(KeyboardControllerTest);
};

TEST_F(KeyboardControllerTest, KeyboardSize) {
  aura::Window* container(controller()->GetContainerWindow());
  aura::Window* keyboard(proxy()->GetKeyboardWindow());
  gfx::Rect screen_bounds = root_window()->bounds();
  root_window()->AddChild(container);
  container->AddChild(keyboard);
  const gfx::Rect& initial_bounds = container->bounds();
  // The container should be positioned at the bottom of screen and has 0
  // height.
  ASSERT_EQ(gfx::Rect(), initial_bounds);
  VerifyKeyboardWindowSize(container, keyboard);


  // In FULL_WIDTH mode, attempt to change window width or move window up from
  // the bottom are ignored. Changing window height is supported.
  gfx::Rect expected_bounds(0,
                            screen_bounds.height() - 50,
                            screen_bounds.width(),
                            50);

  // The x position of new bounds may not be 0 if shelf is on the left side of
  // screen. In FULL_WIDTH mode, the virtual keyboard should always align with
  // the left edge of screen. See http://crbug.com/510595.
  gfx::Rect new_bounds(10, 0, 50, 50);
  keyboard->SetBounds(new_bounds);
  ASSERT_EQ(expected_bounds, container->bounds());
  VerifyKeyboardWindowSize(container, keyboard);

  MockRotateScreen();
  // The above call should resize keyboard to new width while keeping the old
  // height.
  ASSERT_EQ(gfx::Rect(0,
                      screen_bounds.width() - 50,
                      screen_bounds.height(),
                      50),
            container->bounds());
  VerifyKeyboardWindowSize(container, keyboard);
}

TEST_F(KeyboardControllerTest, FloatingKeyboardSize) {
  aura::Window* container(controller()->GetContainerWindow());
  aura::Window* keyboard(proxy()->GetKeyboardWindow());
  gfx::Rect screen_bounds = root_window()->bounds();
  root_window()->AddChild(container);
  controller()->SetKeyboardMode(FLOATING);
  container->AddChild(keyboard);
  gfx::Rect new_bounds(0, 50, 50, 50);
  keyboard->SetBounds(new_bounds);
  ASSERT_EQ(new_bounds, container->bounds());
  VerifyKeyboardWindowSize(container, keyboard);
}

// Tests that tapping/clicking inside the keyboard does not give it focus.
TEST_F(KeyboardControllerTest, ClickDoesNotFocusKeyboard) {
  const gfx::Rect& root_bounds = root_window()->bounds();
  aura::test::EventCountDelegate delegate;
  scoped_ptr<aura::Window> window(new aura::Window(&delegate));
  window->Init(ui::LAYER_NOT_DRAWN);
  window->SetBounds(root_bounds);
  root_window()->AddChild(window.get());
  window->Show();
  window->Focus();

  aura::Window* keyboard_container(controller()->GetContainerWindow());

  root_window()->AddChild(keyboard_container);
  keyboard_container->Show();

  ShowKeyboard();

  EXPECT_TRUE(window->IsVisible());
  EXPECT_TRUE(keyboard_container->IsVisible());
  EXPECT_TRUE(window->HasFocus());
  EXPECT_FALSE(keyboard_container->HasFocus());

  // Click on the keyboard. Make sure the keyboard receives the event, but does
  // not get focus.
  EventObserver observer;
  keyboard_container->AddPreTargetHandler(&observer);

  ui::test::EventGenerator generator(root_window());
  generator.MoveMouseTo(keyboard_container->bounds().CenterPoint());
  generator.ClickLeftButton();
  EXPECT_TRUE(window->HasFocus());
  EXPECT_FALSE(keyboard_container->HasFocus());
  EXPECT_EQ("0 0", delegate.GetMouseButtonCountsAndReset());
  EXPECT_EQ(1, observer.GetEventCount(ui::ET_MOUSE_PRESSED));
  EXPECT_EQ(1, observer.GetEventCount(ui::ET_MOUSE_RELEASED));

  // Click outside of the keyboard. It should reach the window behind.
  generator.MoveMouseTo(gfx::Point());
  generator.ClickLeftButton();
  EXPECT_EQ("1 1", delegate.GetMouseButtonCountsAndReset());
  keyboard_container->RemovePreTargetHandler(&observer);
}

TEST_F(KeyboardControllerTest, VisibilityChangeWithTextInputTypeChange) {
  ui::DummyTextInputClient input_client_0(ui::TEXT_INPUT_TYPE_TEXT);
  ui::DummyTextInputClient input_client_1(ui::TEXT_INPUT_TYPE_TEXT);
  ui::DummyTextInputClient input_client_2(ui::TEXT_INPUT_TYPE_TEXT);
  ui::DummyTextInputClient no_input_client_0(ui::TEXT_INPUT_TYPE_NONE);
  ui::DummyTextInputClient no_input_client_1(ui::TEXT_INPUT_TYPE_NONE);

  aura::Window* keyboard_container(controller()->GetContainerWindow());
  scoped_ptr<KeyboardContainerObserver> keyboard_container_observer(
      new KeyboardContainerObserver(keyboard_container));
  root_window()->AddChild(keyboard_container);

  SetFocus(&input_client_0);

  EXPECT_TRUE(keyboard_container->IsVisible());

  SetFocus(&no_input_client_0);
  // Keyboard should not immediately hide itself. It is delayed to avoid layout
  // flicker when the focus of input field quickly change.
  EXPECT_TRUE(keyboard_container->IsVisible());
  EXPECT_TRUE(WillHideKeyboard());
  // Wait for hide keyboard to finish.
  base::MessageLoop::current()->Run();
  EXPECT_FALSE(keyboard_container->IsVisible());

  SetFocus(&input_client_1);
  EXPECT_TRUE(keyboard_container->IsVisible());

  // Schedule to hide keyboard.
  SetFocus(&no_input_client_1);
  EXPECT_TRUE(WillHideKeyboard());
  // Cancel keyboard hide.
  SetFocus(&input_client_2);

  EXPECT_FALSE(WillHideKeyboard());
  EXPECT_TRUE(keyboard_container->IsVisible());
}

// Test to prevent spurious overscroll boxes when changing tabs during keyboard
// hide. Refer to crbug.com/401670 for more context.
TEST_F(KeyboardControllerTest, CheckOverscrollInsetDuringVisibilityChange) {
  ui::DummyTextInputClient input_client(ui::TEXT_INPUT_TYPE_TEXT);
  ui::DummyTextInputClient no_input_client(ui::TEXT_INPUT_TYPE_NONE);

  aura::Window* keyboard_container(controller()->GetContainerWindow());
  root_window()->AddChild(keyboard_container);

  // Enable touch keyboard / overscroll mode to test insets.
  keyboard::SetTouchKeyboardEnabled(true);
  EXPECT_TRUE(keyboard::IsKeyboardOverscrollEnabled());

  SetFocus(&input_client);
  SetFocus(&no_input_client);
  // Insets should not be enabled for new windows while keyboard is in the
  // process of hiding when overscroll is enabled.
  EXPECT_FALSE(ShouldEnableInsets(proxy()->GetKeyboardWindow()));
  // Cancel keyboard hide.
  SetFocus(&input_client);
  // Insets should be enabled for new windows as hide was cancelled.
  EXPECT_TRUE(ShouldEnableInsets(proxy()->GetKeyboardWindow()));
}

// Verify switch to FLOATING mode will reset the overscroll or resize and when
// in FLOATING mode, overscroll or resize wont be triggered.
TEST_F(KeyboardControllerTest, FloatingKeyboardDontOverscrollOrResize) {
  ui::DummyTextInputClient input_client(ui::TEXT_INPUT_TYPE_TEXT);
  ui::DummyTextInputClient no_input_client(ui::TEXT_INPUT_TYPE_NONE);

  aura::Window* container(controller()->GetContainerWindow());
  root_window()->AddChild(container);
  scoped_ptr<KeyboardContainerObserver> keyboard_container_observer(
      new KeyboardContainerObserver(container));
  gfx::Rect screen_bounds = root_window()->bounds();
  keyboard::SetTouchKeyboardEnabled(true);

  SetFocus(&input_client);
  gfx::Rect expected_bounds(
      0, screen_bounds.height() - kDefaultVirtualKeyboardHeight,
      screen_bounds.width(), kDefaultVirtualKeyboardHeight);
  // Verify overscroll or resize is in effect.
  EXPECT_EQ(expected_bounds, notified_bounds());
  EXPECT_EQ(1, number_of_calls());

  controller()->SetKeyboardMode(FLOATING);
  // Switch to FLOATING should clear overscroll or resize.
  EXPECT_EQ(gfx::Rect(), notified_bounds());
  EXPECT_EQ(2, number_of_calls());
  SetFocus(&no_input_client);
  base::MessageLoop::current()->Run();
  EXPECT_EQ(gfx::Rect(), notified_bounds());
  EXPECT_EQ(3, number_of_calls());
  SetFocus(&input_client);
  // In FLOATING mode, no overscroll or resize should be triggered.
  EXPECT_EQ(3, number_of_calls());
  EXPECT_EQ(gfx::Rect(), controller()->current_keyboard_bounds());
}

// Verify switch to FULL_WIDTH mode will move virtual keyboard to the right
// place and sets the correct overscroll.
TEST_F(KeyboardControllerTest, SwitchToFullWidthVirtualKeyboard) {
  ui::DummyTextInputClient input_client(ui::TEXT_INPUT_TYPE_TEXT);

  aura::Window* container(controller()->GetContainerWindow());
  root_window()->AddChild(container);
  gfx::Rect screen_bounds = root_window()->bounds();
  keyboard::SetTouchKeyboardEnabled(true);
  SetFocus(&input_client);

  controller()->SetKeyboardMode(FLOATING);
  EXPECT_EQ(gfx::Rect(), notified_bounds());
  EXPECT_EQ(gfx::Rect(), controller()->current_keyboard_bounds());

  controller()->SetKeyboardMode(FULL_WIDTH);
  gfx::Rect expected_bounds(
      0, screen_bounds.height() - kDefaultVirtualKeyboardHeight,
      screen_bounds.width(), kDefaultVirtualKeyboardHeight);
  EXPECT_EQ(expected_bounds, notified_bounds());
  EXPECT_EQ(expected_bounds, controller()->current_keyboard_bounds());
}

TEST_F(KeyboardControllerTest, AlwaysVisibleWhenLocked) {
  ui::DummyTextInputClient input_client_0(ui::TEXT_INPUT_TYPE_TEXT);
  ui::DummyTextInputClient input_client_1(ui::TEXT_INPUT_TYPE_TEXT);
  ui::DummyTextInputClient no_input_client_0(ui::TEXT_INPUT_TYPE_NONE);
  ui::DummyTextInputClient no_input_client_1(ui::TEXT_INPUT_TYPE_NONE);

  aura::Window* keyboard_container(controller()->GetContainerWindow());
  scoped_ptr<KeyboardContainerObserver> keyboard_container_observer(
      new KeyboardContainerObserver(keyboard_container));
  root_window()->AddChild(keyboard_container);

  SetFocus(&input_client_0);

  EXPECT_TRUE(keyboard_container->IsVisible());

  // Lock keyboard.
  controller()->set_lock_keyboard(true);

  SetFocus(&no_input_client_0);
  // Keyboard should not try to hide itself as it is locked.
  EXPECT_TRUE(keyboard_container->IsVisible());
  EXPECT_FALSE(WillHideKeyboard());

  SetFocus(&input_client_1);
  EXPECT_TRUE(keyboard_container->IsVisible());

  // Unlock keyboard.
  controller()->set_lock_keyboard(false);

  // Keyboard should hide when focus on no input client.
  SetFocus(&no_input_client_1);
  EXPECT_TRUE(WillHideKeyboard());

  // Wait for hide keyboard to finish.
  base::MessageLoop::current()->Run();
  EXPECT_FALSE(keyboard_container->IsVisible());
}

class KeyboardControllerAnimationTest : public KeyboardControllerTest {
 public:
  KeyboardControllerAnimationTest() {}
  ~KeyboardControllerAnimationTest() override {}

  void SetUp() override {
    // We cannot short-circuit animations for this test.
    ui::ScopedAnimationDurationScaleMode test_duration_mode(
        ui::ScopedAnimationDurationScaleMode::NON_ZERO_DURATION);

    KeyboardControllerTest::SetUp();

    const gfx::Rect& root_bounds = root_window()->bounds();
    keyboard_container()->SetBounds(root_bounds);
    root_window()->AddChild(keyboard_container());
  }

  void TearDown() override {
    KeyboardControllerTest::TearDown();
  }

 protected:
  aura::Window* keyboard_container() {
    return controller()->GetContainerWindow();
  }

  aura::Window* keyboard_window() {
    return proxy()->GetKeyboardWindow();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyboardControllerAnimationTest);
};

// Tests virtual keyboard has correct show and hide animation.
TEST_F(KeyboardControllerAnimationTest, ContainerAnimation) {
  ui::Layer* layer = keyboard_container()->layer();
  ShowKeyboard();

  // Keyboard container and window should immediately become visible before
  // animation starts.
  EXPECT_TRUE(keyboard_container()->IsVisible());
  EXPECT_TRUE(keyboard_window()->IsVisible());
  float show_start_opacity = layer->opacity();
  gfx::Transform transform;
  transform.Translate(0, kAnimationDistance);
  EXPECT_EQ(transform, layer->transform());
  EXPECT_EQ(gfx::Rect(), notified_bounds());

  RunAnimationForLayer(layer);
  EXPECT_TRUE(keyboard_container()->IsVisible());
  EXPECT_TRUE(keyboard_window()->IsVisible());
  float show_end_opacity = layer->opacity();
  EXPECT_LT(show_start_opacity, show_end_opacity);
  EXPECT_EQ(gfx::Transform(), layer->transform());
  // KeyboardController should notify the bounds of container window to its
  // observers after show animation finished.
  EXPECT_EQ(keyboard_container()->bounds(), notified_bounds());

  // Directly hide keyboard without delay.
  controller()->HideKeyboard(KeyboardController::HIDE_REASON_AUTOMATIC);
  EXPECT_TRUE(keyboard_container()->IsVisible());
  EXPECT_TRUE(keyboard_container()->layer()->visible());
  EXPECT_TRUE(keyboard_window()->IsVisible());
  float hide_start_opacity = layer->opacity();
  // KeyboardController should notify the bounds of keyboard window to its
  // observers before hide animation starts.
  EXPECT_EQ(gfx::Rect(), notified_bounds());

  RunAnimationForLayer(layer);
  EXPECT_FALSE(keyboard_container()->IsVisible());
  EXPECT_FALSE(keyboard_container()->layer()->visible());
  EXPECT_FALSE(keyboard_window()->IsVisible());
  float hide_end_opacity = layer->opacity();
  EXPECT_GT(hide_start_opacity, hide_end_opacity);
  EXPECT_EQ(transform, layer->transform());
  EXPECT_EQ(gfx::Rect(), notified_bounds());
}

// Show keyboard during keyboard hide animation should abort the hide animation
// and the keyboard should animate in.
// Test for crbug.com/333284.
TEST_F(KeyboardControllerAnimationTest, ContainerShowWhileHide) {
  ui::Layer* layer = keyboard_container()->layer();
  ShowKeyboard();
  RunAnimationForLayer(layer);

  controller()->HideKeyboard(KeyboardController::HIDE_REASON_AUTOMATIC);
  // Before hide animation finishes, show keyboard again.
  ShowKeyboard();
  RunAnimationForLayer(layer);
  EXPECT_TRUE(keyboard_container()->IsVisible());
  EXPECT_TRUE(keyboard_window()->IsVisible());
  EXPECT_EQ(1.0, layer->opacity());
  EXPECT_EQ(gfx::Transform(), layer->transform());
}

}  // namespace keyboard
