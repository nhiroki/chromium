// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_NATIVE_WIDGET_AURA_H_
#define UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_NATIVE_WIDGET_AURA_H_

#include "base/memory/weak_ptr.h"
#include "ui/aura/client/focus_change_observer.h"
#include "ui/aura/window_delegate.h"
#include "ui/aura/window_tree_host_observer.h"
#include "ui/base/cursor/cursor.h"
#include "ui/views/ime/input_method_delegate.h"
#include "ui/views/widget/native_widget_private.h"
#include "ui/wm/core/compound_event_filter.h"
#include "ui/wm/public/activation_change_observer.h"
#include "ui/wm/public/activation_delegate.h"
#include "ui/wm/public/drag_drop_delegate.h"

namespace aura {
class WindowEventDispatcher;
class WindowTreeHost;
namespace client {
class DragDropClient;
class FocusClient;
class ScreenPositionClient;
class WindowTreeClient;
}
}

namespace wm {
class CompoundEventFilter;
class CursorManager;
class FocusController;
class InputMethodEventFilter;
class ShadowController;
class VisibilityController;
class WindowModalityController;
}

namespace views {
namespace corewm {
class TooltipController;
}
class DesktopCaptureClient;
class DesktopDispatcherClient;
class DesktopEventClient;
class DesktopNativeCursorManager;
class DesktopWindowTreeHost;
class DropHelper;
class FocusManagerEventHandler;
class TooltipManagerAura;
class WindowReorderer;

class VIEWS_EXPORT DesktopNativeWidgetAura
    : public internal::NativeWidgetPrivate,
      public aura::WindowDelegate,
      public aura::client::ActivationDelegate,
      public aura::client::ActivationChangeObserver,
      public aura::client::FocusChangeObserver,
      public views::internal::InputMethodDelegate,
      public aura::client::DragDropDelegate,
      public aura::WindowTreeHostObserver {
 public:
  explicit DesktopNativeWidgetAura(internal::NativeWidgetDelegate* delegate);
  ~DesktopNativeWidgetAura() override;

  // Maps from window to DesktopNativeWidgetAura. |window| must be a root
  // window.
  static DesktopNativeWidgetAura* ForWindow(aura::Window* window);

  // Called by our DesktopWindowTreeHost after it has deleted native resources;
  // this is the signal that we should start our shutdown.
  virtual void OnHostClosed();

  // TODO(beng): remove this method and replace with an implementation of
  //             WindowDestroying() that takes the window being destroyed.
  // Called from ~DesktopWindowTreeHost. This takes the WindowEventDispatcher
  // as by the time we get here |dispatcher_| is NULL.
  virtual void OnDesktopWindowTreeHostDestroyed(aura::WindowTreeHost* host);

  wm::InputMethodEventFilter* input_method_event_filter() {
    return input_method_event_filter_.get();
  }
  wm::CompoundEventFilter* root_window_event_filter() {
    return root_window_event_filter_.get();
  }
  aura::WindowTreeHost* host() {
    return host_.get();
  }

  // Ensures that the correct window is activated/deactivated based on whether
  // we are being activated/deactivated.
  void HandleActivationChanged(bool active);

 protected:
  // Overridden from internal::NativeWidgetPrivate:
  void InitNativeWidget(const Widget::InitParams& params) override;
  NonClientFrameView* CreateNonClientFrameView() override;
  bool ShouldUseNativeFrame() const override;
  bool ShouldWindowContentsBeTransparent() const override;
  void FrameTypeChanged() override;
  Widget* GetWidget() override;
  const Widget* GetWidget() const override;
  gfx::NativeView GetNativeView() const override;
  gfx::NativeWindow GetNativeWindow() const override;
  Widget* GetTopLevelWidget() override;
  const ui::Compositor* GetCompositor() const override;
  const ui::Layer* GetLayer() const override;
  void ReorderNativeViews() override;
  void ViewRemoved(View* view) override;
  void SetNativeWindowProperty(const char* name, void* value) override;
  void* GetNativeWindowProperty(const char* name) const override;
  TooltipManager* GetTooltipManager() const override;
  void SetCapture() override;
  void ReleaseCapture() override;
  bool HasCapture() const override;
  InputMethod* CreateInputMethod() override;
  internal::InputMethodDelegate* GetInputMethodDelegate() override;
  ui::InputMethod* GetHostInputMethod() override;
  void CenterWindow(const gfx::Size& size) override;
  void GetWindowPlacement(gfx::Rect* bounds,
                          ui::WindowShowState* maximized) const override;
  bool SetWindowTitle(const base::string16& title) override;
  void SetWindowIcons(const gfx::ImageSkia& window_icon,
                      const gfx::ImageSkia& app_icon) override;
  void InitModalType(ui::ModalType modal_type) override;
  gfx::Rect GetWindowBoundsInScreen() const override;
  gfx::Rect GetClientAreaBoundsInScreen() const override;
  gfx::Rect GetRestoredBounds() const override;
  void SetBounds(const gfx::Rect& bounds) override;
  void SetSize(const gfx::Size& size) override;
  void StackAbove(gfx::NativeView native_view) override;
  void StackAtTop() override;
  void StackBelow(gfx::NativeView native_view) override;
  void SetShape(gfx::NativeRegion shape) override;
  void Close() override;
  void CloseNow() override;
  void Show() override;
  void Hide() override;
  void ShowMaximizedWithBounds(const gfx::Rect& restored_bounds) override;
  void ShowWithWindowState(ui::WindowShowState state) override;
  bool IsVisible() const override;
  void Activate() override;
  void Deactivate() override;
  bool IsActive() const override;
  void SetAlwaysOnTop(bool always_on_top) override;
  bool IsAlwaysOnTop() const override;
  void SetVisibleOnAllWorkspaces(bool always_visible) override;
  void Maximize() override;
  void Minimize() override;
  bool IsMaximized() const override;
  bool IsMinimized() const override;
  void Restore() override;
  void SetFullscreen(bool fullscreen) override;
  bool IsFullscreen() const override;
  void SetOpacity(unsigned char opacity) override;
  void SetUseDragFrame(bool use_drag_frame) override;
  void FlashFrame(bool flash_frame) override;
  void RunShellDrag(View* view,
                    const ui::OSExchangeData& data,
                    const gfx::Point& location,
                    int operation,
                    ui::DragDropTypes::DragEventSource source) override;
  void SchedulePaintInRect(const gfx::Rect& rect) override;
  void SetCursor(gfx::NativeCursor cursor) override;
  bool IsMouseEventsEnabled() const override;
  void ClearNativeFocus() override;
  gfx::Rect GetWorkAreaBoundsInScreen() const override;
  Widget::MoveLoopResult RunMoveLoop(
      const gfx::Vector2d& drag_offset,
      Widget::MoveLoopSource source,
      Widget::MoveLoopEscapeBehavior escape_behavior) override;
  void EndMoveLoop() override;
  void SetVisibilityChangedAnimationsEnabled(bool value) override;
  void SetVisibilityAnimationDuration(const base::TimeDelta& duration) override;
  void SetVisibilityAnimationTransition(
      Widget::VisibilityTransition transition) override;
  ui::NativeTheme* GetNativeTheme() const override;
  void OnRootViewLayout() override;
  bool IsTranslucentWindowOpacitySupported() const override;
  void OnSizeConstraintsChanged() override;
  void RepostNativeEvent(gfx::NativeEvent native_event) override;

  // Overridden from aura::WindowDelegate:
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;
  void OnBoundsChanged(const gfx::Rect& old_bounds,
                       const gfx::Rect& new_bounds) override {}
  gfx::NativeCursor GetCursor(const gfx::Point& point) override;
  int GetNonClientComponent(const gfx::Point& point) const override;
  bool ShouldDescendIntoChildForEventHandling(
      aura::Window* child,
      const gfx::Point& location) override;
  bool CanFocus() override;
  void OnCaptureLost() override;
  void OnPaint(gfx::Canvas* canvas) override;
  void OnDeviceScaleFactorChanged(float device_scale_factor) override;
  void OnWindowDestroying(aura::Window* window) override;
  void OnWindowDestroyed(aura::Window* window) override;
  void OnWindowTargetVisibilityChanged(bool visible) override;
  bool HasHitTestMask() const override;
  void GetHitTestMask(gfx::Path* mask) const override;

  // Overridden from ui::EventHandler:
  void OnKeyEvent(ui::KeyEvent* event) override;
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnScrollEvent(ui::ScrollEvent* event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

  // Overridden from aura::client::ActivationDelegate:
  bool ShouldActivate() const override;

  // Overridden from aura::client::ActivationChangeObserver:
  void OnWindowActivated(aura::Window* gained_active,
                         aura::Window* lost_active) override;

  // Overridden from aura::client::FocusChangeObserver:
  void OnWindowFocused(aura::Window* gained_focus,
                       aura::Window* lost_focus) override;

  // Overridden from views::internal::InputMethodDelegate:
  void DispatchKeyEventPostIME(const ui::KeyEvent& key) override;

  // Overridden from aura::client::DragDropDelegate:
  void OnDragEntered(const ui::DropTargetEvent& event) override;
  int OnDragUpdated(const ui::DropTargetEvent& event) override;
  void OnDragExited() override;
  int OnPerformDrop(const ui::DropTargetEvent& event) override;

  // Overridden from aura::WindowTreeHostObserver:
  void OnHostCloseRequested(const aura::WindowTreeHost* host) override;
  void OnHostResized(const aura::WindowTreeHost* host) override;
  void OnHostMoved(const aura::WindowTreeHost* host,
                   const gfx::Point& new_origin) override;

 private:
  friend class FocusManagerEventHandler;
  friend class RootWindowDestructionObserver;

  // Installs the input method filter.
  void InstallInputMethodEventFilter();

  // To save a clear on platforms where the window is never transparent, the
  // window is only set as transparent when the glass frame is in use.
  void UpdateWindowTransparency();

  void RootWindowDestroyed();

  scoped_ptr<aura::WindowTreeHost> host_;
  DesktopWindowTreeHost* desktop_window_tree_host_;

  // See class documentation for Widget in widget.h for a note about ownership.
  Widget::InitParams::Ownership ownership_;

  scoped_ptr<DesktopCaptureClient> capture_client_;

  // Child of the root, contains |content_window_|.
  aura::Window* content_window_container_;

  // Child of |content_window_container_|. This is the return value from
  // GetNativeView().
  // WARNING: this may be NULL, in particular during shutdown it becomes NULL.
  aura::Window* content_window_;

  internal::NativeWidgetDelegate* native_widget_delegate_;

  scoped_ptr<wm::FocusController> focus_client_;
  scoped_ptr<DesktopDispatcherClient> dispatcher_client_;
  scoped_ptr<aura::client::ScreenPositionClient> position_client_;
  scoped_ptr<aura::client::DragDropClient> drag_drop_client_;
  scoped_ptr<aura::client::WindowTreeClient> window_tree_client_;
  scoped_ptr<DesktopEventClient> event_client_;
  scoped_ptr<FocusManagerEventHandler> focus_manager_event_handler_;

  // Toplevel event filter which dispatches to other event filters.
  scoped_ptr<wm::CompoundEventFilter> root_window_event_filter_;

  scoped_ptr<wm::InputMethodEventFilter> input_method_event_filter_;

  scoped_ptr<DropHelper> drop_helper_;
  int last_drop_operation_;

  scoped_ptr<corewm::TooltipController> tooltip_controller_;
  scoped_ptr<TooltipManagerAura> tooltip_manager_;

  scoped_ptr<wm::VisibilityController> visibility_controller_;

  scoped_ptr<wm::WindowModalityController>
      window_modality_controller_;

  bool restore_focus_on_activate_;

  gfx::NativeCursor cursor_;
  // We must manually reference count the number of users of |cursor_manager_|
  // because the cursors created by |cursor_manager_| are shared among the
  // DNWAs. We can't just stuff this in a LazyInstance because we need to
  // destroy this as the last DNWA happens; we can't put it off until
  // (potentially) after we tear down the X11 connection because that's a
  // crash.
  static int cursor_reference_count_;
  static wm::CursorManager* cursor_manager_;
  static views::DesktopNativeCursorManager* native_cursor_manager_;

  scoped_ptr<wm::ShadowController> shadow_controller_;

  // Reorders child windows of |window_| associated with a view based on the
  // order of the associated views in the widget's view hierarchy.
  scoped_ptr<WindowReorderer> window_reorderer_;

  // See class documentation for Widget in widget.h for a note about type.
  Widget::InitParams::Type widget_type_;

  // The following factory is used for calls to close the NativeWidgetAura
  // instance.
  base::WeakPtrFactory<DesktopNativeWidgetAura> close_widget_factory_;

  DISALLOW_COPY_AND_ASSIGN(DesktopNativeWidgetAura);
};

}  // namespace views

#endif  // UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_NATIVE_WIDGET_AURA_H_
