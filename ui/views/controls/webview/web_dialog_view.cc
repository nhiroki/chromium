// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/webview/web_dialog_view.h"

#include <vector>

#include "base/utf_string_conversions.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/keycodes/keyboard_codes.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/root_view.h"
#include "ui/views/widget/widget.h"
#include "ui/web_dialogs/web_dialog_delegate.h"
#include "ui/web_dialogs/web_dialog_ui.h"

#if defined(USE_AURA)
#include "ui/base/events/event.h"
#include "ui/views/widget/native_widget_aura.h"
#endif

using content::NativeWebKeyboardEvent;
using content::WebContents;
using content::WebUIMessageHandler;
using ui::WebDialogDelegate;
using ui::WebDialogUI;
using ui::WebDialogWebContentsDelegate;

namespace views {

////////////////////////////////////////////////////////////////////////////////
// WebDialogView, public:

WebDialogView::WebDialogView(
    content::BrowserContext* context,
    WebDialogDelegate* delegate,
    WebContentsHandler* handler)
    : ClientView(NULL, NULL),
      WebDialogWebContentsDelegate(context, handler),
      initialized_(false),
      delegate_(delegate),
      web_view_(new views::WebView(context)),
      is_attempting_close_dialog_(false),
      before_unload_fired_(false),
      closed_via_webui_(false),
      close_contents_called_(false) {
  web_view_->set_allow_accelerators(true);
  AddChildView(web_view_);
  set_contents_view(web_view_);
  SetLayoutManager(new views::FillLayout);
  // Pressing the ESC key will close the dialog.
  AddAccelerator(ui::Accelerator(ui::VKEY_ESCAPE, ui::EF_NONE));
}

WebDialogView::~WebDialogView() {
}

content::WebContents* WebDialogView::web_contents() {
  return web_view_->web_contents();
}

////////////////////////////////////////////////////////////////////////////////
// WebDialogView, views::View implementation:

gfx::Size WebDialogView::GetPreferredSize() {
  gfx::Size out;
  if (delegate_)
    delegate_->GetMinimumDialogSize(&out);
  return out;
}

bool WebDialogView::AcceleratorPressed(const ui::Accelerator& accelerator) {
  // Pressing ESC closes the dialog.
  DCHECK_EQ(ui::VKEY_ESCAPE, accelerator.key_code());
  if (GetWidget())
    GetWidget()->Close();
  return true;
}

void WebDialogView::ViewHierarchyChanged(bool is_add,
                                         views::View* parent,
                                         views::View* child) {
  if (is_add && GetWidget())
    InitDialog();
}

bool WebDialogView::CanClose() {
  // If CloseContents() is called before CanClose(), which is called by
  // RenderViewHostImpl::ClosePageIgnoringUnloadEvents, it indicates
  // beforeunload event should not be fired during closing.
  if ((is_attempting_close_dialog_ && before_unload_fired_) ||
      close_contents_called_) {
    is_attempting_close_dialog_ = false;
    before_unload_fired_ = false;
    return true;
  }

  if (!is_attempting_close_dialog_) {
    // Fire beforeunload event when user attempts to close the dialog.
    is_attempting_close_dialog_ = true;
    web_view_->
        web_contents()->GetRenderViewHost()->FirePageBeforeUnload(false);
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// WebDialogView, views::WidgetDelegate implementation:

bool WebDialogView::CanResize() const {
  return true;
}

ui::ModalType WebDialogView::GetModalType() const {
  return GetDialogModalType();
}

string16 WebDialogView::GetWindowTitle() const {
  if (delegate_)
    return delegate_->GetDialogTitle();
  return string16();
}

std::string WebDialogView::GetWindowName() const {
  if (delegate_)
    return delegate_->GetDialogName();
  return std::string();
}

void WebDialogView::WindowClosing() {
  // If we still have a delegate that means we haven't notified it of the
  // dialog closing. This happens if the user clicks the Close button on the
  // dialog.
  if (delegate_)
    OnDialogClosed("");
}

views::View* WebDialogView::GetContentsView() {
  return this;
}

views::ClientView* WebDialogView::CreateClientView(views::Widget* widget) {
  return this;
}

views::View* WebDialogView::GetInitiallyFocusedView() {
  return web_view_;
}

bool WebDialogView::ShouldShowWindowTitle() const {
  return ShouldShowDialogTitle();
}

views::Widget* WebDialogView::GetWidget() {
  return View::GetWidget();
}

const views::Widget* WebDialogView::GetWidget() const {
  return View::GetWidget();
}

////////////////////////////////////////////////////////////////////////////////
// WebDialogDelegate implementation:

ui::ModalType WebDialogView::GetDialogModalType() const {
  if (delegate_)
    return delegate_->GetDialogModalType();
  return ui::MODAL_TYPE_NONE;
}

string16 WebDialogView::GetDialogTitle() const {
  return GetWindowTitle();
}

GURL WebDialogView::GetDialogContentURL() const {
  if (delegate_)
    return delegate_->GetDialogContentURL();
  return GURL();
}

void WebDialogView::GetWebUIMessageHandlers(
    std::vector<WebUIMessageHandler*>* handlers) const {
  if (delegate_)
    delegate_->GetWebUIMessageHandlers(handlers);
}

void WebDialogView::GetDialogSize(gfx::Size* size) const {
  if (delegate_)
    delegate_->GetDialogSize(size);
}

void WebDialogView::GetMinimumDialogSize(gfx::Size* size) const {
  if (delegate_)
    delegate_->GetMinimumDialogSize(size);
}

std::string WebDialogView::GetDialogArgs() const {
  if (delegate_)
    return delegate_->GetDialogArgs();
  return std::string();
}

void WebDialogView::OnDialogShown(content::WebUI* webui,
                                  content::RenderViewHost* render_view_host) {
  if (delegate_)
    delegate_->OnDialogShown(webui, render_view_host);
}

void WebDialogView::OnDialogClosed(const std::string& json_retval) {
  Detach();
  if (delegate_) {
    // Store the dialog content area size.
    delegate_->StoreDialogSize(GetContentsBounds().size());
  }

  if (GetWidget())
    GetWidget()->Close();

  if (delegate_) {
    delegate_->OnDialogClosed(json_retval);
    delegate_ = NULL;  // We will not communicate further with the delegate.
  }
}

void WebDialogView::OnDialogCloseFromWebUI(const std::string& json_retval) {
  closed_via_webui_ = true;
  dialog_close_retval_ = json_retval;
  if (GetWidget())
    GetWidget()->Close();
}

void WebDialogView::OnCloseContents(WebContents* source,
                                    bool* out_close_dialog) {
  if (delegate_)
    delegate_->OnCloseContents(source, out_close_dialog);
}

bool WebDialogView::ShouldShowDialogTitle() const {
  if (delegate_)
    return delegate_->ShouldShowDialogTitle();
  return true;
}

bool WebDialogView::HandleContextMenu(
    const content::ContextMenuParams& params) {
  if (delegate_)
    return delegate_->HandleContextMenu(params);
  return WebDialogWebContentsDelegate::HandleContextMenu(params);
}

////////////////////////////////////////////////////////////////////////////////
// content::WebContentsDelegate implementation:

void WebDialogView::MoveContents(WebContents* source, const gfx::Rect& pos) {
  // The contained web page wishes to resize itself. We let it do this because
  // if it's a dialog we know about, we trust it not to be mean to the user.
  GetWidget()->SetBounds(pos);
}

// A simplified version of BrowserView::HandleKeyboardEvent().
// We don't handle global keyboard shortcuts here, but that's fine since
// they're all browser-specific. (This may change in the future.)
void WebDialogView::HandleKeyboardEvent(content::WebContents* source,
                                        const NativeWebKeyboardEvent& event) {
#if defined(USE_AURA)
  if (!event.os_event)
    return;
  ui::KeyEvent aura_event(event.os_event->native_event(), false);
  views::NativeWidgetAura* aura_widget =
      static_cast<views::NativeWidgetAura*>(GetWidget()->native_widget());
  aura_widget->OnKeyEvent(&aura_event);
#elif defined(OS_WIN)
  // Any unhandled keyboard/character messages should be defproced.
  // This allows stuff like F10, etc to work correctly.
  DefWindowProc(event.os_event.hwnd, event.os_event.message,
                  event.os_event.wParam, event.os_event.lParam);
#endif
}

void WebDialogView::CloseContents(WebContents* source) {
  close_contents_called_ = true;
  bool close_dialog = false;
  OnCloseContents(source, &close_dialog);
  if (close_dialog)
    OnDialogClosed(closed_via_webui_ ? dialog_close_retval_ : std::string());
}

content::WebContents* WebDialogView::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  content::WebContents* new_contents = NULL;
  if (delegate_ &&
      delegate_->HandleOpenURLFromTab(source, params, &new_contents)) {
    return new_contents;
  }
  return WebDialogWebContentsDelegate::OpenURLFromTab(source, params);
}

void WebDialogView::AddNewContents(content::WebContents* source,
                                   content::WebContents* new_contents,
                                   WindowOpenDisposition disposition,
                                   const gfx::Rect& initial_pos,
                                   bool user_gesture,
                                   bool* was_blocked) {
  if (delegate_ && delegate_->HandleAddNewContents(
          source, new_contents, disposition, initial_pos, user_gesture)) {
    return;
  }
  WebDialogWebContentsDelegate::AddNewContents(
      source, new_contents, disposition, initial_pos, user_gesture,
      was_blocked);
}

void WebDialogView::LoadingStateChanged(content::WebContents* source) {
  if (delegate_)
    delegate_->OnLoadingStateChanged(source);
}

void WebDialogView::BeforeUnloadFired(content::WebContents* tab,
                                      bool proceed,
                                      bool* proceed_to_fire_unload) {
  before_unload_fired_ = true;
  *proceed_to_fire_unload = proceed;
}

////////////////////////////////////////////////////////////////////////////////
// WebDialogView, private:

void WebDialogView::InitDialog() {
  content::WebContents* web_contents = web_view_->GetWebContents();
  if (web_contents->GetDelegate() == this)
    return;

  web_contents->SetDelegate(this);

  // Set the delegate. This must be done before loading the page. See
  // the comment above WebDialogUI in its header file for why.
  WebDialogUI::SetDelegate(web_contents, this);

  web_view_->LoadInitialURL(GetDialogContentURL());
}

}  // namespace views
