// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "athena/home/public/home_card.h"

#include <cmath>
#include <limits>

#include "athena/common/container_priorities.h"
#include "athena/env/public/athena_env.h"
#include "athena/home/app_list_view_delegate.h"
#include "athena/home/athena_start_page_view.h"
#include "athena/home/home_card_constants.h"
#include "athena/home/home_card_gesture_manager.h"
#include "athena/home/minimized_home.h"
#include "athena/home/public/app_model_builder.h"
#include "athena/input/public/accelerator_manager.h"
#include "athena/screen/public/screen_manager.h"
#include "athena/wm/public/window_manager.h"
#include "athena/wm/public/window_manager_observer.h"
#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "ui/app_list/search_provider.h"
#include "ui/app_list/views/app_list_main_view.h"
#include "ui/app_list/views/contents_view.h"
#include "ui/aura/layout_manager.h"
#include "ui/aura/window.h"
#include "ui/compositor/closure_animation_observer.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/views/background.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/wm/core/shadow_types.h"
#include "ui/wm/core/visibility_controller.h"
#include "ui/wm/core/window_animations.h"
#include "ui/wm/public/activation_change_observer.h"
#include "ui/wm/public/activation_client.h"

namespace athena {
namespace {

HomeCard* instance = NULL;

gfx::Rect GetBoundsForState(const gfx::Rect& screen_bounds,
                            HomeCard::State state) {
  switch (state) {
    case HomeCard::HIDDEN:
      break;

    case HomeCard::VISIBLE_CENTERED:
      return screen_bounds;

    case HomeCard::VISIBLE_BOTTOM:
      return gfx::Rect(0,
                       screen_bounds.bottom() - kHomeCardHeight,
                       screen_bounds.width(),
                       kHomeCardHeight);
    case HomeCard::VISIBLE_MINIMIZED:
      return gfx::Rect(0,
                       screen_bounds.bottom() - kHomeCardMinimizedHeight,
                       screen_bounds.width(),
                       kHomeCardMinimizedHeight);
  }

  NOTREACHED();
  return gfx::Rect();
}

// Makes sure the homecard is center-aligned horizontally and bottom-aligned
// vertically.
class HomeCardLayoutManager : public aura::LayoutManager {
 public:
  explicit HomeCardLayoutManager()
      : home_card_(NULL) {}

  virtual ~HomeCardLayoutManager() {}

  void Layout() {
    // |home_card| could be detached from the root window (e.g. when it is being
    // destroyed).
    if (!home_card_ || !home_card_->IsVisible() || !home_card_->GetRootWindow())
      return;

    {
      ui::ScopedLayerAnimationSettings settings(
          home_card_->layer()->GetAnimator());
      settings.SetTweenType(gfx::Tween::EASE_IN_OUT);
      SetChildBoundsDirect(home_card_, GetBoundsForState(
          home_card_->GetRootWindow()->bounds(), HomeCard::Get()->GetState()));
    }
  }

 private:
  // aura::LayoutManager:
  virtual void OnWindowResized() OVERRIDE { Layout(); }
  virtual void OnWindowAddedToLayout(aura::Window* child) OVERRIDE {
    if (!home_card_) {
      home_card_ = child;
      Layout();
    }
  }
  virtual void OnWillRemoveWindowFromLayout(aura::Window* child) OVERRIDE {
    if (home_card_ == child)
      home_card_ = NULL;
  }
  virtual void OnWindowRemovedFromLayout(aura::Window* child) OVERRIDE {
    Layout();
  }
  virtual void OnChildWindowVisibilityChanged(aura::Window* child,
                                              bool visible) OVERRIDE {
    Layout();
  }
  virtual void SetChildBounds(aura::Window* child,
                              const gfx::Rect& requested_bounds) OVERRIDE {
    SetChildBoundsDirect(child, requested_bounds);
  }

  aura::Window* home_card_;

  DISALLOW_COPY_AND_ASSIGN(HomeCardLayoutManager);
};

// The container view of home card contents of each state.
class HomeCardView : public views::WidgetDelegateView {
 public:
  HomeCardView(app_list::AppListViewDelegate* view_delegate,
               aura::Window* container,
               HomeCardGestureManager::Delegate* gesture_delegate)
      : gesture_delegate_(gesture_delegate),
        weak_factory_(this) {
    // Ideally AppListMainView should be used here and have AthenaStartPageView
    // as its child view, so that custom pages and apps grid are available in
    // the home card.
    // TODO(mukai): make it so after the detailed UI has been fixed.
    main_view_ = new AthenaStartPageView(view_delegate);
    AddChildView(main_view_);

    minimized_view_ = CreateMinimizedHome();
    minimized_view_->SetPaintToLayer(true);
    AddChildView(minimized_view_);
  }

  void SetStateProgress(HomeCard::State from_state,
                        HomeCard::State to_state,
                        float progress) {
    if (from_state == HomeCard::VISIBLE_BOTTOM &&
        to_state == HomeCard::VISIBLE_MINIMIZED) {
      SetStateProgress(to_state, from_state, 1.0 - progress);
      return;
    }

    // View from minimized to bottom.
    if (from_state == HomeCard::VISIBLE_MINIMIZED &&
        to_state == HomeCard::VISIBLE_BOTTOM) {
      main_view_->SetVisible(true);
      minimized_view_->SetVisible(true);
      minimized_view_->layer()->SetOpacity(1.0f - progress);
      return;
    }

    SetState(to_state);
  }

  void SetState(HomeCard::State state) {
    main_view_->SetVisible(state == HomeCard::VISIBLE_BOTTOM ||
                           state == HomeCard::VISIBLE_CENTERED);
    minimized_view_->SetVisible(state == HomeCard::VISIBLE_MINIMIZED);
    if (minimized_view_->visible())
      minimized_view_->layer()->SetOpacity(1.0f);
    if (state == HomeCard::VISIBLE_CENTERED)
      main_view_->RequestFocusOnSearchBox();
    else
      GetWidget()->GetFocusManager()->ClearFocus();
    wm::SetShadowType(GetWidget()->GetNativeView(),
                      state == HomeCard::VISIBLE_MINIMIZED ?
                      wm::SHADOW_TYPE_NONE :
                      wm::SHADOW_TYPE_RECTANGULAR);
  }

  void SetStateWithAnimation(HomeCard::State from_state,
                             HomeCard::State to_state) {
    if ((from_state == HomeCard::VISIBLE_MINIMIZED &&
         to_state == HomeCard::VISIBLE_BOTTOM) ||
        (from_state == HomeCard::VISIBLE_BOTTOM &&
         to_state == HomeCard::VISIBLE_MINIMIZED)) {
      minimized_view_->SetVisible(true);
      main_view_->SetVisible(true);
      {
        ui::ScopedLayerAnimationSettings settings(
            minimized_view_->layer()->GetAnimator());
        settings.SetTweenType(gfx::Tween::EASE_IN_OUT);
        settings.AddObserver(new ui::ClosureAnimationObserver(
            base::Bind(&HomeCardView::SetState,
                       weak_factory_.GetWeakPtr(),
                       to_state)));
        minimized_view_->layer()->SetOpacity(
            (to_state == HomeCard::VISIBLE_MINIMIZED) ? 1.0f : 0.0f);
      }
    } else {
      // TODO(mukai): Take care of other transition.
      SetState(to_state);
    }
  }

  void ClearGesture() {
    gesture_manager_.reset();
  }

  // views::View:
  virtual void Layout() OVERRIDE {
    for (int i = 0; i < child_count(); ++i) {
      views::View* child = child_at(i);
      if (child->visible()) {
        if (child == minimized_view_) {
          gfx::Rect minimized_bounds = bounds();
          minimized_bounds.set_y(
              minimized_bounds.bottom() - kHomeCardMinimizedHeight);
          minimized_bounds.set_height(kHomeCardMinimizedHeight);
          child->SetBoundsRect(minimized_bounds);
        } else {
          child->SetBoundsRect(bounds());
        }
      }
    }
  }
  virtual void OnGestureEvent(ui::GestureEvent* event) OVERRIDE {
    if (!gesture_manager_ &&
        event->type() == ui::ET_GESTURE_SCROLL_BEGIN) {
      gesture_manager_.reset(new HomeCardGestureManager(
          gesture_delegate_,
          GetWidget()->GetNativeWindow()->GetRootWindow()->bounds()));
    }

    if (gesture_manager_)
      gesture_manager_->ProcessGestureEvent(event);
  }

 private:
  // views::WidgetDelegate:
  virtual views::View* GetContentsView() OVERRIDE {
    return this;
  }

  AthenaStartPageView* main_view_;
  views::View* minimized_view_;
  scoped_ptr<HomeCardGestureManager> gesture_manager_;
  HomeCardGestureManager::Delegate* gesture_delegate_;
  base::WeakPtrFactory<HomeCardView> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(HomeCardView);
};

class HomeCardImpl : public HomeCard,
                     public AcceleratorHandler,
                     public HomeCardGestureManager::Delegate,
                     public WindowManagerObserver,
                     public aura::client::ActivationChangeObserver {
 public:
  explicit HomeCardImpl(AppModelBuilder* model_builder);
  virtual ~HomeCardImpl();

  void Init();

 private:
  enum Command {
    COMMAND_SHOW_HOME_CARD,
  };
  void InstallAccelerators();

  // Overridden from HomeCard:
  virtual void SetState(State state) OVERRIDE;
  virtual State GetState() OVERRIDE;
  virtual void RegisterSearchProvider(
      app_list::SearchProvider* search_provider) OVERRIDE;
  virtual void UpdateVirtualKeyboardBounds(
      const gfx::Rect& bounds) OVERRIDE;

  // AcceleratorHandler:
  virtual bool IsCommandEnabled(int command_id) const OVERRIDE { return true; }
  virtual bool OnAcceleratorFired(int command_id,
                                  const ui::Accelerator& accelerator) OVERRIDE;

  // HomeCardGestureManager::Delegate:
  virtual void OnGestureEnded(State final_state) OVERRIDE;
  virtual void OnGestureProgressed(
      State from_state, State to_state, float progress) OVERRIDE;

  // WindowManagerObserver:
  virtual void OnOverviewModeEnter() OVERRIDE;
  virtual void OnOverviewModeExit() OVERRIDE;

  // aura::client::ActivationChangeObserver:
  virtual void OnWindowActivated(aura::Window* gained_active,
                                 aura::Window* lost_active) OVERRIDE;

  scoped_ptr<AppModelBuilder> model_builder_;

  HomeCard::State state_;

  // original_state_ is the state which the home card should go back to after
  // the virtual keyboard is hidden.
  HomeCard::State original_state_;

  views::Widget* home_card_widget_;
  HomeCardView* home_card_view_;
  scoped_ptr<AppListViewDelegate> view_delegate_;
  HomeCardLayoutManager* layout_manager_;
  aura::client::ActivationClient* activation_client_;  // Not owned

  // Right now HomeCard allows only one search provider.
  // TODO(mukai): port app-list's SearchController and Mixer.
  scoped_ptr<app_list::SearchProvider> search_provider_;

  DISALLOW_COPY_AND_ASSIGN(HomeCardImpl);
};

HomeCardImpl::HomeCardImpl(AppModelBuilder* model_builder)
    : model_builder_(model_builder),
      state_(HIDDEN),
      original_state_(VISIBLE_MINIMIZED),
      home_card_widget_(NULL),
      home_card_view_(NULL),
      layout_manager_(NULL),
      activation_client_(NULL) {
  DCHECK(!instance);
  instance = this;
  WindowManager::GetInstance()->AddObserver(this);
}

HomeCardImpl::~HomeCardImpl() {
  DCHECK(instance);
  WindowManager::GetInstance()->RemoveObserver(this);
  if (activation_client_)
    activation_client_->RemoveObserver(this);
  home_card_widget_->CloseNow();

  // Reset the view delegate first as it access search provider during
  // shutdown.
  view_delegate_.reset();
  search_provider_.reset();
  instance = NULL;
}

void HomeCardImpl::Init() {
  InstallAccelerators();
  ScreenManager::ContainerParams params("HomeCardContainer", CP_HOME_CARD);
  params.can_activate_children = true;
  aura::Window* container = ScreenManager::Get()->CreateContainer(params);
  layout_manager_ = new HomeCardLayoutManager();

  container->SetLayoutManager(layout_manager_);
  wm::SetChildWindowVisibilityChangesAnimated(container);

  view_delegate_.reset(new AppListViewDelegate(model_builder_.get()));
  if (search_provider_)
    view_delegate_->RegisterSearchProvider(search_provider_.get());

  home_card_view_ = new HomeCardView(view_delegate_.get(), container, this);
  home_card_widget_ = new views::Widget();
  views::Widget::InitParams widget_params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  widget_params.parent = container;
  widget_params.delegate = home_card_view_;
  widget_params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  home_card_widget_->Init(widget_params);

  SetState(VISIBLE_MINIMIZED);
  home_card_view_->Layout();

  activation_client_ =
      aura::client::GetActivationClient(container->GetRootWindow());
  if (activation_client_)
    activation_client_->AddObserver(this);

  int work_area_bottom_inset =
      GetBoundsForState(home_card_widget_->GetNativeWindow()->bounds(),
                        HomeCard::VISIBLE_MINIMIZED).height();
  AthenaEnv::Get()->SetDisplayWorkAreaInsets(
      gfx::Insets(0, 0, work_area_bottom_inset, 0));
}

void HomeCardImpl::InstallAccelerators() {
  const AcceleratorData accelerator_data[] = {
      {TRIGGER_ON_PRESS, ui::VKEY_L, ui::EF_CONTROL_DOWN,
       COMMAND_SHOW_HOME_CARD, AF_NONE},
  };
  AcceleratorManager::Get()->RegisterAccelerators(
      accelerator_data, arraysize(accelerator_data), this);
}

void HomeCardImpl::SetState(HomeCard::State state) {
  if (state_ == state)
    return;

  // Update |state_| before changing the visibility of the widgets, so that
  // LayoutManager callbacks get the correct state.
  HomeCard::State old_state = state_;
  state_ = state;
  original_state_ = state;
  if (state_ == HIDDEN) {
    home_card_widget_->Hide();
  } else {
    if (state_ == VISIBLE_CENTERED)
      home_card_widget_->Show();
    else
      home_card_widget_->ShowInactive();
    home_card_view_->SetStateWithAnimation(old_state, state);
    layout_manager_->Layout();
  }
}

HomeCard::State HomeCardImpl::GetState() {
  return state_;
}

void HomeCardImpl::RegisterSearchProvider(
    app_list::SearchProvider* search_provider) {
  DCHECK(!search_provider_);
  search_provider_.reset(search_provider);
  view_delegate_->RegisterSearchProvider(search_provider_.get());
}

void HomeCardImpl::UpdateVirtualKeyboardBounds(
    const gfx::Rect& bounds) {
  if (state_ == VISIBLE_MINIMIZED && !bounds.IsEmpty()) {
    SetState(HIDDEN);
    original_state_ = VISIBLE_MINIMIZED;
  } else if (state_ == VISIBLE_BOTTOM && !bounds.IsEmpty()) {
    SetState(VISIBLE_CENTERED);
    original_state_ = VISIBLE_BOTTOM;
  } else if (state_ != original_state_ && bounds.IsEmpty()) {
    SetState(original_state_);
  }
}

bool HomeCardImpl::OnAcceleratorFired(int command_id,
                                      const ui::Accelerator& accelerator) {
  DCHECK_EQ(COMMAND_SHOW_HOME_CARD, command_id);

  if (state_ == VISIBLE_CENTERED && original_state_ != VISIBLE_BOTTOM)
    SetState(VISIBLE_MINIMIZED);
  else if (state_ == VISIBLE_MINIMIZED)
    SetState(VISIBLE_CENTERED);
  return true;
}

void HomeCardImpl::OnGestureEnded(State final_state) {
  home_card_view_->ClearGesture();
  if (state_ != final_state &&
      (state_ == VISIBLE_MINIMIZED || final_state == VISIBLE_MINIMIZED)) {
    SetState(final_state);
    WindowManager::GetInstance()->ToggleOverview();
  } else {
    HomeCard::State old_state = state_;
    state_ = final_state;
    home_card_view_->SetStateWithAnimation(old_state, final_state);
    layout_manager_->Layout();
  }
}

void HomeCardImpl::OnGestureProgressed(
    State from_state, State to_state, float progress) {
  home_card_view_->SetStateProgress(from_state, to_state, progress);

  gfx::Rect screen_bounds =
      home_card_widget_->GetNativeWindow()->GetRootWindow()->bounds();
  home_card_widget_->SetBounds(gfx::Tween::RectValueBetween(
      progress,
      GetBoundsForState(screen_bounds, from_state),
      GetBoundsForState(screen_bounds, to_state)));

  // TODO(mukai): signals the update to the window manager so that it shows the
  // intermediate visual state of overview mode.
}

void HomeCardImpl::OnOverviewModeEnter() {
  if (state_ == VISIBLE_MINIMIZED)
    SetState(VISIBLE_BOTTOM);
}

void HomeCardImpl::OnOverviewModeExit() {
  SetState(VISIBLE_MINIMIZED);
}

void HomeCardImpl::OnWindowActivated(aura::Window* gained_active,
                                     aura::Window* lost_active) {
  if (state_ != HIDDEN &&
      gained_active != home_card_widget_->GetNativeWindow()) {
    SetState(VISIBLE_MINIMIZED);
  }
}

}  // namespace

// static
HomeCard* HomeCard::Create(AppModelBuilder* model_builder) {
  (new HomeCardImpl(model_builder))->Init();
  DCHECK(instance);
  return instance;
}

// static
void HomeCard::Shutdown() {
  DCHECK(instance);
  delete instance;
  instance = NULL;
}

// static
HomeCard* HomeCard::Get() {
  DCHECK(instance);
  return instance;
}

}  // namespace athena
