// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/signin/account_reconcilor_factory.h"

#include "base/memory/singleton.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "components/signin/core/browser/account_reconcilor.h"
#include "ios/chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "ios/chrome/browser/signin/oauth2_token_service_factory.h"
#include "ios/chrome/browser/signin/signin_client_factory.h"
#include "ios/chrome/browser/signin/signin_manager_factory.h"
#include "ios/public/provider/chrome/browser/browser_state/chrome_browser_state.h"

namespace ios {

AccountReconcilorFactory::AccountReconcilorFactory()
    : BrowserStateKeyedServiceFactory(
          "AccountReconcilor",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(GaiaCookieManagerServiceFactory::GetInstance());
  DependsOn(OAuth2TokenServiceFactory::GetInstance());
  DependsOn(SigninClientFactory::GetInstance());
  DependsOn(SigninManagerFactory::GetInstance());
}

AccountReconcilorFactory::~AccountReconcilorFactory() {}

// static
AccountReconcilor* AccountReconcilorFactory::GetForBrowserState(
    ios::ChromeBrowserState* browser_state) {
  return static_cast<AccountReconcilor*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
AccountReconcilorFactory* AccountReconcilorFactory::GetInstance() {
  return base::Singleton<AccountReconcilorFactory>::get();
}

scoped_ptr<KeyedService> AccountReconcilorFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  ios::ChromeBrowserState* chrome_browser_state =
      ios::ChromeBrowserState::FromBrowserState(context);
  scoped_ptr<AccountReconcilor> reconcilor(new AccountReconcilor(
      OAuth2TokenServiceFactory::GetForBrowserState(chrome_browser_state),
      SigninManagerFactory::GetForBrowserState(chrome_browser_state),
      SigninClientFactory::GetForBrowserState(chrome_browser_state),
      GaiaCookieManagerServiceFactory::GetForBrowserState(
          chrome_browser_state)));
  reconcilor->Initialize(true /* start_reconcile_if_tokens_available */);
  return reconcilor.Pass();
}

}  // namespace ios
