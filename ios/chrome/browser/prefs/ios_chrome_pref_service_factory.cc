// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/prefs/ios_chrome_pref_service_factory.h"

#include <vector>

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/prefs/json_pref_store.h"
#include "base/prefs/persistent_pref_store.h"
#include "base/prefs/pref_filter.h"
#include "base/prefs/pref_service.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "components/search_engines/default_search_pref_migration.h"
#include "components/syncable_prefs/pref_service_syncable.h"
#include "components/syncable_prefs/pref_service_syncable_factory.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/prefs/ios_chrome_pref_model_associator_client.h"

namespace {

const char kPreferencesFilename[] = "Preferences";

// Record PersistentPrefStore's reading errors distribution.
void HandleReadError(PersistentPrefStore::PrefReadError error) {
  // Sample the histogram also for the successful case in order to get a
  // baseline on the success rate in addition to the error distribution.
  UMA_HISTOGRAM_ENUMERATION("PrefService.ReadError", error,
                            PersistentPrefStore::PREF_READ_ERROR_MAX_ENUM);
}

void PrepareFactory(PrefServiceSyncableFactory* factory,
                    policy::PolicyService* policy_service,
                    const base::FilePath& pref_filename,
                    base::SequencedTaskRunner* pref_io_task_runner,
                    bool async) {
  factory->set_user_prefs(make_scoped_refptr(new JsonPrefStore(
      pref_filename, pref_io_task_runner, scoped_ptr<PrefFilter>())));

#if defined(ENABLE_CONFIGURATION_POLICY)
  policy::BrowserPolicyConnector* policy_connector =
      GetApplicationContext()->GetBrowserPolicyConnector();
  factory->SetManagedPolicies(policy_service, policy_connector);
  factory->SetRecommendedPolicies(policy_service, policy_connector);
#endif  // ENABLE_CONFIGURATION_POLICY

  factory->set_async(async);
  factory->set_read_error_callback(base::Bind(&HandleReadError));
  factory->SetPrefModelAssociatorClient(
      IOSChromePrefModelAssociatorClient::GetInstance());
}

}  // namespace

scoped_ptr<PrefService> CreateLocalState(
    const base::FilePath& pref_filename,
    base::SequencedTaskRunner* pref_io_task_runner,
    policy::PolicyService* policy_service,
    const scoped_refptr<PrefRegistry>& pref_registry,
    bool async) {
  PrefServiceSyncableFactory factory;
  PrepareFactory(&factory, policy_service, pref_filename, pref_io_task_runner,
                 async);
  return factory.Create(pref_registry.get());
}

scoped_ptr<PrefServiceSyncable> CreateBrowserStatePrefs(
    const base::FilePath& browser_state_path,
    base::SequencedTaskRunner* pref_io_task_runner,
    TrackedPreferenceValidationDelegate* validation_delegate,
    policy::PolicyService* policy_service,
    const scoped_refptr<user_prefs::PrefRegistrySyncable>& pref_registry,
    bool async) {
  // chrome_prefs::CreateProfilePrefs uses ProfilePrefStoreManager to create
  // the preference store however since Chrome on iOS does not need to track
  // preference modifications (as applications are sand-boxed), it can use a
  // simple JsonPrefStore to store them (which is what PrefStoreManager uses
  // on platforms that do not track preference modifications).
  PrefServiceSyncableFactory factory;
  PrepareFactory(&factory, policy_service,
                 browser_state_path.Append(kPreferencesFilename),
                 pref_io_task_runner, async);
  scoped_ptr<PrefServiceSyncable> pref_service =
      factory.CreateSyncable(pref_registry.get());
  ConfigureDefaultSearchPrefMigrationToDictionaryValue(pref_service.get());
  return pref_service.Pass();
}

scoped_ptr<PrefServiceSyncable> CreateIncognitoBrowserStatePrefs(
    PrefServiceSyncable* pref_service) {
  // List of keys that cannot be changed in the user prefs file by the incognito
  // browser state. All preferences that store information about the browsing
  // history or behaviour of the user should have this property.
  std::vector<const char*> overlay_pref_names;
  overlay_pref_names.push_back(proxy_config::prefs::kProxy);
  return make_scoped_ptr(pref_service->CreateIncognitoPrefService(
      nullptr,  // incognito_extension_pref_store
      overlay_pref_names));
}
