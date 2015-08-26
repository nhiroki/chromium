// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_MAC_H_
#define CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_MAC_H_

#import <Cocoa/Cocoa.h>
#include <IOSurface/IOSurface.h>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/mac/scoped_nsobject.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "content/browser/compositor/browser_compositor_view_mac.h"
#include "content/browser/compositor/delegated_frame_host.h"
#include "content/browser/renderer_host/input/mouse_wheel_rails_filter_mac.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/common/content_export.h"
#include "content/common/cursors/webcursor.h"
#include "content/common/edit_command.h"
#import "content/public/browser/render_widget_host_view_mac_base.h"
#include "ipc/ipc_sender.h"
#include "third_party/WebKit/public/web/WebCompositionUnderline.h"
#include "ui/accelerated_widget_mac/accelerated_widget_mac.h"
#include "ui/accelerated_widget_mac/display_link_mac.h"
#include "ui/accelerated_widget_mac/io_surface_layer.h"
#include "ui/base/cocoa/remote_layer_api.h"
#import "ui/base/cocoa/tool_tip_base_view.h"
#include "ui/gfx/display_observer.h"

struct ViewHostMsg_TextInputState_Params;

namespace content {
class RenderWidgetHostImpl;
class RenderWidgetHostViewMac;
class RenderWidgetHostViewMacEditCommandHelper;
class WebContents;
}

namespace ui {
class Compositor;
class Layer;
}

@class FullscreenWindowManager;
@protocol RenderWidgetHostViewMacDelegate;

@protocol RenderWidgetHostViewMacOwner
- (content::RenderWidgetHostViewMac*)renderWidgetHostViewMac;
@end

// This is the view that lives in the Cocoa view hierarchy. In Windows-land,
// RenderWidgetHostViewWin is both the view and the delegate. We split the roles
// but that means that the view needs to own the delegate and will dispose of it
// when it's removed from the view system.
@interface RenderWidgetHostViewCocoa
    : ToolTipBaseView<RenderWidgetHostViewMacBase,
                      RenderWidgetHostViewMacOwner,
                      NSTextInputClient> {
 @private
  scoped_ptr<content::RenderWidgetHostViewMac> renderWidgetHostView_;
  // This ivar is the cocoa delegate of the NSResponder.
  base::scoped_nsobject<NSObject<RenderWidgetHostViewMacDelegate>>
      responderDelegate_;
  BOOL canBeKeyView_;
  BOOL closeOnDeactivate_;
  BOOL opaque_;
  scoped_ptr<content::RenderWidgetHostViewMacEditCommandHelper>
      editCommand_helper_;

  // Is YES if there was a mouse-down as yet unbalanced with a mouse-up.
  BOOL hasOpenMouseDown_;

  NSWindow* lastWindow_;  // weak

  // The cursor for the page. This is passed up from the renderer.
  base::scoped_nsobject<NSCursor> currentCursor_;

  // Variables used by our implementaion of the NSTextInput protocol.
  // An input method of Mac calls the methods of this protocol not only to
  // notify an application of its status, but also to retrieve the status of
  // the application. That is, an application cannot control an input method
  // directly.
  // This object keeps the status of a composition of the renderer and returns
  // it when an input method asks for it.
  // We need to implement Objective-C methods for the NSTextInput protocol. On
  // the other hand, we need to implement a C++ method for an IPC-message
  // handler which receives input-method events from the renderer.

  // Represents the input-method attributes supported by this object.
  base::scoped_nsobject<NSArray> validAttributesForMarkedText_;

  // Indicates if we are currently handling a key down event.
  BOOL handlingKeyDown_;

  // Indicates if there is any marked text.
  BOOL hasMarkedText_;

  // Indicates if unmarkText is called or not when handling a keyboard
  // event.
  BOOL unmarkTextCalled_;

  // The range of current marked text inside the whole content of the DOM node
  // being edited.
  // TODO(suzhe): This is currently a fake value, as we do not support accessing
  // the whole content yet.
  NSRange markedRange_;

  // The selected range, cached from a message sent by the renderer.
  NSRange selectedRange_;

  // Text to be inserted which was generated by handling a key down event.
  base::string16 textToBeInserted_;

  // Marked text which was generated by handling a key down event.
  base::string16 markedText_;

  // Selected range of |markedText_|.
  NSRange markedTextSelectedRange_;

  // Underline information of the |markedText_|.
  std::vector<blink::WebCompositionUnderline> underlines_;

  // Indicates if doCommandBySelector method receives any edit command when
  // handling a key down event.
  BOOL hasEditCommands_;

  // Contains edit commands received by the -doCommandBySelector: method when
  // handling a key down event, not including inserting commands, eg. insertTab,
  // etc.
  content::EditCommands editCommands_;

  // The plugin that currently has focus (-1 if no plugin has focus).
  int focusedPluginIdentifier_;

  // Whether or not plugin IME is currently enabled active.
  BOOL pluginImeActive_;

  // Whether the previous mouse event was ignored due to hitTest check.
  BOOL mouseEventWasIgnored_;

  // Event monitor for scroll wheel end event.
  id endWheelMonitor_;

  // When a gesture starts, the system does not inform the view of which type
  // of gesture is happening (magnify, rotate, etc), rather, it just informs
  // the view that some as-yet-undefined gesture is starting. Capture the
  // information about the gesture's beginning event here. It will be used to
  // create a specific gesture begin event later.
  scoped_ptr<blink::WebGestureEvent> gestureBeginEvent_;

  // To avoid accidental pinches, require that a certain zoom threshold be
  // reached before forwarding it to the browser. Use |pinchUnusedAmount_| to
  // hold this value. If the user reaches this value, don't re-require the
  // threshold be reached until the page has been zoomed back to page scale of
  // one.
  bool pinchHasReachedZoomThreshold_;
  float pinchUnusedAmount_;
  NSTimeInterval pinchLastGestureTimestamp_;

  // This is set if a GesturePinchBegin event has been sent in the lifetime of
  // |gestureBeginEvent_|. If set, a GesturePinchEnd will be sent when the
  // gesture ends.
  BOOL gestureBeginPinchSent_;

  // If true then escape key down events are suppressed until the first escape
  // key up event. (The up event is suppressed as well). This is used by the
  // flash fullscreen code to avoid sending a key up event without a matching
  // key down event.
  BOOL suppressNextEscapeKeyUp_;

  // The set of key codes from key down events that we haven't seen the matching
  // key up events yet.
  // Used for filtering out non-matching NSKeyUp events.
  std::set<unsigned short> keyDownCodes_;

  // The filter used to guide touch events towards a horizontal or vertical
  // orientation.
  content::MouseWheelRailsFilterMac mouseWheelFilter_;
}

@property(nonatomic, readonly) NSRange selectedRange;
@property(nonatomic, readonly) BOOL suppressNextEscapeKeyUp;

- (void)setCanBeKeyView:(BOOL)can;
- (void)setCloseOnDeactivate:(BOOL)b;
- (void)setOpaque:(BOOL)opaque;
// True for always-on-top special windows (e.g. Balloons and Panels).
- (BOOL)acceptsMouseEventsWhenInactive;
// Cancel ongoing composition (abandon the marked text).
- (void)cancelComposition;
// Confirm ongoing composition.
- (void)confirmComposition;
// Enables or disables plugin IME.
- (void)setPluginImeActive:(BOOL)active;
// Updates the current plugin focus state.
- (void)pluginFocusChanged:(BOOL)focused forPlugin:(int)pluginId;
// Evaluates the event in the context of plugin IME, if plugin IME is enabled.
// Returns YES if the event was handled.
- (BOOL)postProcessEventForPluginIme:(NSEvent*)event;
- (void)updateCursor:(NSCursor*)cursor;
- (NSRect)firstViewRectForCharacterRange:(NSRange)theRange
                             actualRange:(NSRangePointer)actualRange;
@end

namespace content {

///////////////////////////////////////////////////////////////////////////////
// RenderWidgetHostViewMac
//
//  An object representing the "View" of a rendered web page. This object is
//  responsible for displaying the content of the web page, and integrating with
//  the Cocoa view system. It is the implementation of the RenderWidgetHostView
//  that the cross-platform RenderWidgetHost object uses
//  to display the data.
//
//  Comment excerpted from render_widget_host.h:
//
//    "The lifetime of the RenderWidgetHost* is tied to the render process.
//     If the render process dies, the RenderWidgetHost* goes away and all
//     references to it must become NULL."
//
// RenderWidgetHostView class hierarchy described in render_widget_host_view.h.
class CONTENT_EXPORT RenderWidgetHostViewMac
    : public RenderWidgetHostViewBase,
      public DelegatedFrameHostClient,
      public ui::AcceleratedWidgetMacNSView,
      public IPC::Sender,
      public gfx::DisplayObserver {
 public:
  // The view will associate itself with the given widget. The native view must
  // be hooked up immediately to the view hierarchy, or else when it is
  // deleted it will delete this out from under the caller.
  //
  // When |is_guest_view_hack| is true, this view isn't really the view for
  // the |widget|, a RenderWidgetHostViewGuest is.
  // TODO(lazyboy): Remove |is_guest_view_hack| once BrowserPlugin has migrated
  // to use RWHVChildFrame (http://crbug.com/330264).
  RenderWidgetHostViewMac(RenderWidgetHost* widget, bool is_guest_view_hack);
  ~RenderWidgetHostViewMac() override;

  RenderWidgetHostViewCocoa* cocoa_view() const { return cocoa_view_; }

  // |delegate| is used to separate out the logic from the NSResponder delegate.
  // |delegate| is retained by this class.
  // |delegate| should be set at most once.
  CONTENT_EXPORT void SetDelegate(
    NSObject<RenderWidgetHostViewMacDelegate>* delegate);
  void SetAllowPauseForResizeOrRepaint(bool allow);

  // RenderWidgetHostView implementation.
  bool OnMessageReceived(const IPC::Message& msg) override;
  void InitAsChild(gfx::NativeView parent_view) override;
  RenderWidgetHost* GetRenderWidgetHost() const override;
  void SetSize(const gfx::Size& size) override;
  void SetBounds(const gfx::Rect& rect) override;
  gfx::Vector2dF GetLastScrollOffset() const override;
  gfx::NativeView GetNativeView() const override;
  gfx::NativeViewId GetNativeViewId() const override;
  gfx::NativeViewAccessible GetNativeViewAccessible() override;
  bool HasFocus() const override;
  bool IsSurfaceAvailableForCopy() const override;
  void Show() override;
  void Hide() override;
  bool IsShowing() override;
  void WasUnOccluded() override;
  void WasOccluded() override;
  gfx::Rect GetViewBounds() const override;
  void SetShowingContextMenu(bool showing) override;
  void SetActive(bool active) override;
  void SetWindowVisibility(bool visible) override;
  void WindowFrameChanged() override;
  void ShowDefinitionForSelection() override;
  bool SupportsSpeech() const override;
  void SpeakSelection() override;
  bool IsSpeaking() const override;
  void StopSpeaking() override;
  void SetBackgroundColor(SkColor color) override;

  // Implementation of RenderWidgetHostViewBase.
  void InitAsPopup(RenderWidgetHostView* parent_host_view,
                   const gfx::Rect& pos) override;
  void InitAsFullscreen(RenderWidgetHostView* reference_host_view) override;
  void MovePluginWindows(const std::vector<WebPluginGeometry>& moves) override;
  void Focus() override;
  void UpdateCursor(const WebCursor& cursor) override;
  void SetIsLoading(bool is_loading) override;
  void TextInputStateChanged(
      const ViewHostMsg_TextInputState_Params& params) override;
  void ImeCancelComposition() override;
  void ImeCompositionRangeChanged(
      const gfx::Range& range,
      const std::vector<gfx::Rect>& character_bounds) override;
  void RenderProcessGone(base::TerminationStatus status,
                         int error_code) override;
  void RenderWidgetHostGone() override;
  void Destroy() override;
  void SetTooltipText(const base::string16& tooltip_text) override;
  void SelectionChanged(const base::string16& text,
                        size_t offset,
                        const gfx::Range& range) override;
  void SelectionBoundsChanged(
      const ViewHostMsg_SelectionBounds_Params& params) override;
  void CopyFromCompositingSurface(const gfx::Rect& src_subrect,
                                  const gfx::Size& dst_size,
                                  ReadbackRequestCallback& callback,
                                  SkColorType preferred_color_type) override;
  void CopyFromCompositingSurfaceToVideoFrame(
      const gfx::Rect& src_subrect,
      const scoped_refptr<media::VideoFrame>& target,
      const base::Callback<void(bool)>& callback) override;
  bool CanCopyToVideoFrame() const override;
  bool CanSubscribeFrame() const override;
  void BeginFrameSubscription(
      scoped_ptr<RenderWidgetHostViewFrameSubscriber> subscriber) override;
  void EndFrameSubscription() override;
  void OnSwapCompositorFrame(uint32 output_surface_id,
                             scoped_ptr<cc::CompositorFrame> frame) override;
  BrowserAccessibilityManager* CreateBrowserAccessibilityManager(
      BrowserAccessibilityDelegate* delegate) override;
  gfx::Point AccessibilityOriginInScreen(const gfx::Rect& bounds) override;
  bool PostProcessEventForPluginIme(
      const NativeWebKeyboardEvent& event) override;

  bool HasAcceleratedSurface(const gfx::Size& desired_size) override;
  void GetScreenInfo(blink::WebScreenInfo* results) override;
  bool GetScreenColorProfile(std::vector<char>* color_profile) override;
  gfx::Rect GetBoundsInRootWindow() override;
  gfx::GLSurfaceHandle GetCompositingSurface() override;

  bool LockMouse() override;
  void UnlockMouse() override;
  void WheelEventAck(const blink::WebMouseWheelEvent& event,
                     InputEventAckState ack_result) override;

  uint32_t GetSurfaceIdNamespace() override;

  // IPC::Sender implementation.
  bool Send(IPC::Message* message) override;

  // gfx::DisplayObserver implementation.
  void OnDisplayAdded(const gfx::Display& new_display) override;
  void OnDisplayRemoved(const gfx::Display& old_display) override;
  void OnDisplayMetricsChanged(const gfx::Display& display,
                               uint32_t metrics) override;

  // Forwards the mouse event to the renderer.
  void ForwardMouseEvent(const blink::WebMouseEvent& event);

  void KillSelf();

  void SetTextInputActive(bool active);

  // Sends completed plugin IME notification and text back to the renderer.
  void PluginImeCompositionCompleted(const base::string16& text, int plugin_id);

  const std::string& selected_text() const { return selected_text_; }

  // Returns true and stores first rectangle for character range if the
  // requested |range| is already cached, otherwise returns false.
  // Exposed for testing.
  CONTENT_EXPORT bool GetCachedFirstRectForCharacterRange(
      NSRange range, NSRect* rect, NSRange* actual_range);

  // Returns true if there is line break in |range| and stores line breaking
  // point to |line_breaking_point|. The |line_break_point| is valid only if
  // this function returns true.
  bool GetLineBreakIndex(const std::vector<gfx::Rect>& bounds,
                         const gfx::Range& range,
                         size_t* line_break_point);

  // Returns composition character boundary rectangle. The |range| is
  // composition based range. Also stores |actual_range| which is corresponding
  // to actually used range for returned rectangle.
  gfx::Rect GetFirstRectForCompositionRange(const gfx::Range& range,
                                            gfx::Range* actual_range);

  // Converts from given whole character range to composition oriented range. If
  // the conversion failed, return gfx::Range::InvalidRange.
  gfx::Range ConvertCharacterRangeToCompositionRange(
      const gfx::Range& request_range);

  WebContents* GetWebContents();

  // These member variables should be private, but the associated ObjC class
  // needs access to them and can't be made a friend.

  // The associated Model.  Can be NULL if Destroy() is called when
  // someone (other than superview) has retained |cocoa_view_|.
  RenderWidgetHostImpl* render_widget_host_;

  // Current text input type.
  ui::TextInputType text_input_type_;
  bool can_compose_inline_;

  // The background CoreAnimation layer which is hosted by |cocoa_view_|.
  base::scoped_nsobject<CALayer> background_layer_;

  // The state of |delegated_frame_host_| and |browser_compositor_| to
  // manage being visible, hidden, or occluded.
  enum BrowserCompositorViewState {
    // Effects:
    // - |browser_compositor_| exists and |delegated_frame_host_| is
    //    visible.
    // Happens when:
    // - |render_widet_host_| is in the visible state (this includes when
    //   the tab isn't visible, but tab capture is enabled).
    BrowserCompositorActive,
    // Effects:
    // - |browser_compositor_| exists, but |delegated_frame_host_| has
    //   been hidden.
    // Happens when:
    // - The |render_widget_host_| is hidden, but |cocoa_view_| is still in the
    //   NSWindow hierarchy.
    // - This happens when |cocoa_view_| is hidden (minimized, on another
    //   occluded by other windows, etc). The |browser_compositor_| and
    //   its CALayers are kept around so that we will have content to show when
    //   we are un-occluded.
    BrowserCompositorSuspended,
    // Effects:
    // - |browser_compositor_| has been destroyed and
    //   |delegated_frame_host_| has been hidden.
    // Happens when:
    // - The |render_widget_host_| is hidden or dead, and |cocoa_view_| is not
    //   attached to a NSWindow.
    // - This happens for backgrounded tabs.
    BrowserCompositorDestroyed,
  };
  BrowserCompositorViewState browser_compositor_state_;

  // Delegated frame management and compositor.
  scoped_ptr<DelegatedFrameHost> delegated_frame_host_;
  scoped_ptr<ui::Layer> root_layer_;

  // Container for ui::Compositor the CALayer tree drawn by it.
  scoped_ptr<BrowserCompositorMac> browser_compositor_;

  // Placeholder that is allocated while browser_compositor_ is NULL,
  // indicating that a BrowserCompositorViewMac may be allocated. This is to
  // help in recycling the internals of BrowserCompositorViewMac.
  scoped_ptr<BrowserCompositorMacPlaceholder>
      browser_compositor_placeholder_;

  // Set when the currently-displayed frame is the minimum scale. Used to
  // determine if pinch gestures need to be thresholded.
  bool page_at_minimum_scale_;

  NSWindow* pepper_fullscreen_window() const {
    return pepper_fullscreen_window_;
  }

  CONTENT_EXPORT void release_pepper_fullscreen_window_for_testing();

  RenderWidgetHostViewMac* fullscreen_parent_host_view() const {
    return fullscreen_parent_host_view_;
  }

  int window_number() const;

  // The scale factor for the screen that the view is currently on.
  float ViewScaleFactor() const;

  // Update properties, such as the scale factor for the backing store
  // and for any CALayers, and the screen color profile.
  void UpdateBackingStoreProperties();

  // Ensure that the display link is associated with the correct display.
  void UpdateDisplayLink();

  void PauseForPendingResizeOrRepaintsAndDraw();

  // DelegatedFrameHostClient implementation.
  ui::Layer* DelegatedFrameHostGetLayer() const override;
  bool DelegatedFrameHostIsVisible() const override;
  gfx::Size DelegatedFrameHostDesiredSizeInDIP() const override;
  bool DelegatedFrameCanCreateResizeLock() const override;
  scoped_ptr<ResizeLock> DelegatedFrameHostCreateResizeLock(
      bool defer_compositor_lock) override;
  void DelegatedFrameHostResizeLockWasReleased() override;
  void DelegatedFrameHostSendCompositorSwapAck(
      int output_surface_id,
      const cc::CompositorFrameAck& ack) override;
  void DelegatedFrameHostSendReclaimCompositorResources(
      int output_surface_id,
      const cc::CompositorFrameAck& ack) override;
  void DelegatedFrameHostOnLostCompositorResources() override;
  void DelegatedFrameHostUpdateVSyncParameters(
      const base::TimeTicks& timebase,
      const base::TimeDelta& interval) override;

  // AcceleratedWidgetMacNSView implementation.
  NSView* AcceleratedWidgetGetNSView() const override;
  bool AcceleratedWidgetShouldIgnoreBackpressure() const override;
  void AcceleratedWidgetGetVSyncParameters(
      base::TimeTicks* timebase, base::TimeDelta* interval) const override;
  void AcceleratedWidgetSwapCompleted(
      const std::vector<ui::LatencyInfo>& latency_info) override;
  void AcceleratedWidgetHitError() override;

  // Transition from being in the Suspended state to being in the Destroyed
  // state, if appropriate (see BrowserCompositorViewState for details).
  void DestroySuspendedBrowserCompositorViewIfNeeded();

 private:
  friend class RenderWidgetHostViewMacTest;

  // Returns whether this render view is a popup (autocomplete window).
  bool IsPopup() const;

  // Shuts down the render_widget_host_.  This is a separate function so we can
  // invoke it from the message loop.
  void ShutdownHost();

  // Tear down all components of the browser compositor in an order that will
  // ensure no dangling references.
  void ShutdownBrowserCompositor();

  // The state of the the browser compositor and delegated frame host. See
  // BrowserCompositorViewState for details.
  void EnsureBrowserCompositorView();
  void SuspendBrowserCompositorView();
  void DestroyBrowserCompositorView();

  // IPC message handlers.
  void OnPluginFocusChanged(bool focused, int plugin_id);
  void OnStartPluginIme();
  void OnGetRenderedTextCompleted(const std::string& text);

  // Send updated vsync parameters to the renderer.
  void SendVSyncParametersToRenderer();

  // Dispatches a TTS session.
  void SpeakText(const std::string& text);

  // The associated view. This is weak and is inserted into the view hierarchy
  // to own this RenderWidgetHostViewMac object. Set to nil at the start of the
  // destructor.
  RenderWidgetHostViewCocoa* cocoa_view_;

  // Indicates if the page is loading.
  bool is_loading_;

  // Whether it's allowed to pause waiting for a new frame.
  bool allow_pause_for_resize_or_repaint_;

  // The last scroll offset of the view.
  gfx::Vector2dF last_scroll_offset_;

  // The text to be shown in the tooltip, supplied by the renderer.
  base::string16 tooltip_text_;

  // True when this view acts as a platform view hack for a
  // RenderWidgetHostViewGuest.
  bool is_guest_view_hack_;

  // selected text on the renderer.
  std::string selected_text_;

  // The window used for popup widgets.
  base::scoped_nsobject<NSWindow> popup_window_;

  // The fullscreen window used for pepper flash.
  base::scoped_nsobject<NSWindow> pepper_fullscreen_window_;
  base::scoped_nsobject<FullscreenWindowManager> fullscreen_window_manager_;
  // Our parent host view, if this is fullscreen.  NULL otherwise.
  RenderWidgetHostViewMac* fullscreen_parent_host_view_;

  // Display link for getting vsync info.
  scoped_refptr<ui::DisplayLinkMac> display_link_;

  // The current VSync timebase and interval. This is zero until the first call
  // to SendVSyncParametersToRenderer(), and refreshed regularly thereafter.
  base::TimeTicks vsync_timebase_;
  base::TimeDelta vsync_interval_;

  // The current composition character range and its bounds.
  gfx::Range composition_range_;
  std::vector<gfx::Rect> composition_bounds_;

  // The current caret bounds.
  gfx::Rect caret_rect_;

  // Factory used to safely scope delayed calls to ShutdownHost().
  base::WeakPtrFactory<RenderWidgetHostViewMac> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(RenderWidgetHostViewMac);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_MAC_H_
