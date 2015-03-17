// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_AFFILIATION_BACKEND_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_AFFILIATION_BACKEND_H_

#include <map>
#include <vector>

#include "base/containers/scoped_ptr_hash_map.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "components/password_manager/core/browser/affiliation_fetch_throttler_delegate.h"
#include "components/password_manager/core/browser/affiliation_fetcher_delegate.h"
#include "components/password_manager/core/browser/affiliation_service.h"
#include "components/password_manager/core/browser/affiliation_utils.h"
#include "components/password_manager/core/browser/facet_manager_host.h"

namespace base {
class Clock;
class FilePath;
class SingleThreadTaskRunner;
class ThreadChecker;
class TickClock;
class Time;
}  // namespace base

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace password_manager {

class AffiliationDatabase;
class AffiliationFetcher;
class AffiliationFetchThrottler;
class FacetManager;

// The AffiliationBackend is the part of the AffiliationService that lives on a
// background thread suitable for performing blocking I/O. As most tasks require
// I/O, the backend ends up doing most of the work for the AffiliationService;
// the latter being just a thin layer that delegates most tasks to the backend.
//
// This class is not thread-safe, but it is fine to construct it on one thread
// and then transfer it to the background thread for the rest of its life.
// Initialize() must be called already on the final (background) thread.
class AffiliationBackend : public FacetManagerHost,
                           public AffiliationFetcherDelegate,
                           public AffiliationFetchThrottlerDelegate {
 public:
  using StrategyOnCacheMiss = AffiliationService::StrategyOnCacheMiss;

  // Constructs an instance that will use |request_context_getter| for all
  // network requests, use |task_runner| for asynchronous tasks, and will rely
  // on |time_source| and |time_tick_source| to tell the current time/ticks.
  // Construction is very cheap, expensive steps are deferred to Initialize().
  AffiliationBackend(
      const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      scoped_ptr<base::Clock> time_source,
      scoped_ptr<base::TickClock> time_tick_source);
  ~AffiliationBackend() override;

  // Performs the I/O-heavy part of initialization. The database used to cache
  // affiliation information locally will be opened/created at |db_path|.
  void Initialize(const base::FilePath& db_path);

  // Implementations for methods of the same name in AffiliationService. They
  // are not documented here again. See affiliation_service.h for details:
  void GetAffiliations(
      const FacetURI& facet_uri,
      StrategyOnCacheMiss cache_miss_strategy,
      const AffiliationService::ResultCallback& callback,
      const scoped_refptr<base::TaskRunner>& callback_task_runner);
  void Prefetch(const FacetURI& facet_uri, const base::Time& keep_fresh_until);
  void CancelPrefetch(const FacetURI& facet_uri,
                      const base::Time& keep_fresh_until);
  void TrimCache();

 private:
  friend class AffiliationBackendTest;

  // Retrieves the FacetManager corresponding to |facet_uri|, creating it and
  // storing it into |facet_managers_| if it did not exist.
  FacetManager* GetOrCreateFacetManager(const FacetURI& facet_uri);

  // Scheduled by RequestNotificationAtTime() to be called back at times when a
  // FacetManager needs to be notified.
  void OnSendNotification(const FacetURI& facet_uri);

  // FacetManagerHost:
  bool ReadAffiliationsFromDatabase(
      const FacetURI& facet_uri,
      AffiliatedFacetsWithUpdateTime* affiliations) override;
  void SignalNeedNetworkRequest() override;
  void RequestNotificationAtTime(const FacetURI& facet_uri,
                                 base::Time time) override;

  // AffiliationFetcherDelegate:
  void OnFetchSucceeded(
      scoped_ptr<AffiliationFetcherDelegate::Result> result) override;
  void OnFetchFailed() override;
  void OnMalformedResponse() override;

  // AffiliationFetchThrottlerDelegate:
  bool OnCanSendNetworkRequest() override;

  // Returns the number of in-memory FacetManagers. Used only for testing.
  size_t facet_manager_count_for_testing() { return facet_managers_.size(); }

  // To be called after Initialize() to use |throttler| instead of the default
  // one. Used only for testing.
  void SetThrottlerForTesting(scoped_ptr<AffiliationFetchThrottler> throttler);

  // Created in Initialize(), and ensures that all subsequent methods are called
  // on the same thread.
  scoped_ptr<base::ThreadChecker> thread_checker_;

  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  scoped_ptr<base::Clock> clock_;
  scoped_ptr<base::TickClock> tick_clock_;

  scoped_ptr<AffiliationDatabase> cache_;
  scoped_ptr<AffiliationFetcher> fetcher_;
  scoped_ptr<AffiliationFetchThrottler> throttler_;

  // Contains a FacetManager for each facet URI that need ongoing attention. To
  // save memory, managers are discarded as soon as they become redundant.
  base::ScopedPtrHashMap<FacetURI, FacetManager> facet_managers_;

  base::WeakPtrFactory<AffiliationBackend> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AffiliationBackend);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_AFFILIATION_BACKEND_H_
