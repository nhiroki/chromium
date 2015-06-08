// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_IMPL_H_

#include <deque>
#include <list>
#include <map>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/process/kill.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "cc/resources/shared_bitmap.h"
#include "content/browser/renderer_host/event_with_latency_info.h"
#include "content/browser/renderer_host/input/input_ack_handler.h"
#include "content/browser/renderer_host/input/input_router_client.h"
#include "content/browser/renderer_host/input/render_widget_host_latency_tracker.h"
#include "content/browser/renderer_host/input/synthetic_gesture.h"
#include "content/browser/renderer_host/input/touch_emulator_client.h"
#include "content/common/input/input_event_ack_state.h"
#include "content/common/input/synthetic_gesture_packet.h"
#include "content/common/view_message_enums.h"
#include "content/public/browser/readback_types.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/common/page_zoom.h"
#include "ipc/ipc_listener.h"
#include "third_party/WebKit/public/platform/WebDisplayMode.h"
#include "ui/base/ime/text_input_mode.h"
#include "ui/base/ime/text_input_type.h"
#include "ui/events/gesture_detection/gesture_provider_config_helper.h"
#include "ui/events/latency_info.h"
#include "ui/gfx/native_widget_types.h"

struct AcceleratedSurfaceMsg_BufferPresented_Params;
struct ViewHostMsg_BeginSmoothScroll_Params;
struct ViewHostMsg_SelectionBounds_Params;
struct ViewHostMsg_TextInputState_Params;
struct ViewHostMsg_UpdateRect_Params;
struct ViewMsg_Resize_Params;

namespace base {
class TimeTicks;
}

namespace cc {
class CompositorFrame;
class CompositorFrameAck;
}

namespace gfx {
class Range;
}

namespace ui {
class KeyEvent;
}

namespace blink {
class WebInputEvent;
class WebMouseEvent;
struct WebCompositionUnderline;
struct WebScreenInfo;
}

#if defined(OS_ANDROID)
namespace blink {
class WebLayer;
}
#endif

namespace content {
class BrowserAccessibilityManager;
class InputRouter;
class MockRenderWidgetHost;
class RenderWidgetHostDelegate;
class RenderWidgetHostViewBase;
class SyntheticGestureController;
class TimeoutMonitor;
class TouchEmulator;
class WebCursor;
struct EditCommand;

// This implements the RenderWidgetHost interface that is exposed to
// embedders of content, and adds things only visible to content.
class CONTENT_EXPORT RenderWidgetHostImpl
    : virtual public RenderWidgetHost,
      public InputRouterClient,
      public InputAckHandler,
      public TouchEmulatorClient,
      public IPC::Listener {
 public:
  // routing_id can be MSG_ROUTING_NONE, in which case the next available
  // routing id is taken from the RenderProcessHost.
  // If this object outlives |delegate|, DetachDelegate() must be called when
  // |delegate| goes away.
  RenderWidgetHostImpl(RenderWidgetHostDelegate* delegate,
                       RenderProcessHost* process,
                       int routing_id,
                       bool hidden);
  ~RenderWidgetHostImpl() override;

  // Similar to RenderWidgetHost::FromID, but returning the Impl object.
  static RenderWidgetHostImpl* FromID(int32 process_id, int32 routing_id);

  // Returns all RenderWidgetHosts including swapped out ones for
  // internal use. The public interface
  // RendgerWidgetHost::GetRenderWidgetHosts only returns active ones.
  static scoped_ptr<RenderWidgetHostIterator> GetAllRenderWidgetHosts();

  // Use RenderWidgetHostImpl::From(rwh) to downcast a
  // RenderWidgetHost to a RenderWidgetHostImpl.  Internally, this
  // uses RenderWidgetHost::AsRenderWidgetHostImpl().
  static RenderWidgetHostImpl* From(RenderWidgetHost* rwh);

  void set_hung_renderer_delay(const base::TimeDelta& delay) {
    hung_renderer_delay_ = delay;
  }

  // RenderWidgetHost implementation.
  void UpdateTextDirection(blink::WebTextDirection direction) override;
  void NotifyTextDirection() override;
  void Focus() override;
  void Blur() override;
  void SetActive(bool active) override;
  void CopyFromBackingStore(const gfx::Rect& src_rect,
                            const gfx::Size& accelerated_dst_size,
                            ReadbackRequestCallback& callback,
                            const SkColorType preferred_color_type) override;
  bool CanCopyFromBackingStore() override;
#if defined(OS_ANDROID)
  void LockBackingStore() override;
  void UnlockBackingStore() override;
#endif
  void ForwardMouseEvent(const blink::WebMouseEvent& mouse_event) override;
  void ForwardWheelEvent(const blink::WebMouseWheelEvent& wheel_event) override;
  void ForwardKeyboardEvent(const NativeWebKeyboardEvent& key_event) override;
  RenderProcessHost* GetProcess() const override;
  int GetRoutingID() const override;
  RenderWidgetHostView* GetView() const override;
  bool IsLoading() const override;
  bool IsRenderView() const override;
  void ResizeRectChanged(const gfx::Rect& new_rect) override;
  void RestartHangMonitorTimeout() override;
  void SetIgnoreInputEvents(bool ignore_input_events) override;
  void WasResized() override;
  void AddKeyPressEventCallback(const KeyPressEventCallback& callback) override;
  void RemoveKeyPressEventCallback(
      const KeyPressEventCallback& callback) override;
  void AddMouseEventCallback(const MouseEventCallback& callback) override;
  void RemoveMouseEventCallback(const MouseEventCallback& callback) override;
  void GetWebScreenInfo(blink::WebScreenInfo* result) override;

  // Forces redraw in the renderer and when the update reaches the browser
  // grabs snapshot from the compositor. Returns PNG-encoded snapshot.
  void GetSnapshotFromBrowser(
      const base::Callback<void(const unsigned char*,size_t)> callback);

  const NativeWebKeyboardEvent* GetLastKeyboardEvent() const;

  // Notification that the screen info has changed.
  void NotifyScreenInfoChanged();

  // Sets the View of this RenderWidgetHost.
  void SetView(RenderWidgetHostViewBase* view);

  int surface_id() const { return surface_id_; }

  bool empty() const { return current_size_.IsEmpty(); }

  // Called when a renderer object already been created for this host, and we
  // just need to be attached to it. Used for window.open, <select> dropdown
  // menus, and other times when the renderer initiates creating an object.
  virtual void Init();

  // Initializes a RenderWidgetHost that is attached to a RenderFrameHost.
  void InitForFrame();

  // Signal whether this RenderWidgetHost is owned by a RenderFrameHost, in
  // which case it does not do self-deletion.
  void set_owned_by_render_frame_host(bool owned_by_rfh) {
    owned_by_render_frame_host_ = owned_by_rfh;
  }

  // Tells the renderer to die and then calls Destroy().
  virtual void Shutdown();

  // IPC::Listener
  bool OnMessageReceived(const IPC::Message& msg) override;

  // Sends a message to the corresponding object in the renderer.
  bool Send(IPC::Message* msg) override;

  // Indicates if the page has finished loading.
  virtual void SetIsLoading(bool is_loading);

  // Called to notify the RenderWidget that it has been hidden or restored from
  // having been hidden.
  virtual void WasHidden();
  virtual void WasShown(const ui::LatencyInfo& latency_info);

  // Returns true if the RenderWidget is hidden.
  bool is_hidden() const { return is_hidden_; }

  // Called to notify the RenderWidget that its associated native window
  // got/lost focused.
  virtual void GotFocus();
  virtual void LostCapture();

  // Indicates whether the RenderWidgetHost thinks it is focused.
  // This is different from RenderWidgetHostView::HasFocus() in the sense that
  // it reflects what the renderer process knows: it saves the state that is
  // sent/received.
  // RenderWidgetHostView::HasFocus() is checking whether the view is focused so
  // it is possible in some edge cases that a view was requested to be focused
  // but it failed, thus HasFocus() returns false.
  bool is_focused() const { return is_focused_; }

  // Called to notify the RenderWidget that it has lost the mouse lock.
  virtual void LostMouseLock();

  // Noifies the RenderWidget of the current mouse cursor visibility state.
  void SendCursorVisibilityState(bool is_visible);

  // Notifies the RenderWidgetHost that the View was destroyed.
  void ViewDestroyed();

#if defined(OS_MACOSX)
  // Pause for a moment to wait for pending repaint or resize messages sent to
  // the renderer to arrive. If pending resize messages are for an old window
  // size, then also pump through a new resize message if there is time.
  void PauseForPendingResizeOrRepaints();

  // Whether pausing may be useful.
  bool CanPauseForPendingResizeOrRepaints();

  // Wait for a surface matching the size of the widget's view, possibly
  // blocking until the renderer sends a new frame.
  void WaitForSurface();
#endif

  bool resize_ack_pending_for_testing() { return resize_ack_pending_; }

  // GPU accelerated version of GetBackingStore function. This will
  // trigger a re-composite to the view. It may fail if a resize is pending, or
  // if a composite has already been requested and not acked yet.
  bool ScheduleComposite();

  // Starts a hang monitor timeout. If there's already a hang monitor timeout
  // the new one will only fire if it has a shorter delay than the time
  // left on the existing timeouts.
  void StartHangMonitorTimeout(base::TimeDelta delay);

  // Stops all existing hang monitor timeouts and assumes the renderer is
  // responsive.
  void StopHangMonitorTimeout();

  // Forwards the given message to the renderer. These are called by the view
  // when it has received a message.
  void ForwardGestureEventWithLatencyInfo(
      const blink::WebGestureEvent& gesture_event,
      const ui::LatencyInfo& ui_latency);
  void ForwardTouchEventWithLatencyInfo(
      const blink::WebTouchEvent& touch_event,
      const ui::LatencyInfo& ui_latency);
  void ForwardMouseEventWithLatencyInfo(
      const blink::WebMouseEvent& mouse_event,
      const ui::LatencyInfo& ui_latency);
  void ForwardWheelEventWithLatencyInfo(
      const blink::WebMouseWheelEvent& wheel_event,
      const ui::LatencyInfo& ui_latency);

  // Enables/disables touch emulation using mouse event. See TouchEmulator.
  void SetTouchEventEmulationEnabled(
      bool enabled, ui::GestureProviderConfigType config_type);

  // TouchEmulatorClient implementation.
  void ForwardGestureEvent(
      const blink::WebGestureEvent& gesture_event) override;
  void ForwardEmulatedTouchEvent(
      const blink::WebTouchEvent& touch_event) override;
  void SetCursor(const WebCursor& cursor) override;
  void ShowContextMenuAtPoint(const gfx::Point& point) override;

  // Queues a synthetic gesture for testing purposes.  Invokes the on_complete
  // callback when the gesture is finished running.
  void QueueSyntheticGesture(
      scoped_ptr<SyntheticGesture> synthetic_gesture,
      const base::Callback<void(SyntheticGesture::Result)>& on_complete);

  void CancelUpdateTextDirection();

  // Notifies the renderer whether or not the input method attached to this
  // process is activated.
  // When the input method is activated, a renderer process sends IPC messages
  // to notify the status of its composition node. (This message is mainly used
  // for notifying the position of the input cursor so that the browser can
  // display input method windows under the cursor.)
  void SetInputMethodActive(bool activate);

  // Update the composition node of the renderer (or WebKit).
  // WebKit has a special node (a composition node) for input method to change
  // its text without affecting any other DOM nodes. When the input method
  // (attached to the browser) updates its text, the browser sends IPC messages
  // to update the composition node of the renderer.
  // (Read the comments of each function for its detail.)

  // Sets the text of the composition node.
  // This function can also update the cursor position and mark the specified
  // range in the composition node.
  // A browser should call this function:
  // * when it receives a WM_IME_COMPOSITION message with a GCS_COMPSTR flag
  //   (on Windows);
  // * when it receives a "preedit_changed" signal of GtkIMContext (on Linux);
  // * when markedText of NSTextInput is called (on Mac).
  void ImeSetComposition(
      const base::string16& text,
      const std::vector<blink::WebCompositionUnderline>& underlines,
      int selection_start,
      int selection_end);

  // Finishes an ongoing composition with the specified text.
  // A browser should call this function:
  // * when it receives a WM_IME_COMPOSITION message with a GCS_RESULTSTR flag
  //   (on Windows);
  // * when it receives a "commit" signal of GtkIMContext (on Linux);
  // * when insertText of NSTextInput is called (on Mac).
  void ImeConfirmComposition(const base::string16& text,
                             const gfx::Range& replacement_range,
                             bool keep_selection);

  // Cancels an ongoing composition.
  void ImeCancelComposition();

  // This is for derived classes to give us access to the resizer rect.
  // And to also expose it to the RenderWidgetHostView.
  virtual gfx::Rect GetRootWindowResizerRect() const;

  bool ignore_input_events() const {
    return ignore_input_events_;
  }

  bool input_method_active() const {
    return input_method_active_;
  }

  // Whether forwarded WebInputEvents should be ignored.  True if either
  // |ignore_input_events_| or |process_->IgnoreInputEvents()| is true.
  bool IgnoreInputEvents() const;

  bool has_touch_handler() const { return has_touch_handler_; }

  // Notification that the user has made some kind of input that could
  // perform an action. See OnUserGesture for more details.
  void StartUserGesture();

  // Set the RenderView background transparency.
  void SetBackgroundOpaque(bool opaque);

  // Notifies the renderer that the next key event is bound to one or more
  // pre-defined edit commands
  void SetEditCommandsForNextKeyEvent(
      const std::vector<EditCommand>& commands);

  // Executes the edit command on the RenderView.
  void ExecuteEditCommand(const std::string& command,
                          const std::string& value);

  // Tells the renderer to scroll the currently focused node into rect only if
  // the currently focused node is a Text node (textfield, text area or content
  // editable divs).
  void ScrollFocusedEditableNodeIntoRect(const gfx::Rect& rect);

  // Requests the renderer to move the caret selection towards the point.
  void MoveCaret(const gfx::Point& point);

  // Called when the reponse to a pending mouse lock request has arrived.
  // Returns true if |allowed| is true and the mouse has been successfully
  // locked.
  bool GotResponseToLockMouseRequest(bool allowed);

  // Tells the RenderWidget about the latest vsync parameters.
  virtual void UpdateVSyncParameters(base::TimeTicks timebase,
                                     base::TimeDelta interval);

  // Called by the view in response to OnSwapCompositorFrame.
  static void SendSwapCompositorFrameAck(
      int32 route_id,
      uint32 output_surface_id,
      int renderer_host_id,
      const cc::CompositorFrameAck& ack);

  // Called by the view to return resources to the compositor.
  static void SendReclaimCompositorResources(int32 route_id,
                                             uint32 output_surface_id,
                                             int renderer_host_id,
                                             const cc::CompositorFrameAck& ack);

  void set_allow_privileged_mouse_lock(bool allow) {
    allow_privileged_mouse_lock_ = allow;
  }

  // Resets state variables related to tracking pending size and painting.
  //
  // We need to reset these flags when we want to repaint the contents of
  // browser plugin in this RWH. Resetting these flags will ensure we ignore
  // any previous pending acks that are not relevant upon repaint.
  void ResetSizeAndRepaintPendingFlags();

  void DetachDelegate();

  // Update the renderer's cache of the screen rect of the view and window.
  void SendScreenRects();

  // Suppreses future char events until a keydown. See
  // suppress_next_char_events_.
  void SuppressNextCharEvents();

  // Called by the view in response to a flush request.
  void FlushInput();

  // Request a flush signal from the view.
  void SetNeedsFlush();

  // Indicates whether the renderer drives the RenderWidgetHosts's size or the
  // other way around.
  bool auto_resize_enabled() { return auto_resize_enabled_; }

  // The minimum size of this renderer when auto-resize is enabled.
  const gfx::Size& min_size_for_auto_resize() const {
    return min_size_for_auto_resize_;
  }

  // The maximum size of this renderer when auto-resize is enabled.
  const gfx::Size& max_size_for_auto_resize() const {
    return max_size_for_auto_resize_;
  }

  void FrameSwapped(const ui::LatencyInfo& latency_info);
  void DidReceiveRendererFrame();

  // Returns the ID that uniquely describes this component to the latency
  // subsystem.
  int64 GetLatencyComponentId() const;

  base::TimeDelta GetEstimatedBrowserCompositeTime() const;

  static void CompositorFrameDrawn(
      const std::vector<ui::LatencyInfo>& latency_info);

  // Don't check whether we expected a resize ack during layout tests.
  static void DisableResizeAckCheckForTesting();

  InputRouter* input_router() { return input_router_.get(); }

  // Get the BrowserAccessibilityManager for the root of the frame tree,
  BrowserAccessibilityManager* GetRootBrowserAccessibilityManager();

  // Get the BrowserAccessibilityManager for the root of the frame tree,
  // or create it if it doesn't already exist.
  BrowserAccessibilityManager* GetOrCreateRootBrowserAccessibilityManager();

  void RejectMouseLockOrUnlockIfNecessary();

#if defined(OS_WIN)
  gfx::NativeViewAccessible GetParentNativeViewAccessible();
#endif

  void set_renderer_initialized(bool renderer_initialized) {
    renderer_initialized_ = renderer_initialized;
  }

 protected:
  RenderWidgetHostImpl* AsRenderWidgetHostImpl() override;

  // Called when we receive a notification indicating that the renderer
  // process has gone. This will reset our state so that our state will be
  // consistent if a new renderer is created.
  void RendererExited(base::TerminationStatus status, int exit_code);

  // Retrieves an id the renderer can use to refer to its view.
  // This is used for various IPC messages, including plugins.
  gfx::NativeViewId GetNativeViewId() const;

  // Retrieves an id for the surface that the renderer can draw to
  // when accelerated compositing is enabled.
  gfx::GLSurfaceHandle GetCompositingSurface();

  // ---------------------------------------------------------------------------
  // The following methods are overridden by RenderViewHost to send upwards to
  // its delegate.

  // Called when a mousewheel event was not processed by the renderer.
  virtual void UnhandledWheelEvent(const blink::WebMouseWheelEvent& event) {}

  // Notification that the user has made some kind of input that could
  // perform an action. The gestures that count are 1) any mouse down
  // event and 2) enter or space key presses.
  virtual void OnUserGesture() {}

  // Callbacks for notification when the renderer becomes unresponsive to user
  // input events, and subsequently responsive again.
  virtual void NotifyRendererUnresponsive() {}
  virtual void NotifyRendererResponsive() {}

  // Called when auto-resize resulted in the renderer size changing.
  virtual void OnRenderAutoResized(const gfx::Size& new_size) {}

  // ---------------------------------------------------------------------------

  // RenderViewHost overrides this method to impose further restrictions on when
  // to allow mouse lock.
  // Once the request is approved or rejected, GotResponseToLockMouseRequest()
  // will be called.
  virtual void RequestToLockMouse(bool user_gesture,
                                  bool last_unlocked_by_target);

  bool IsMouseLocked() const;

  // RenderViewHost overrides this method to report whether tab-initiated
  // fullscreen was granted.
  virtual bool IsFullscreenGranted() const;

  virtual blink::WebDisplayMode GetDisplayMode() const;

  // Indicates if the render widget host should track the render widget's size
  // as opposed to visa versa.
  void SetAutoResize(bool enable,
                     const gfx::Size& min_size,
                     const gfx::Size& max_size);

  // Fills in the |resize_params| struct.
  // Returns |false| if the update is redundant, |true| otherwise.
  bool GetResizeParams(ViewMsg_Resize_Params* resize_params);

  // Sets the |resize_params| that were sent to the renderer bundled with the
  // request to create a new RenderWidget.
  void SetInitialRenderSizeParams(const ViewMsg_Resize_Params& resize_params);

  // Expose increment/decrement of the in-flight event count, so
  // RenderViewHostImpl can account for in-flight beforeunload/unload events.
  int increment_in_flight_event_count() { return ++in_flight_event_count_; }
  int decrement_in_flight_event_count() {
    DCHECK_GT(in_flight_event_count_, 0);
    return --in_flight_event_count_;
  }

  bool renderer_initialized() const { return renderer_initialized_; }

  // The View associated with the RenderViewHost. The lifetime of this object
  // is associated with the lifetime of the Render process. If the Renderer
  // crashes, its View is destroyed and this pointer becomes NULL, even though
  // render_view_host_ lives on to load another URL (creating a new View while
  // doing so).
  RenderWidgetHostViewBase* view_;

  // A weak pointer to the view. The above pointer should be weak, but changing
  // that to be weak causes crashes on Android.
  // TODO(ccameron): Fix this.
  // http://crbug.com/404828
  base::WeakPtr<RenderWidgetHostViewBase> view_weak_;

  // This value indicates how long to wait before we consider a renderer hung.
  base::TimeDelta hung_renderer_delay_;

 private:
  friend class MockRenderWidgetHost;

  // Tell this object to destroy itself.
  void Destroy();

  // Called by |hang_monitor_timeout_| on delayed response from the renderer.
  void RendererIsUnresponsive();

  // Called if we know the renderer is responsive. When we currently think the
  // renderer is unresponsive, this will clear that state and call
  // NotifyRendererResponsive.
  void RendererIsResponsive();

  // IPC message handlers
  void OnRenderViewReady();
  void OnRenderProcessGone(int status, int error_code);
  void OnClose();
  void OnUpdateScreenRectsAck();
  void OnRequestMove(const gfx::Rect& pos);
  void OnSetTooltipText(const base::string16& tooltip_text,
                        blink::WebTextDirection text_direction_hint);
  bool OnSwapCompositorFrame(const IPC::Message& message);
  void OnUpdateRect(const ViewHostMsg_UpdateRect_Params& params);
  void OnQueueSyntheticGesture(const SyntheticGesturePacket& gesture_packet);
  virtual void OnFocus();
  virtual void OnBlur();
  void OnSetCursor(const WebCursor& cursor);
  void OnTextInputTypeChanged(ui::TextInputType type,
                              ui::TextInputMode input_mode,
                              bool can_compose_inline,
                              int flags);

  void OnImeCompositionRangeChanged(
      const gfx::Range& range,
      const std::vector<gfx::Rect>& character_bounds);
  void OnImeCancelComposition();
  void OnLockMouse(bool user_gesture,
                   bool last_unlocked_by_target,
                   bool privileged);
  void OnUnlockMouse();
  void OnShowDisambiguationPopup(const gfx::Rect& rect_pixels,
                                 const gfx::Size& size,
                                 const cc::SharedBitmapId& id);
#if defined(OS_WIN)
  void OnWindowlessPluginDummyWindowCreated(
      gfx::NativeViewId dummy_activation_window);
  void OnWindowlessPluginDummyWindowDestroyed(
      gfx::NativeViewId dummy_activation_window);
#endif
  void OnSelectionChanged(const base::string16& text,
                          size_t offset,
                          const gfx::Range& range);
  void OnSelectionBoundsChanged(
      const ViewHostMsg_SelectionBounds_Params& params);
  void OnSnapshot(bool success, const SkBitmap& bitmap);

  // Called (either immediately or asynchronously) after we're done with our
  // BackingStore and can send an ACK to the renderer so it can paint onto it
  // again.
  void DidUpdateBackingStore(const ViewHostMsg_UpdateRect_Params& params,
                             const base::TimeTicks& paint_start);

  // Give key press listeners a chance to handle this key press. This allow
  // widgets that don't have focus to still handle key presses.
  bool KeyPressListenersHandleEvent(const NativeWebKeyboardEvent& event);

  // InputRouterClient
  InputEventAckState FilterInputEvent(
      const blink::WebInputEvent& event,
      const ui::LatencyInfo& latency_info) override;
  void IncrementInFlightEventCount() override;
  void DecrementInFlightEventCount() override;
  void OnHasTouchEventHandlers(bool has_handlers) override;
  void DidFlush() override;
  void DidOverscroll(const DidOverscrollParams& params) override;
  void DidStopFlinging() override;

  // InputAckHandler
  void OnKeyboardEventAck(const NativeWebKeyboardEvent& event,
                          InputEventAckState ack_result) override;
  void OnWheelEventAck(const MouseWheelEventWithLatencyInfo& event,
                       InputEventAckState ack_result) override;
  void OnTouchEventAck(const TouchEventWithLatencyInfo& event,
                       InputEventAckState ack_result) override;
  void OnGestureEventAck(const GestureEventWithLatencyInfo& event,
                         InputEventAckState ack_result) override;
  void OnUnexpectedEventAck(UnexpectedEventAckType type) override;

  void OnSyntheticGestureCompleted(SyntheticGesture::Result result);

  // Called when there is a new auto resize (using a post to avoid a stack
  // which may get in recursive loops).
  void DelayedAutoResized();

  void WindowSnapshotReachedScreen(int snapshot_id);

  void OnSnapshotDataReceived(int snapshot_id,
                              const unsigned char* png,
                              size_t size);

  void OnSnapshotDataReceivedAsync(
      int snapshot_id,
      scoped_refptr<base::RefCountedBytes> png_data);

  // true if a renderer has once been valid. We use this flag to display a sad
  // tab only when we lose our renderer and not if a paint occurs during
  // initialization.
  bool renderer_initialized_;

  // Our delegate, which wants to know mainly about keyboard events.
  // It will remain non-NULL until DetachDelegate() is called.
  RenderWidgetHostDelegate* delegate_;

  // Created during construction but initialized during Init*(). Therefore, it
  // is guaranteed never to be NULL, but its channel may be NULL if the
  // renderer crashed, so you must always check that.
  RenderProcessHost* process_;

  // The ID of the corresponding object in the Renderer Instance.
  int routing_id_;

  // The ID of the surface corresponding to this render widget.
  int surface_id_;

  // Indicates whether a page is loading or not.
  bool is_loading_;

  // Indicates whether a page is hidden or not. It has to stay in sync with the
  // most recent call to process_->WidgetRestored() / WidgetHidden().
  bool is_hidden_;

  // Set if we are waiting for a repaint ack for the view.
  bool repaint_ack_pending_;

  // True when waiting for RESIZE_ACK.
  bool resize_ack_pending_;

  // The current size of the RenderWidget.
  gfx::Size current_size_;

  // Resize information that was previously sent to the renderer.
  scoped_ptr<ViewMsg_Resize_Params> old_resize_params_;

  // The next auto resize to send.
  gfx::Size new_auto_size_;

  // True if the render widget host should track the render widget's size as
  // opposed to visa versa.
  bool auto_resize_enabled_;

  // The minimum size for the render widget if auto-resize is enabled.
  gfx::Size min_size_for_auto_resize_;

  // The maximum size for the render widget if auto-resize is enabled.
  gfx::Size max_size_for_auto_resize_;

  bool waiting_for_screen_rects_ack_;
  gfx::Rect last_view_screen_rect_;
  gfx::Rect last_window_screen_rect_;

  // Keyboard event listeners.
  std::vector<KeyPressEventCallback> key_press_event_callbacks_;

  // Mouse event callbacks.
  std::vector<MouseEventCallback> mouse_event_callbacks_;

  // If true, then we should repaint when restoring even if we have a
  // backingstore.  This flag is set to true if we receive a paint message
  // while is_hidden_ to true.  Even though we tell the render widget to hide
  // itself, a paint message could already be in flight at that point.
  bool needs_repainting_on_restore_;

  // This is true if the renderer is currently unresponsive.
  bool is_unresponsive_;

  // This value denotes the number of input events yet to be acknowledged
  // by the renderer.
  int in_flight_event_count_;

  // Flag to detect recursive calls to GetBackingStore().
  bool in_get_backing_store_;

  // Used for UMA histogram logging to measure the time for a repaint view
  // operation to finish.
  base::TimeTicks repaint_start_time_;

  // Set to true if we shouldn't send input events from the render widget.
  bool ignore_input_events_;

  // Indicates whether IME is active.
  bool input_method_active_;

  // Set when we update the text direction of the selected input element.
  bool text_direction_updated_;
  blink::WebTextDirection text_direction_;

  // Set when we cancel updating the text direction.
  // This flag also ignores succeeding update requests until we call
  // NotifyTextDirection().
  bool text_direction_canceled_;

  // Indicates if the next sequence of Char events should be suppressed or not.
  // System may translate a RawKeyDown event into zero or more Char events,
  // usually we send them to the renderer directly in sequence. However, If a
  // RawKeyDown event was not handled by the renderer but was handled by
  // our UnhandledKeyboardEvent() method, e.g. as an accelerator key, then we
  // shall not send the following sequence of Char events, which was generated
  // by this RawKeyDown event, to the renderer. Otherwise the renderer may
  // handle the Char events and cause unexpected behavior.
  // For example, pressing alt-2 may let the browser switch to the second tab,
  // but the Char event generated by alt-2 may also activate a HTML element
  // if its accesskey happens to be "2", then the user may get confused when
  // switching back to the original tab, because the content may already be
  // changed.
  bool suppress_next_char_events_;

  bool pending_mouse_lock_request_;
  bool allow_privileged_mouse_lock_;

  // Keeps track of whether the webpage has any touch event handler. If it does,
  // then touch events are sent to the renderer. Otherwise, the touch events are
  // not sent to the renderer.
  bool has_touch_handler_;

  scoped_ptr<SyntheticGestureController> synthetic_gesture_controller_;

  scoped_ptr<TouchEmulator> touch_emulator_;

  // Receives and handles all input events.
  scoped_ptr<InputRouter> input_router_;

  scoped_ptr<TimeoutMonitor> hang_monitor_timeout_;

#if defined(OS_WIN)
  std::list<HWND> dummy_windows_for_activation_;
#endif

  RenderWidgetHostLatencyTracker latency_tracker_;

  int next_browser_snapshot_id_;
  typedef std::map<int,
      base::Callback<void(const unsigned char*, size_t)> > PendingSnapshotMap;
  PendingSnapshotMap pending_browser_snapshots_;

  // Indicates whether a RenderFramehost has ownership, in which case this
  // object does not self destroy.
  bool owned_by_render_frame_host_;

  // Indicates whether this RenderWidgetHost thinks is focused. This is trying
  // to match what the renderer process knows. It is different from
  // RenderWidgetHostView::HasFocus in that in that the focus request may fail,
  // causing HasFocus to return false when is_focused_ is true.
  bool is_focused_;

  base::WeakPtrFactory<RenderWidgetHostImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(RenderWidgetHostImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_IMPL_H_
