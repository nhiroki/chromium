// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_NATIVE_WIDGET_PRIVATE_H_
#define UI_VIEWS_WIDGET_NATIVE_WIDGET_PRIVATE_H_

#include "base/strings/string16.h"
#include "ui/base/ui_base_types.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/widget/native_widget.h"

namespace gfx {
class FontList;
class ImageSkia;
class Rect;
}

namespace ui {
class InputMethod;
class NativeTheme;
class OSExchangeData;
}

namespace views {
class InputMethod;
class TooltipManager;
namespace internal {
class InputMethodDelegate;

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetPrivate interface
//
//  A NativeWidget subclass internal to views that provides Widget a conduit for
//  communication with a backend-specific native widget implementation.
//
//  Many of the methods here are pass-thrus for Widget, and as such there is no
//  documentation for them here. In that case, see methods of the same name in
//  widget.h.
//
//  IMPORTANT: This type is intended for use only by the views system and for
//             NativeWidget implementations. This file should not be included
//             in code that does not fall into one of these use cases.
//
class VIEWS_EXPORT NativeWidgetPrivate : public NativeWidget {
 public:
  virtual ~NativeWidgetPrivate() {}

  // Creates an appropriate default NativeWidgetPrivate implementation for the
  // current OS/circumstance.
  static NativeWidgetPrivate* CreateNativeWidget(
      internal::NativeWidgetDelegate* delegate);

  static NativeWidgetPrivate* GetNativeWidgetForNativeView(
      gfx::NativeView native_view);
  static NativeWidgetPrivate* GetNativeWidgetForNativeWindow(
      gfx::NativeWindow native_window);

  // Retrieves the top NativeWidgetPrivate in the hierarchy containing the given
  // NativeView, or NULL if there is no NativeWidgetPrivate that contains it.
  static NativeWidgetPrivate* GetTopLevelNativeWidget(
      gfx::NativeView native_view);

  static void GetAllChildWidgets(gfx::NativeView native_view,
                                 Widget::Widgets* children);
  static void GetAllOwnedWidgets(gfx::NativeView native_view,
                                 Widget::Widgets* owned);
  static void ReparentNativeView(gfx::NativeView native_view,
                                 gfx::NativeView new_parent);

  // Returns true if any mouse button is currently down.
  static bool IsMouseButtonDown();

  // Returns true if any touch device is currently down.
  static bool IsTouchDown();

  static gfx::FontList GetWindowTitleFontList();

  // Initializes the NativeWidget.
  virtual void InitNativeWidget(const Widget::InitParams& params) = 0;

  // Returns a NonClientFrameView for the widget's NonClientView, or NULL if
  // the NativeWidget wants no special NonClientFrameView.
  virtual NonClientFrameView* CreateNonClientFrameView() = 0;

  virtual bool ShouldUseNativeFrame() const = 0;
  virtual bool ShouldWindowContentsBeTransparent() const = 0;
  virtual void FrameTypeChanged() = 0;

  // Returns the Widget associated with this NativeWidget. This function is
  // guaranteed to return non-NULL for the lifetime of the NativeWidget.
  virtual Widget* GetWidget() = 0;
  virtual const Widget* GetWidget() const = 0;

  // Returns the NativeView/Window associated with this NativeWidget.
  virtual gfx::NativeView GetNativeView() const = 0;
  virtual gfx::NativeWindow GetNativeWindow() const = 0;

  // Returns the topmost Widget in a hierarchy.
  virtual Widget* GetTopLevelWidget() = 0;

  // Returns the Compositor, or NULL if there isn't one associated with this
  // NativeWidget.
  virtual const ui::Compositor* GetCompositor() const = 0;
  virtual ui::Compositor* GetCompositor() = 0;

  // Returns the NativeWidget's layer, if any.
  virtual ui::Layer* GetLayer() = 0;

  // Reorders the widget's child NativeViews which are associated to the view
  // tree (eg via a NativeViewHost) to match the z-order of the views in the
  // view tree. The z-order of views with layers relative to views with
  // associated NativeViews is used to reorder the NativeView layers. This
  // method assumes that the widget's child layers which are owned by a view are
  // already in the correct z-order relative to each other and does no
  // reordering if there are no views with an associated NativeView.
  virtual void ReorderNativeViews() = 0;

  // Notifies the NativeWidget that a view was removed from the Widget's view
  // hierarchy.
  virtual void ViewRemoved(View* view) = 0;

  // Sets/Gets a native window property on the underlying native window object.
  // Returns NULL if the property does not exist. Setting the property value to
  // NULL removes the property.
  virtual void SetNativeWindowProperty(const char* name, void* value) = 0;
  virtual void* GetNativeWindowProperty(const char* name) const = 0;

  // Returns the native widget's tooltip manager. Called from the View hierarchy
  // to update tooltips.
  virtual TooltipManager* GetTooltipManager() const = 0;

  // Sets or releases event capturing for this native widget.
  virtual void SetCapture() = 0;
  virtual void ReleaseCapture() = 0;

  // Returns true if this native widget is capturing events.
  virtual bool HasCapture() const = 0;

  // Returns the InputMethod for this native widget.
  // Note that all widgets in a widget hierarchy share the same input method.
  // TODO(suzhe): rename to GetInputMethod() when NativeWidget implementation
  // class doesn't inherit Widget anymore.
  virtual InputMethod* CreateInputMethod() = 0;

  // Returns the InputMethodDelegate for this native widget.
  virtual InputMethodDelegate* GetInputMethodDelegate() = 0;

  // Returns the ui::InputMethod for this native widget.
  // TODO(yukishiino): Rename this method to GetInputMethod once we remove
  // views::InputMethod.
  virtual ui::InputMethod* GetHostInputMethod() = 0;

  // Centers the window and sizes it to the specified size.
  virtual void CenterWindow(const gfx::Size& size) = 0;

  // Retrieves the window's current restored bounds and "show" state, for
  // persisting.
  virtual void GetWindowPlacement(
      gfx::Rect* bounds,
      ui::WindowShowState* show_state) const = 0;

  // Sets the NativeWindow title. Returns true if the title changed.
  virtual bool SetWindowTitle(const base::string16& title) = 0;

  // Sets the Window icons. |window_icon| is a 16x16 icon suitable for use in
  // a title bar. |app_icon| is a larger size for use in the host environment
  // app switching UI.
  virtual void SetWindowIcons(const gfx::ImageSkia& window_icon,
                              const gfx::ImageSkia& app_icon) = 0;

  // Initializes the modal type of the window to |modal_type|. Called from
  // NativeWidgetDelegate::OnNativeWidgetCreated() before the widget is
  // initially parented.
  virtual void InitModalType(ui::ModalType modal_type) = 0;

  // See method documentation in Widget.
  virtual gfx::Rect GetWindowBoundsInScreen() const = 0;
  virtual gfx::Rect GetClientAreaBoundsInScreen() const = 0;
  virtual gfx::Rect GetRestoredBounds() const = 0;
  virtual void SetBounds(const gfx::Rect& bounds) = 0;
  virtual void SetSize(const gfx::Size& size) = 0;
  virtual void StackAbove(gfx::NativeView native_view) = 0;
  virtual void StackAtTop() = 0;
  virtual void StackBelow(gfx::NativeView native_view) = 0;
  virtual void SetShape(gfx::NativeRegion shape) = 0;
  virtual void Close() = 0;
  virtual void CloseNow() = 0;
  virtual void Show() = 0;
  virtual void Hide() = 0;
  // Invoked if the initial show should maximize the window. |restored_bounds|
  // is the bounds of the window when not maximized.
  virtual void ShowMaximizedWithBounds(const gfx::Rect& restored_bounds) = 0;
  virtual void ShowWithWindowState(ui::WindowShowState show_state) = 0;
  virtual bool IsVisible() const = 0;
  virtual void Activate() = 0;
  virtual void Deactivate() = 0;
  virtual bool IsActive() const = 0;
  virtual void SetAlwaysOnTop(bool always_on_top) = 0;
  virtual bool IsAlwaysOnTop() const = 0;
  virtual void SetVisibleOnAllWorkspaces(bool always_visible) = 0;
  virtual void Maximize() = 0;
  virtual void Minimize() = 0;
  virtual bool IsMaximized() const = 0;
  virtual bool IsMinimized() const = 0;
  virtual void Restore() = 0;
  virtual void SetFullscreen(bool fullscreen) = 0;
  virtual bool IsFullscreen() const = 0;
  virtual void SetOpacity(unsigned char opacity) = 0;
  virtual void SetUseDragFrame(bool use_drag_frame) = 0;
  virtual void FlashFrame(bool flash) = 0;
  virtual void RunShellDrag(View* view,
                            const ui::OSExchangeData& data,
                            const gfx::Point& location,
                            int operation,
                            ui::DragDropTypes::DragEventSource source) = 0;
  virtual void SchedulePaintInRect(const gfx::Rect& rect) = 0;
  virtual void SetCursor(gfx::NativeCursor cursor) = 0;
  virtual bool IsMouseEventsEnabled() const = 0;
  virtual void ClearNativeFocus() = 0;
  virtual gfx::Rect GetWorkAreaBoundsInScreen() const = 0;
  virtual Widget::MoveLoopResult RunMoveLoop(
      const gfx::Vector2d& drag_offset,
      Widget::MoveLoopSource source,
      Widget::MoveLoopEscapeBehavior escape_behavior) = 0;
  virtual void EndMoveLoop() = 0;
  virtual void SetVisibilityChangedAnimationsEnabled(bool value) = 0;
  virtual ui::NativeTheme* GetNativeTheme() const = 0;
  virtual void OnRootViewLayout() const = 0;
  virtual bool IsTranslucentWindowOpacitySupported() const = 0;

  // Repost an unhandled event to the native widget for default OS processing.
  virtual void RepostNativeEvent(gfx::NativeEvent native_event) = 0;

  // Overridden from NativeWidget:
  virtual internal::NativeWidgetPrivate* AsNativeWidgetPrivate() OVERRIDE;
};

}  // namespace internal
}  // namespace views

#endif  // UI_VIEWS_WIDGET_NATIVE_WIDGET_PRIVATE_H_
