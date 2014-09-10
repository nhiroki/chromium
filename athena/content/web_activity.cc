// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "athena/content/web_activity.h"

#include "athena/activity/public/activity_factory.h"
#include "athena/activity/public/activity_manager.h"
#include "athena/content/public/dialogs.h"
#include "athena/input/public/accelerator_manager.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/common/content_switches.h"
#include "ui/aura/window.h"
#include "ui/compositor/closure_animation_observer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/views/controls/webview/unhandled_keyboard_event_handler.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/widget/widget.h"

namespace athena {
namespace {

class WebActivityController : public AcceleratorHandler {
 public:
  enum Command {
    CMD_BACK,
    CMD_FORWARD,
    CMD_RELOAD,
    CMD_RELOAD_IGNORE_CACHE,
    CMD_CLOSE,
    CMD_STOP,
  };

  explicit WebActivityController(views::WebView* web_view)
      : web_view_(web_view), reserved_accelerator_enabled_(true) {}
  virtual ~WebActivityController() {}

  // Installs accelerators for web activity.
  void InstallAccelerators() {
    accelerator_manager_ = AcceleratorManager::CreateForFocusManager(
                               web_view_->GetFocusManager()).Pass();
    const AcceleratorData accelerator_data[] = {
        {TRIGGER_ON_PRESS, ui::VKEY_R, ui::EF_CONTROL_DOWN, CMD_RELOAD,
         AF_NONE},
        {TRIGGER_ON_PRESS, ui::VKEY_BROWSER_REFRESH, ui::EF_NONE, CMD_RELOAD,
         AF_NONE},
        {TRIGGER_ON_PRESS, ui::VKEY_BROWSER_REFRESH, ui::EF_CONTROL_DOWN,
         CMD_RELOAD_IGNORE_CACHE, AF_NONE},
        {TRIGGER_ON_PRESS, ui::VKEY_BROWSER_FORWARD, ui::EF_NONE, CMD_FORWARD,
         AF_NONE},
        {TRIGGER_ON_PRESS, ui::VKEY_BROWSER_BACK, ui::EF_NONE, CMD_BACK,
         AF_NONE},
        {TRIGGER_ON_PRESS, ui::VKEY_W, ui::EF_CONTROL_DOWN, CMD_CLOSE, AF_NONE},
        {TRIGGER_ON_PRESS, ui::VKEY_ESCAPE, ui::EF_NONE, CMD_STOP, AF_NONE},
    };
    accelerator_manager_->RegisterAccelerators(
        accelerator_data, arraysize(accelerator_data), this);
  }

  // Methods that are called before and after key events are consumed by the web
  // contents.
  // See the documentation in WebContentsDelegate: for more details.
  bool PreHandleKeyboardEvent(content::WebContents* source,
                              const content::NativeWebKeyboardEvent& event,
                              bool* is_keyboard_shortcut) {
    ui::Accelerator accelerator(
        static_cast<ui::KeyboardCode>(event.windowsKeyCode),
        content::GetModifiersFromNativeWebKeyboardEvent(event));
    if (event.type == blink::WebInputEvent::KeyUp)
      accelerator.set_type(ui::ET_KEY_RELEASED);

    if (reserved_accelerator_enabled_ &&
        accelerator_manager_->IsRegistered(accelerator, AF_RESERVED)) {
      return web_view_->GetFocusManager()->ProcessAccelerator(accelerator);
    }
    *is_keyboard_shortcut =
        accelerator_manager_->IsRegistered(accelerator, AF_NONE);
    return false;
  }

  void HandleKeyboardEvent(content::WebContents* source,
                           const content::NativeWebKeyboardEvent& event) {
    unhandled_keyboard_event_handler_.HandleKeyboardEvent(
        event, web_view_->GetFocusManager());
  }

 private:
  // AcceleratorHandler:
  virtual bool IsCommandEnabled(int command_id) const OVERRIDE {
    switch (command_id) {
      case CMD_RELOAD:
      case CMD_RELOAD_IGNORE_CACHE:
        return true;
      case CMD_BACK:
        return web_view_->GetWebContents()->GetController().CanGoBack();
      case CMD_FORWARD:
        return web_view_->GetWebContents()->GetController().CanGoForward();
      case CMD_CLOSE:
        // TODO(oshima): check onbeforeunload handler.
        return true;
      case CMD_STOP:
        return web_view_->GetWebContents()->IsLoading();
    }
    return false;
  }

  virtual bool OnAcceleratorFired(int command_id,
                                  const ui::Accelerator& accelerator) OVERRIDE {
    switch (command_id) {
      case CMD_RELOAD:
        web_view_->GetWebContents()->GetController().Reload(false);
        return true;
      case CMD_RELOAD_IGNORE_CACHE:
        web_view_->GetWebContents()->GetController().ReloadIgnoringCache(false);
        return true;
      case CMD_BACK:
        web_view_->GetWebContents()->GetController().GoBack();
        return true;
      case CMD_FORWARD:
        web_view_->GetWebContents()->GetController().GoForward();
        return true;
      case CMD_CLOSE:
        web_view_->GetWidget()->Close();
        return true;
      case CMD_STOP:
        web_view_->GetWebContents()->Stop();
        return true;
    }
    return false;
  }

  views::WebView* web_view_;
  bool reserved_accelerator_enabled_;
  scoped_ptr<AcceleratorManager> accelerator_manager_;
  views::UnhandledKeyboardEventHandler unhandled_keyboard_event_handler_;

  DISALLOW_COPY_AND_ASSIGN(WebActivityController);
};

const SkColor kDefaultTitleColor = SkColorSetRGB(0xf2, 0xf2, 0xf2);
const SkColor kDefaultUnavailableColor = SkColorSetRGB(0xbb, 0x77, 0x77);

}  // namespace

// A web view for athena's web activity. Note that AthenaWebView will create its
// own content so that it can eject and reload it.
class AthenaWebView : public views::WebView {
 public:
  AthenaWebView(content::BrowserContext* context)
      : views::WebView(context), controller_(new WebActivityController(this)),
        fullscreen_(false) {
    SetEmbedFullscreenWidgetMode(true);
    // TODO(skuhne): Add content observer to detect renderer crash and set
    // content status to unloaded if that happens.
  }

  AthenaWebView(content::WebContents* web_contents)
      : views::WebView(web_contents->GetBrowserContext()),
        controller_(new WebActivityController(this)) {
    scoped_ptr<content::WebContents> old_contents(
        SwapWebContents(scoped_ptr<content::WebContents>(web_contents)));
  }

  virtual ~AthenaWebView() {}

  void InstallAccelerators() { controller_->InstallAccelerators(); }

  void EvictContent() {
    scoped_ptr<content::WebContents> old_contents(SwapWebContents(
        scoped_ptr<content::WebContents>(content::WebContents::Create(
            content::WebContents::CreateParams(browser_context())))));
    evicted_web_contents_.reset(
        content::WebContents::Create(content::WebContents::CreateParams(
            old_contents->GetBrowserContext())));
    evicted_web_contents_->GetController().CopyStateFrom(
        old_contents->GetController());
    // As soon as the new contents becomes visible, it should reload.
    // TODO(skuhne): This breaks script connections with other activities.
    // Even though this is the same technique as used by the TabStripModel,
    // we might want to address this cleaner since we are more likely to
    // run into this state. by unloading.
  }

  void ReloadContent() {
    CHECK(evicted_web_contents_.get());
    scoped_ptr<content::WebContents> replaced_contents(SwapWebContents(
        evicted_web_contents_.Pass()));
  }

  // Check if the content got evicted.
  const bool IsContentEvicted() { return !!evicted_web_contents_.get(); }

  // content::WebContentsDelegate:
  virtual content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) OVERRIDE {
    switch(params.disposition) {
      case CURRENT_TAB: {
        DCHECK(source == web_contents());
        content::NavigationController::LoadURLParams load_url_params(
            params.url);
        load_url_params.referrer = params.referrer;
        load_url_params.frame_tree_node_id = params.frame_tree_node_id;
        load_url_params.transition_type = params.transition;
        load_url_params.extra_headers = params.extra_headers;
        load_url_params.should_replace_current_entry =
            params.should_replace_current_entry;
        load_url_params.is_renderer_initiated = params.is_renderer_initiated;
        load_url_params.transferred_global_request_id =
            params.transferred_global_request_id;
        web_contents()->GetController().LoadURLWithParams(load_url_params);
        return web_contents();
      }
      case NEW_FOREGROUND_TAB:
      case NEW_BACKGROUND_TAB:
      case NEW_POPUP:
      case NEW_WINDOW: {
        ActivityManager::Get()->AddActivity(
            ActivityFactory::Get()->CreateWebActivity(
                browser_context(), base::string16(), params.url));
        break;
      }
      default:
        break;
    }
    // NULL is returned if the URL wasn't opened immediately.
    return NULL;
  }

  virtual bool CanOverscrollContent() const OVERRIDE {
    const std::string value = CommandLine::ForCurrentProcess()->
        GetSwitchValueASCII(switches::kOverscrollHistoryNavigation);
    return value != "0";
  }

  virtual void AddNewContents(content::WebContents* source,
                              content::WebContents* new_contents,
                              WindowOpenDisposition disposition,
                              const gfx::Rect& initial_pos,
                              bool user_gesture,
                              bool* was_blocked) OVERRIDE {
    ActivityManager::Get()->AddActivity(
        new WebActivity(new AthenaWebView(new_contents)));
  }

  virtual bool PreHandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event,
      bool* is_keyboard_shortcut) OVERRIDE {
    return controller_->PreHandleKeyboardEvent(
        source, event, is_keyboard_shortcut);
  }

  virtual void HandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event) OVERRIDE {
    controller_->HandleKeyboardEvent(source, event);
  }

  virtual void ToggleFullscreenModeForTab(content::WebContents* web_contents,
                                          bool enter_fullscreen) OVERRIDE {
    fullscreen_ = enter_fullscreen;
    GetWidget()->SetFullscreen(fullscreen_);
  }

  virtual bool IsFullscreenForTabOrPending(
      const content::WebContents* web_contents) const OVERRIDE {
    return fullscreen_;
  }

  virtual void LoadingStateChanged(content::WebContents* source,
                                   bool to_different_document) OVERRIDE {
    bool has_stopped = source == NULL || !source->IsLoading();
    LoadProgressChanged(source, has_stopped ? 1 : 0);
  }

  virtual void LoadProgressChanged(content::WebContents* source,
                                   double progress) OVERRIDE {
    if (!progress)
      return;

    if (!progress_bar_) {
      CreateProgressBar();
      source->GetNativeView()->layer()->Add(progress_bar_.get());
    }
    progress_bar_->SetBounds(gfx::Rect(
        0, 0, progress * progress_bar_->parent()->bounds().width(), 3));
    if (progress < 1)
      return;

    ui::ScopedLayerAnimationSettings settings(progress_bar_->GetAnimator());
    settings.SetTweenType(gfx::Tween::EASE_IN);
    ui::Layer* layer = progress_bar_.get();
    settings.AddObserver(new ui::ClosureAnimationObserver(
        base::Bind(&base::DeletePointer<ui::Layer>, progress_bar_.release())));
    layer->SetOpacity(0.f);
  }

  virtual content::ColorChooser* OpenColorChooser(
      content::WebContents* web_contents,
      SkColor color,
      const std::vector<content::ColorSuggestion>& suggestions) OVERRIDE {
    return athena::OpenColorChooser(web_contents, color, suggestions);
  }

  // Called when a file selection is to be done.
  virtual void RunFileChooser(
      content::WebContents* web_contents,
      const content::FileChooserParams& params) OVERRIDE {
    return athena::OpenFileChooser(web_contents, params);
  }

 private:
  void CreateProgressBar() {
    CHECK(!progress_bar_);
    progress_bar_.reset(new ui::Layer(ui::LAYER_SOLID_COLOR));
    progress_bar_->SetColor(SkColorSetRGB(0x17, 0x59, 0xcd));
  }

  scoped_ptr<WebActivityController> controller_;

  // If the activity got evicted, this is the web content which holds the known
  // state of the content before eviction.
  scoped_ptr<content::WebContents> evicted_web_contents_;

  scoped_ptr<ui::Layer> progress_bar_;

  // TODO(oshima): Find out if we should support window fullscreen.
  // It may still useful when a user is in split mode.
  bool fullscreen_;

  DISALLOW_COPY_AND_ASSIGN(AthenaWebView);
};

WebActivity::WebActivity(content::BrowserContext* browser_context,
                         const base::string16& title,
                         const GURL& url)
    : browser_context_(browser_context),
      title_(title),
      url_(url),
      web_view_(NULL),
      title_color_(kDefaultTitleColor),
      current_state_(ACTIVITY_UNLOADED) {
}

WebActivity::WebActivity(AthenaWebView* web_view)
    : browser_context_(web_view->browser_context()),
      url_(web_view->GetWebContents()->GetURL()),
      web_view_(web_view),
      current_state_(ACTIVITY_UNLOADED) {
  // Transition to state ACTIVITY_INVISIBLE to perform the same setup steps
  // as on new activities (namely adding a WebContentsObserver).
  SetCurrentState(ACTIVITY_INVISIBLE);
}

WebActivity::~WebActivity() {
  // It is not required to change the activity state to UNLOADED - unless we
  // would add state observers.
}

ActivityViewModel* WebActivity::GetActivityViewModel() {
  return this;
}

void WebActivity::SetCurrentState(Activity::ActivityState state) {
  DCHECK_NE(state, current_state_);
  switch (state) {
    case ACTIVITY_VISIBLE:
      if (!web_view_)
        break;
      MakeVisible();
      ReloadAndObserve();
      break;
    case ACTIVITY_INVISIBLE:
      if (!web_view_)
        break;
      if (current_state_ == ACTIVITY_VISIBLE)
        MakeInvisible();
      ReloadAndObserve();
      break;
    case ACTIVITY_BACKGROUND_LOW_PRIORITY:
      DCHECK(ACTIVITY_VISIBLE == current_state_ ||
             ACTIVITY_INVISIBLE == current_state_);
      // TODO(skuhne): Do this.
      break;
    case ACTIVITY_PERSISTENT:
      DCHECK_EQ(ACTIVITY_BACKGROUND_LOW_PRIORITY, current_state_);
      // TODO(skuhne): Do this. As soon as the new resource management is
      // agreed upon - or remove otherwise.
      break;
    case ACTIVITY_UNLOADED:
      DCHECK_NE(ACTIVITY_UNLOADED, current_state_);
      Observe(NULL);
      web_view_->EvictContent();
      break;
  }
  // Remember the last requested state.
  current_state_ = state;
}

Activity::ActivityState WebActivity::GetCurrentState() {
  // If the content is evicted, the state has to be UNLOADED.
  DCHECK(!web_view_ ||
         !web_view_->IsContentEvicted() ||
         current_state_ == ACTIVITY_UNLOADED);
  return current_state_;
}

bool WebActivity::IsVisible() {
  return web_view_ &&
         web_view_->visible() &&
         current_state_ != ACTIVITY_UNLOADED;
}

Activity::ActivityMediaState WebActivity::GetMediaState() {
  // TODO(skuhne): The function GetTabMediaStateForContents(WebContents),
  // and the AudioStreamMonitor needs to be moved from Chrome into contents to
  // make it more modular and so that we can use it from here.
  return Activity::ACTIVITY_MEDIA_STATE_NONE;
}

aura::Window* WebActivity::GetWindow() {
  return !web_view_ ? NULL : web_view_->GetWidget()->GetNativeWindow();
}

void WebActivity::Init() {
  DCHECK(web_view_);
  web_view_->InstallAccelerators();
}

SkColor WebActivity::GetRepresentativeColor() const {
  // TODO(sad): Compute the color from the favicon.
  return web_view_ ? title_color_ : kDefaultUnavailableColor;
}

base::string16 WebActivity::GetTitle() const {
  if (!title_.empty())
    return title_;
  // TODO(oshima): Use title set by the web contents.
  return web_view_ ? base::UTF8ToUTF16(
                         web_view_->GetWebContents()->GetVisibleURL().host())
                   : base::string16();
}

bool WebActivity::UsesFrame() const {
  return true;
}

views::View* WebActivity::GetContentsView() {
  if (!web_view_) {
    web_view_ = new AthenaWebView(browser_context_);
    web_view_->LoadInitialURL(url_);
    // Make sure the content gets properly shown.
    if (current_state_ == ACTIVITY_VISIBLE) {
      MakeVisible();
      ReloadAndObserve();
    } else if (current_state_ == ACTIVITY_INVISIBLE) {
      MakeInvisible();
      ReloadAndObserve();
    } else {
      // If not previously specified, we change the state now to invisible..
      SetCurrentState(ACTIVITY_INVISIBLE);
    }
  }
  return web_view_;
}

void WebActivity::CreateOverviewModeImage() {
  // TODO(skuhne): Create an overview.
}

gfx::ImageSkia WebActivity::GetOverviewModeImage() {
  return overview_mode_image_;
}

void WebActivity::PrepareContentsForOverview() {
  // Turn on fast resizing to avoid re-laying out the web contents when
  // entering / exiting overview mode.
  web_view_->SetFastResize(true);
}

void WebActivity::ResetContentsView() {
  web_view_->SetFastResize(false);
  web_view_->Layout();
}

void WebActivity::TitleWasSet(content::NavigationEntry* entry,
                              bool explicit_set) {
  ActivityManager::Get()->UpdateActivity(this);
}

void WebActivity::DidUpdateFaviconURL(
    const std::vector<content::FaviconURL>& candidates) {
  ActivityManager::Get()->UpdateActivity(this);
}

void WebActivity::DidChangeThemeColor(SkColor theme_color) {
  title_color_ = theme_color;
  ActivityManager::Get()->UpdateActivity(this);
}

void WebActivity::MakeVisible() {
  // TODO(skuhne): Once we know how to handle the Overview mode, this has to
  // be moved into an ActivityContentController which is used by all activities.
  // Make the content visible.
  // TODO(skuhne): If this can be combined with app_activity, move this into a
  // separate class.
  web_view_->SetVisible(true);
  web_view_->GetWebContents()->GetNativeView()->Show();
  // If we have a proxy image, we can delete it now since the contet goes live.
  // TODO(skuhne): Once we have figured out how to do overview mode that code
  // needs to go here.
  overview_mode_image_ = gfx::ImageSkia();
}

void WebActivity::MakeInvisible() {
  // TODO(skuhne): Once we know how to handle the Overview mode, this has to
  // be moved into an ActivityContentController which is used by all activities.
  // TODO(skuhne): If this can be combined with app_activity, move this into a
  // separate class.
  DCHECK(web_view_->visible());
  // Create our proxy image / layer.
  if (current_state_ == ACTIVITY_VISIBLE) {
    // Create a proxy image of the current visible content.
    // TODO(skuhne): Do this once we figure out how to do overview mode.
    overview_mode_image_ = gfx::ImageSkia();
  }
  // Now we can hide this.
  // Note: This might have to be done asynchronously after the read back took
  // place.
  web_view_->GetWebContents()->GetNativeView()->Hide();
  web_view_->SetVisible(false);
}

void WebActivity::ReloadAndObserve() {
  if (web_view_->IsContentEvicted()) {
    DCHECK_EQ(ACTIVITY_UNLOADED, current_state_);
    web_view_->ReloadContent();
  }
  Observe(web_view_->GetWebContents());
}

}  // namespace athena
