// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/window_sizer/window_sizer.h"

#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/prefs/pref_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window_state.h"
#include "chrome/browser/ui/host_desktop.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "ui/base/ui_base_switches.h"
#include "ui/gfx/screen.h"

#if defined(USE_ASH)
#include "ash/shell.h"
#include "ash/wm/window_positioner.h"
#include "chrome/browser/ui/ash/ash_init.h"
#endif

namespace {

// Minimum height of the visible part of a window.
const int kMinVisibleHeight = 30;
// Minimum width of the visible part of a window.
const int kMinVisibleWidth = 30;

///////////////////////////////////////////////////////////////////////////////
// An implementation of WindowSizer::StateProvider that gets the last active
// and persistent state from the browser window and the user's profile.
class DefaultStateProvider : public WindowSizer::StateProvider {
 public:
  DefaultStateProvider(const std::string& app_name, const Browser* browser)
      : app_name_(app_name), browser_(browser) {
  }

  // Overridden from WindowSizer::StateProvider:
  virtual bool GetPersistentState(
      gfx::Rect* bounds,
      gfx::Rect* work_area,
      ui::WindowShowState* show_state) const OVERRIDE {
    DCHECK(bounds);
    DCHECK(show_state);

    if (!browser_ || !browser_->profile()->GetPrefs())
      return false;

    std::string window_name(chrome::GetWindowPlacementKey(browser_));
    const base::DictionaryValue* wp_pref =
        browser_->profile()->GetPrefs()->GetDictionary(window_name.c_str());
    int top = 0, left = 0, bottom = 0, right = 0;
    bool maximized = false;
    bool has_prefs = wp_pref &&
                     wp_pref->GetInteger("top", &top) &&
                     wp_pref->GetInteger("left", &left) &&
                     wp_pref->GetInteger("bottom", &bottom) &&
                     wp_pref->GetInteger("right", &right) &&
                     wp_pref->GetBoolean("maximized", &maximized);
    bounds->SetRect(left, top, std::max(0, right - left),
                    std::max(0, bottom - top));

    int work_area_top = 0;
    int work_area_left = 0;
    int work_area_bottom = 0;
    int work_area_right = 0;
    if (wp_pref) {
      wp_pref->GetInteger("work_area_top", &work_area_top);
      wp_pref->GetInteger("work_area_left", &work_area_left);
      wp_pref->GetInteger("work_area_bottom", &work_area_bottom);
      wp_pref->GetInteger("work_area_right", &work_area_right);
      if (*show_state == ui::SHOW_STATE_DEFAULT && maximized)
        *show_state = ui::SHOW_STATE_MAXIMIZED;
    }
    work_area->SetRect(work_area_left, work_area_top,
                      std::max(0, work_area_right - work_area_left),
                      std::max(0, work_area_bottom - work_area_top));

    return has_prefs;
  }

  virtual bool GetLastActiveWindowState(
      gfx::Rect* bounds,
      ui::WindowShowState* show_state) const OVERRIDE {
    DCHECK(show_state);
    // Applications are always restored with the same position.
    if (!app_name_.empty())
      return false;

    // If a reference browser is set, use its window. Otherwise find last
    // active. Panels are never used as reference browsers as panels are
    // specially positioned.
    BrowserWindow* window = NULL;
    // Window may be null if browser is just starting up.
    if (browser_ && browser_->window()) {
      window = browser_->window();
    } else {
      // This code is only ran on the native desktop (on the ash
      // desktop, GetTabbedBrowserBoundsAsh should take over below
      // before this is reached).  TODO(gab): This code should go in a
      // native desktop specific window sizer as part of fixing
      // crbug.com/175812.
      const BrowserList* native_browser_list =
          BrowserList::GetInstance(chrome::HOST_DESKTOP_TYPE_NATIVE);
      for (BrowserList::const_reverse_iterator it =
               native_browser_list->begin_last_active();
           it != native_browser_list->end_last_active(); ++it) {
        Browser* last_active = *it;
        if (last_active && last_active->is_type_tabbed()) {
          window = last_active->window();
          DCHECK(window);
          break;
        }
      }
    }

    if (window) {
      *bounds = window->GetRestoredBounds();
      if (*show_state == ui::SHOW_STATE_DEFAULT && window->IsMaximized())
        *show_state = ui::SHOW_STATE_MAXIMIZED;
      return true;
    }

    return false;
  }

 private:
  std::string app_name_;

  // If set, is used as the reference browser for GetLastActiveWindowState.
  const Browser* browser_;
  DISALLOW_COPY_AND_ASSIGN(DefaultStateProvider);
};

class DefaultTargetDisplayProvider : public WindowSizer::TargetDisplayProvider {
 public:
  DefaultTargetDisplayProvider() {}
  virtual ~DefaultTargetDisplayProvider() {}

  virtual gfx::Display GetTargetDisplay(
      const gfx::Screen* screen,
      const gfx::Rect& bounds) const OVERRIDE {
#if defined(USE_ASH)
    bool force_ash = false;
    // On Windows check if the browser is launched to serve ASH. If yes then
    // we should get the display for the corresponding root window created for
    // ASH. This ensures that the display gets the correct workarea, etc.
    // If the ASH shell does not exist then the current behavior is to open
    // browser windows if any on the desktop. Preserve that for now.
    // TODO(ananta).
    // This effectively means that the running browser process is in a split
    // personality mode, part of it running in ASH and the other running in
    // desktop. This may cause apps and other widgets to not work correctly.
    // Revisit and address.
#if defined(OS_WIN)
    force_ash = ash::Shell::HasInstance() &&
        CommandLine::ForCurrentProcess()->HasSwitch(switches::kViewerConnect);
#endif
    // Use the target display on ash.
    if (chrome::ShouldOpenAshOnStartup() || force_ash) {
      aura::Window* target = ash::Shell::GetTargetRootWindow();
      return screen->GetDisplayNearestWindow(target);
    }
#endif
    // Find the size of the work area of the monitor that intersects the bounds
    // of the anchor window.
    return screen->GetDisplayMatching(bounds);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DefaultTargetDisplayProvider);
};

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// WindowSizer, public:

WindowSizer::WindowSizer(
    scoped_ptr<StateProvider> state_provider,
    scoped_ptr<TargetDisplayProvider> target_display_provider,
    const Browser* browser)
    : state_provider_(state_provider.Pass()),
      target_display_provider_(target_display_provider.Pass()),
      // TODO(scottmg): NativeScreen is wrong. http://crbug.com/133312
      screen_(gfx::Screen::GetNativeScreen()),
      browser_(browser) {
}

WindowSizer::WindowSizer(
    scoped_ptr<StateProvider> state_provider,
    scoped_ptr<TargetDisplayProvider> target_display_provider,
    gfx::Screen* screen,
    const Browser* browser)
    : state_provider_(state_provider.Pass()),
      target_display_provider_(target_display_provider.Pass()),
      screen_(screen),
      browser_(browser) {
  DCHECK(screen_);
}

WindowSizer::~WindowSizer() {
}

// static
void WindowSizer::GetBrowserWindowBoundsAndShowState(
    const std::string& app_name,
    const gfx::Rect& specified_bounds,
    const Browser* browser,
    gfx::Rect* window_bounds,
    ui::WindowShowState* show_state) {
  scoped_ptr<StateProvider> state_provider(
      new DefaultStateProvider(app_name, browser));
  scoped_ptr<TargetDisplayProvider> target_display_provider(
      new DefaultTargetDisplayProvider);
  const WindowSizer sizer(state_provider.Pass(),
                          target_display_provider.Pass(),
                          browser);
  sizer.DetermineWindowBoundsAndShowState(specified_bounds,
                                          window_bounds,
                                          show_state);
}

///////////////////////////////////////////////////////////////////////////////
// WindowSizer, private:

void WindowSizer::DetermineWindowBoundsAndShowState(
    const gfx::Rect& specified_bounds,
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  DCHECK(bounds);
  DCHECK(show_state);
  // Pre-populate the window state with our default.
  *show_state = GetWindowDefaultShowState();
  *bounds = specified_bounds;

#if defined(USE_ASH)
  // See if ash should decide the window placement.
  if (GetBrowserBoundsAsh(bounds, show_state))
    return;
#endif

  if (bounds->IsEmpty()) {
    // See if there's last active window's placement information.
    if (GetLastActiveWindowBounds(bounds, show_state))
      return;
    // See if there's saved placement information.
    if (GetSavedWindowBounds(bounds, show_state))
      return;

    // No saved placement, figure out some sensible default size based on
    // the user's screen size.
    GetDefaultWindowBounds(GetTargetDisplay(gfx::Rect()), bounds);
    return;
  }

  // In case that there was a bound given we need to make sure that it is
  // visible and fits on the screen.
  // Find the size of the work area of the monitor that intersects the bounds
  // of the anchor window. Note: AdjustBoundsToBeVisibleOnMonitorContaining
  // does not exactly what we want: It makes only sure that "a minimal part"
  // is visible on the screen.
  gfx::Rect work_area = screen_->GetDisplayMatching(*bounds).work_area();
  // Resize so that it fits.
  bounds->AdjustToFit(work_area);
}

bool WindowSizer::GetLastActiveWindowBounds(
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  DCHECK(bounds);
  DCHECK(show_state);
  if (!state_provider_.get() ||
      !state_provider_->GetLastActiveWindowState(bounds, show_state))
    return false;
  gfx::Rect last_window_bounds = *bounds;
  bounds->Offset(kWindowTilePixels, kWindowTilePixels);
  AdjustBoundsToBeVisibleOnDisplay(screen_->GetDisplayMatching(*bounds),
                                   gfx::Rect(),
                                   bounds);
  return true;
}

bool WindowSizer::GetSavedWindowBounds(gfx::Rect* bounds,
                                       ui::WindowShowState* show_state) const {
  DCHECK(bounds);
  DCHECK(show_state);
  gfx::Rect saved_work_area;
  if (!state_provider_.get() ||
      !state_provider_->GetPersistentState(bounds,
                                           &saved_work_area,
                                           show_state))
    return false;
  AdjustBoundsToBeVisibleOnDisplay(GetTargetDisplay(*bounds),
                                   saved_work_area,
                                   bounds);
  return true;
}

void WindowSizer::GetDefaultWindowBounds(const gfx::Display& display,
                                         gfx::Rect* default_bounds) const {
  DCHECK(default_bounds);
#if defined(USE_ASH)
  // TODO(beng): insufficient but currently necessary. http://crbug.com/133312
  if (chrome::ShouldOpenAshOnStartup()) {
    *default_bounds = ash::WindowPositioner::GetDefaultWindowBounds(display);
    return;
  }
#endif
  gfx::Rect work_area = display.work_area();

  // The default size is either some reasonably wide width, or if the work
  // area is narrower, then the work area width less some aesthetic padding.
  int default_width = std::min(work_area.width() - 2 * kWindowTilePixels, 1050);
  int default_height = work_area.height() - 2 * kWindowTilePixels;

  // For wider aspect ratio displays at higher resolutions, we might size the
  // window narrower to allow two windows to easily be placed side-by-side.
  gfx::Rect screen_size = screen_->GetPrimaryDisplay().bounds();
  double width_to_height =
    static_cast<double>(screen_size.width()) / screen_size.height();

  // The least wide a screen can be to qualify for the halving described above.
  static const int kMinScreenWidthForWindowHalving = 1600;
  // We assume 16:9/10 is a fairly standard indicator of a wide aspect ratio
  // computer display.
  if (((width_to_height * 10) >= 16) &&
      work_area.width() > kMinScreenWidthForWindowHalving) {
    // Halve the work area, subtracting aesthetic padding on either side.
    // The padding is set so that two windows, side by side have
    // kWindowTilePixels between screen edge and each other.
    default_width = static_cast<int>(work_area.width() / 2. -
        1.5 * kWindowTilePixels);
  }
  default_bounds->SetRect(kWindowTilePixels + work_area.x(),
                          kWindowTilePixels + work_area.y(),
                          default_width, default_height);
}

void WindowSizer::AdjustBoundsToBeVisibleOnDisplay(
    const gfx::Display& display,
    const gfx::Rect& saved_work_area,
    gfx::Rect* bounds) const {
  DCHECK(bounds);

  // If |bounds| is empty, reset to the default size.
  if (bounds->IsEmpty()) {
    gfx::Rect default_bounds;
    GetDefaultWindowBounds(display, &default_bounds);
    if (bounds->height() <= 0)
      bounds->set_height(default_bounds.height());
    if (bounds->width() <= 0)
      bounds->set_width(default_bounds.width());
  }

  // Ensure the minimum height and width.
  bounds->set_height(std::max(kMinVisibleHeight, bounds->height()));
  bounds->set_width(std::max(kMinVisibleWidth, bounds->width()));

  gfx::Rect work_area = display.work_area();
  // Ensure that the title bar is not above the work area.
  if (bounds->y() < work_area.y())
    bounds->set_y(work_area.y());

  // Reposition and resize the bounds if the saved_work_area is different from
  // the current work area and the current work area doesn't completely contain
  // the bounds.
  if (!saved_work_area.IsEmpty() &&
      saved_work_area != work_area &&
      !work_area.Contains(*bounds)) {
    bounds->set_width(std::min(bounds->width(), work_area.width()));
    bounds->set_height(std::min(bounds->height(), work_area.height()));
    bounds->set_x(
        std::max(work_area.x(),
                 std::min(bounds->x(), work_area.right() - bounds->width())));
    bounds->set_y(
        std::max(work_area.y(),
                 std::min(bounds->y(), work_area.bottom() - bounds->height())));
  }

#if defined(OS_MACOSX)
  // Limit the maximum height.  On the Mac the sizer is on the
  // bottom-right of the window, and a window cannot be moved "up"
  // past the menubar.  If the window is too tall you'll never be able
  // to shrink it again.  Windows does not have this limitation
  // (e.g. can be resized from the top).
  bounds->set_height(std::min(work_area.height(), bounds->height()));

  // On mac, we want to be aggressive about repositioning windows that are
  // partially offscreen.  If the window is partially offscreen horizontally,
  // move it to be flush with the left edge of the work area.
  if (bounds->x() < work_area.x() || bounds->right() > work_area.right())
    bounds->set_x(work_area.x());

  // If the window is partially offscreen vertically, move it to be flush with
  // the top of the work area.
  if (bounds->y() < work_area.y() || bounds->bottom() > work_area.bottom())
    bounds->set_y(work_area.y());
#else
  // On non-Mac platforms, we are less aggressive about repositioning.  Simply
  // ensure that at least kMinVisibleWidth * kMinVisibleHeight is visible.
  const int min_y = work_area.y() + kMinVisibleHeight - bounds->height();
  const int min_x = work_area.x() + kMinVisibleWidth - bounds->width();
  const int max_y = work_area.bottom() - kMinVisibleHeight;
  const int max_x = work_area.right() - kMinVisibleWidth;
  bounds->set_y(std::max(min_y, std::min(max_y, bounds->y())));
  bounds->set_x(std::max(min_x, std::min(max_x, bounds->x())));
#endif  // defined(OS_MACOSX)
}

gfx::Display WindowSizer::GetTargetDisplay(const gfx::Rect& bounds) const {
  return target_display_provider_->GetTargetDisplay(screen_, bounds);
}

ui::WindowShowState WindowSizer::GetWindowDefaultShowState() const {
  if (!browser_)
    return ui::SHOW_STATE_DEFAULT;

  // Only tabbed browsers use the command line or preference state, with the
  // exception of devtools.
  bool show_state = !browser_->is_type_tabbed() && !browser_->is_devtools();

#if defined(USE_AURA)
  // We use the apps save state on aura.
  show_state &= !browser_->is_app();
#endif

  if (show_state)
    return browser_->initial_show_state();

  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kStartMaximized))
    return ui::SHOW_STATE_MAXIMIZED;

  if (browser_->initial_show_state() != ui::SHOW_STATE_DEFAULT)
    return browser_->initial_show_state();

  // Otherwise we use the default which can be overridden later on.
  return ui::SHOW_STATE_DEFAULT;
}
