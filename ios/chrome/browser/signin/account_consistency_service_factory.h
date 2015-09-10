// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_ACCOUNT_CONSISTENCY_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_SIGNIN_ACCOUNT_CONSISTENCY_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

class AccountConsistencyService;

namespace ios {

class ChromeBrowserState;

// Singleton that owns the AccountConsistencyService(s) and associates them with
// browser states.
class AccountConsistencyServiceFactory
    : public BrowserStateKeyedServiceFactory {
 public:
  // Returns the instance of AccountConsistencyService associated with this
  // browser state (creating one if none exists). Returns null if this browser
  // state cannot have an AccountConsistencyService (for example, if it is
  // incognito or if WKWebView is not enabled).
  static AccountConsistencyService* GetForBrowserState(
      ios::ChromeBrowserState* browser_state);

  // Returns an instance of the factory singleton.
  static AccountConsistencyServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<AccountConsistencyServiceFactory>;

  AccountConsistencyServiceFactory();
  ~AccountConsistencyServiceFactory() override;

  // BrowserStateKeyedServiceFactory:
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  scoped_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  bool ServiceIsCreatedWithBrowserState() const override;

  DISALLOW_COPY_AND_ASSIGN(AccountConsistencyServiceFactory);
};

}  // namespace ios

#endif  // IOS_CHROME_BROWSER_SIGNIN_ACCOUNT_CONSISTENCY_SERVICE_FACTORY_H_
