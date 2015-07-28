// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/child_account_info_fetcher.h"

#include "build/build_config.h"

#if defined(OS_ANDROID)
#include "components/signin/core/browser/child_account_info_fetcher_android.h"
#else
#include "components/signin/core/browser/child_account_info_fetcher_impl.h"
#endif

// static
ChildAccountInfoFetcher* ChildAccountInfoFetcher::CreateFrom(
    const std::string& account_id,
    AccountFetcherService* fetcher_service,
    OAuth2TokenService* token_service,
    net::URLRequestContextGetter* request_context_getter,
    invalidation::InvalidationService* invalidation_service) {
#if defined(OS_ANDROID)
  ChildAccountInfoFetcherAndroid::StartFetchingChildAccountInfo(fetcher_service,
                                                                account_id);
  return nullptr;
#else
  return new ChildAccountInfoFetcherImpl(account_id, fetcher_service,
                                         token_service, request_context_getter,
                                         invalidation_service);
#endif
}

ChildAccountInfoFetcher::~ChildAccountInfoFetcher() {
}
