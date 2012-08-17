// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/webui_login_display_host.h"

#include "ash/desktop_background/desktop_background_controller.h"
#include "ash/shell.h"
#include "ash/shell_window_ids.h"
#include "ash/wm/window_animations.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/time.h"
#include "base/values.h"
#include "chrome/browser/browser_shutdown.h"
#include "chrome/browser/chromeos/kiosk_mode/kiosk_mode_settings.h"
#include "chrome/browser/chromeos/login/oobe_display.h"
#include "chrome/browser/chromeos/login/webui_login_display.h"
#include "chrome/browser/chromeos/login/webui_login_view.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/ui/webui/chromeos/login/oobe_ui.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/views/widget/widget.h"

namespace chromeos {

namespace {

// URL which corresponds to the login WebUI.
const char kLoginURL[] = "chrome://oobe/login";
// URL which corresponds to the OOBE WebUI.
const char kOobeURL[] = "chrome://oobe";

// Duration of sign-in transition animation.
const int kLoginFadeoutTransitionDurationMs = 700;

// Number of times we try to reload OOBE/login WebUI if it crashes.
const int kCrashCountLimit = 5;

// When wallpaper animation is not disabled (no flag --disable-boot-animation)
// initialize OOBE/sign in WebUI in hidden state in parallel with
// wallpaper animation.
const bool kInitializeWebUIInParallelDefault = true;

// Switch values that might be used to override WebUI init type.
const char kWebUIInitParallel[] = "parallel";
const char kWebUIInitPostpone[] = "postpone";

}  // namespace

// WebUILoginDisplayHost -------------------------------------------------------

WebUILoginDisplayHost::WebUILoginDisplayHost(const gfx::Rect& background_bounds)
    : BaseLoginDisplayHost(background_bounds),
      login_window_(NULL),
      login_view_(NULL),
      webui_login_display_(NULL),
      is_showing_login_(false),
      is_wallpaper_loaded_(false),
      initialize_webui_in_parallel_(kInitializeWebUIInParallelDefault),
      status_area_saved_visibility_(false),
      crash_count_(0),
      restore_path_(RESTORE_UNKNOWN) {
  bool is_registered = WizardController::IsDeviceRegistered();
  bool zero_delay_enabled = WizardController::IsZeroDelayEnabled();
  if (!CommandLine::ForCurrentProcess()->HasSwitch(switches::kDisableNewOobe) &&
      !CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableOobeAnimation) &&
      !zero_delay_enabled) {
    bool disable_boot_animation = CommandLine::ForCurrentProcess()->
        HasSwitch(switches::kDisableBootAnimation);
    waiting_for_wallpaper_load_ = !is_registered || !disable_boot_animation;
  } else {
    waiting_for_wallpaper_load_ = false;
  }

  if (waiting_for_wallpaper_load_) {
    registrar_.Add(this, chrome::NOTIFICATION_WALLPAPER_ANIMATION_FINISHED,
                   content::NotificationService::AllSources());
    // Prevents white flashing on OOBE (http://crbug.com/131569).
    aura::Env::GetInstance()->set_render_white_bg(false);

    // Check if WebUI init type is overriden.
    if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kAshWebUIInit)) {
      const std::string override_type = CommandLine::ForCurrentProcess()->
          GetSwitchValueASCII(switches::kAshWebUIInit);
      if (override_type == kWebUIInitParallel)
        initialize_webui_in_parallel_ = true;
      else if (override_type == kWebUIInitPostpone)
        initialize_webui_in_parallel_ = false;
    }

    // Don't postpone WebUI initialization on first boot, otherwise we miss
    // initial animation.
    if (!WizardController::IsOobeCompleted())
      initialize_webui_in_parallel_ = false;
  }
  // In case if we're not waiting for wallpaper load,
  // |initialize_webui_in_parallel_| value is ignored through the code flow.
}

WebUILoginDisplayHost::~WebUILoginDisplayHost() {
  if (login_window_)
    login_window_->Close();
}

// LoginDisplayHost implementation ---------------------------------------------

LoginDisplay* WebUILoginDisplayHost::CreateLoginDisplay(
    LoginDisplay::Delegate* delegate) {
  webui_login_display_ = new WebUILoginDisplay(delegate);
  webui_login_display_->set_background_bounds(background_bounds());
  return webui_login_display_;
}

gfx::NativeWindow WebUILoginDisplayHost::GetNativeWindow() const {
  return login_window_ ? login_window_->GetNativeWindow() : NULL;
}

views::Widget* WebUILoginDisplayHost::GetWidget() const {
  return login_window_;
}

void WebUILoginDisplayHost::OpenProxySettings() {
  if (login_view_)
    login_view_->OpenProxySettings();
}

void WebUILoginDisplayHost::SetOobeProgressBarVisible(bool visible) {
  GetOobeUI()->ShowOobeUI(visible);
}

void WebUILoginDisplayHost::SetShutdownButtonEnabled(bool enable) {
}

void WebUILoginDisplayHost::SetStatusAreaVisible(bool visible) {
  if (waiting_for_wallpaper_load_ && initialize_webui_in_parallel_)
    status_area_saved_visibility_ = visible;
  else if (login_view_)
    login_view_->SetStatusAreaVisible(visible);
}

void WebUILoginDisplayHost::StartWizard(const std::string& first_screen_name,
                                        DictionaryValue* screen_parameters) {
  // Keep parameters to restore if renderer crashes.
  restore_path_ = RESTORE_WIZARD;
  wizard_first_screen_name_ = first_screen_name;
  if (screen_parameters)
    wizard_screen_parameters_.reset(screen_parameters->DeepCopy());
  else
    wizard_screen_parameters_.reset(NULL);
  is_showing_login_ = false;
  scoped_ptr<DictionaryValue> scoped_parameters(screen_parameters);

  if (waiting_for_wallpaper_load_ && !initialize_webui_in_parallel_)
    return;

  if (!login_window_)
    LoadURL(GURL(kOobeURL));

  BaseLoginDisplayHost::StartWizard(first_screen_name,
                                    scoped_parameters.release());
}

void WebUILoginDisplayHost::StartSignInScreen() {
  restore_path_ = RESTORE_SIGN_IN;
  is_showing_login_ = true;

  if (waiting_for_wallpaper_load_ && !initialize_webui_in_parallel_)
    return;

  if (!login_window_)
    LoadURL(GURL(kLoginURL));

  BaseLoginDisplayHost::StartSignInScreen();
  CHECK(webui_login_display_);
  GetOobeUI()->ShowSigninScreen(webui_login_display_);
  if (chromeos::KioskModeSettings::Get()->IsKioskModeEnabled())
    SetStatusAreaVisible(false);
}

void WebUILoginDisplayHost::OnPreferencesChanged() {
  if (is_showing_login_)
    webui_login_display_->OnPreferencesChanged();
}

void WebUILoginDisplayHost::OnBrowserCreated() {
  // Close lock window now so that the launched browser can receive focus.
  if (login_window_) {
    login_window_->Close();
    login_window_ = NULL;
    login_view_ = NULL;
  }
}

void WebUILoginDisplayHost::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  BaseLoginDisplayHost::Observe(type, source, details);
  if (chrome::NOTIFICATION_WALLPAPER_ANIMATION_FINISHED == type) {
    is_wallpaper_loaded_ = true;
    ash::Shell::GetInstance()->user_wallpaper_delegate()->
        OnWallpaperBootAnimationFinished();
    if (waiting_for_wallpaper_load_) {
      // StartWizard / StartSignInScreen could be called multiple times through
      // the lifetime of host.
      // Make sure that subsequent calls are not postponed.
      waiting_for_wallpaper_load_ = false;
      if (initialize_webui_in_parallel_)
        ShowWebUI();
      else
        StartPostponedWebUI();
    }
    registrar_.Remove(this,
                      chrome::NOTIFICATION_WALLPAPER_ANIMATION_FINISHED,
                      content::NotificationService::AllSources());
  }
}

void WebUILoginDisplayHost::LoadURL(const GURL& url) {
  if (!login_window_) {
    views::Widget::InitParams params(
        views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
    params.bounds = background_bounds();
    params.show_state = ui::SHOW_STATE_FULLSCREEN;
    if (!CommandLine::ForCurrentProcess()->HasSwitch(switches::kDisableNewOobe))
      params.transparent = true;
    params.parent =
        ash::Shell::GetContainer(
            ash::Shell::GetPrimaryRootWindow(),
            ash::internal::kShellWindowId_LockScreenContainer);

    login_window_ = new views::Widget;
    login_window_->Init(params);
    login_view_ = new WebUILoginView();

    login_view_->Init(login_window_);

    ash::SetWindowVisibilityAnimationDuration(
        login_window_->GetNativeView(),
        base::TimeDelta::FromMilliseconds(kLoginFadeoutTransitionDurationMs));
    ash::SetWindowVisibilityAnimationTransition(
        login_window_->GetNativeView(),
        ash::ANIMATE_HIDE);

    login_window_->SetContentsView(login_view_);
    login_view_->UpdateWindowType();

    // When not waiting for wallpaper any request to load a URL in WebUI
    // should trigger window visibility as well.
    // Otherwise, when we're waiting for wallpaper load then show WebUI
    // right away only if it is not initialized in parallel i.e. was postponed.
    // In case of WebUI being initialized in parallel with wallpaper load
    // it will be hidden initially.
    if (!waiting_for_wallpaper_load_ || !initialize_webui_in_parallel_)
      login_window_->Show();
    else
      login_view_->set_is_hidden(true);
    login_window_->GetNativeView()->SetName("WebUILoginView");
    login_view_->OnWindowCreated();
  }
  // Subscribe to crash events.
  content::WebContentsObserver::Observe(login_view_->GetWebContents());
  login_view_->LoadURL(url);
}

void WebUILoginDisplayHost::RenderViewGone(base::TerminationStatus status) {
  // Do not try to restore on shutdown
  if (browser_shutdown::GetShutdownType() != browser_shutdown::NOT_VALID)
    return;

  crash_count_++;
  if (crash_count_ > kCrashCountLimit)
    return;

  if (status != base::TERMINATION_STATUS_NORMAL_TERMINATION) {
    // Restart if renderer has crashed.
    LOG(ERROR) << "Renderer crash on login window";
    switch (restore_path_) {
      case RESTORE_WIZARD:
        StartWizard(wizard_first_screen_name_,
                    wizard_screen_parameters_.release());
        break;
      case RESTORE_SIGN_IN:
        StartSignInScreen();
        break;
      default:
        NOTREACHED();
        break;
    }
  }
}

OobeUI* WebUILoginDisplayHost::GetOobeUI() const {
  if (!login_view_)
    return NULL;
  return static_cast<OobeUI*>(login_view_->GetWebUI()->GetController());
}

WizardController* WebUILoginDisplayHost::CreateWizardController() {
  // TODO(altimofeev): ensure that WebUI is ready.
  OobeDisplay* oobe_display = GetOobeUI();
  return new WizardController(this, oobe_display);
}

void WebUILoginDisplayHost::ShowWebUI() {
  if (!login_window_ || !login_view_) {
    NOTREACHED();
    return;
  }
  login_window_->Show();
  login_view_->GetWebContents()->Focus();
  login_view_->SetStatusAreaVisible(status_area_saved_visibility_);
  login_view_->OnPostponedShow();
}

void WebUILoginDisplayHost::StartPostponedWebUI() {
  if (!is_wallpaper_loaded_) {
    NOTREACHED();
    return;
  }

  // Wallpaper has finished loading before StartWizard/StartSignInScreen has
  // been called. In general this should not happen.
  // Let go through normal code path when one of those will be called.
  if (restore_path_ == RESTORE_UNKNOWN) {
    NOTREACHED();
    return;
  }

  switch (restore_path_) {
    case RESTORE_WIZARD:
      StartWizard(wizard_first_screen_name_,
                  wizard_screen_parameters_.release());
      break;
    case RESTORE_SIGN_IN:
      StartSignInScreen();
      break;
    default:
      NOTREACHED();
      break;
  }
}

}  // namespace chromeos
