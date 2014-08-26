// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_file_value_serializer.h"
#include "base/memory/scoped_ptr.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/histogram_samples.h"
#include "base/metrics/statistics_recorder.h"
#include "base/path_service.h"
#include "base/prefs/pref_service.h"
#include "base/prefs/scoped_user_pref_update.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/prefs/chrome_pref_service_factory.h"
#include "chrome/browser/prefs/profile_pref_store_manager.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/search_engines/default_search_manager.h"
#include "content/public/common/content_switches.h"
#include "extensions/browser/pref_names.h"
#include "extensions/common/extension.h"

#if defined(OS_CHROMEOS)
#include "chromeos/chromeos_switches.h"
#endif

namespace {

// Extension ID of chrome/test/data/extensions/good.crx
const char kGoodCrxId[] = "ldnnhddmnhbkjipkidpdiheffobcpfmf";

// Explicit expectations from the caller of GetTrackedPrefHistogramCount(). This
// enables detailed reporting of the culprit on failure.
enum AllowedBuckets {
  // Allow no samples in any buckets.
  ALLOW_NONE = -1,
  // Any integer between BEGIN_ALLOW_SINGLE_BUCKET and END_ALLOW_SINGLE_BUCKET
  // indicates that only this specific bucket is allowed to have a sample.
  BEGIN_ALLOW_SINGLE_BUCKET = 0,
  END_ALLOW_SINGLE_BUCKET = 100,
  // Allow any buckets (no extra verifications performed).
  ALLOW_ANY
};

// Returns the number of times |histogram_name| was reported so far; adding the
// results of the first 100 buckets (there are only ~19 reporting IDs as of this
// writing; varies depending on the platform). |allowed_buckets| hints at extra
// requirements verified in this method (see AllowedBuckets for details).
int GetTrackedPrefHistogramCount(const char* histogram_name,
                                 int allowed_buckets) {
  const base::HistogramBase* histogram =
      base::StatisticsRecorder::FindHistogram(histogram_name);
  if (!histogram)
    return 0;

  scoped_ptr<base::HistogramSamples> samples(histogram->SnapshotSamples());
  int sum = 0;
  for (int i = 0; i < 100; ++i) {
    int count_for_id = samples->GetCount(i);
    EXPECT_GE(count_for_id, 0);
    sum += count_for_id;

    if (allowed_buckets == ALLOW_NONE ||
        (allowed_buckets != ALLOW_ANY && i != allowed_buckets)) {
      EXPECT_EQ(0, count_for_id) << "Unexpected reporting_id: " << i;
    }
  }
  return sum;
}

scoped_ptr<base::DictionaryValue> ReadPrefsDictionary(
    const base::FilePath& pref_file) {
  JSONFileValueSerializer serializer(pref_file);
  int error_code = JSONFileValueSerializer::JSON_NO_ERROR;
  std::string error_str;
  scoped_ptr<base::Value> prefs(
      serializer.Deserialize(&error_code, &error_str));
  if (!prefs || error_code != JSONFileValueSerializer::JSON_NO_ERROR) {
    ADD_FAILURE() << "Error #" << error_code << ": " << error_str;
    return scoped_ptr<base::DictionaryValue>();
  }
  if (!prefs->IsType(base::Value::TYPE_DICTIONARY)) {
    ADD_FAILURE();
    return scoped_ptr<base::DictionaryValue>();
  }
  return scoped_ptr<base::DictionaryValue>(
      static_cast<base::DictionaryValue*>(prefs.release()));
}

#define PREF_HASH_BROWSER_TEST(fixture, test_name)                          \
  IN_PROC_BROWSER_TEST_P(fixture, PRE_##test_name) {                        \
    SetupPreferences();                                                     \
  }                                                                         \
  IN_PROC_BROWSER_TEST_P(fixture, test_name) {                              \
    VerifyReactionToPrefAttack();                                           \
  }                                                                         \
  INSTANTIATE_TEST_CASE_P(                                                  \
      fixture##Instance,                                                    \
      fixture,                                                              \
      testing::Values(                                                      \
          chrome_prefs::internals::kSettingsEnforcementGroupNoEnforcement,  \
          chrome_prefs::internals::kSettingsEnforcementGroupEnforceAlways,  \
          chrome_prefs::internals::                                         \
              kSettingsEnforcementGroupEnforceAlwaysWithDSE,                \
          chrome_prefs::internals::                                         \
              kSettingsEnforcementGroupEnforceAlwaysWithExtensionsAndDSE));

// A base fixture designed such that implementations do two things:
//  1) Override all three pure-virtual methods below to setup, attack, and
//     verify preferenes throughout the tests provided by this fixture.
//  2) Instantiate their test via the PREF_HASH_BROWSER_TEST macro above.
// Based on top of ExtensionBrowserTest to allow easy interaction with the
// ExtensionService.
class PrefHashBrowserTestBase
    : public ExtensionBrowserTest,
      public testing::WithParamInterface<std::string> {
 public:
  // List of potential protection levels for this test in strict increasing
  // order of protection levels.
  enum SettingsProtectionLevel {
    PROTECTION_DISABLED_ON_PLATFORM,
    PROTECTION_DISABLED_FOR_GROUP,
    PROTECTION_ENABLED_BASIC,
    PROTECTION_ENABLED_DSE,
    PROTECTION_ENABLED_EXTENSIONS,
    // Represents the strongest level (i.e. always equivalent to the last one in
    // terms of protection), leave this one last when adding new levels.
    PROTECTION_ENABLED_ALL
  };

  PrefHashBrowserTestBase()
      : protection_level_(GetProtectionLevelFromTrialGroup(GetParam())) {
  }

  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    ExtensionBrowserTest::SetUpCommandLine(command_line);
    EXPECT_FALSE(command_line->HasSwitch(switches::kForceFieldTrials));
    command_line->AppendSwitchASCII(
        switches::kForceFieldTrials,
        std::string(chrome_prefs::internals::kSettingsEnforcementTrialName) +
            "/" + GetParam() + "/");
#if defined(OS_CHROMEOS)
    command_line->AppendSwitch(
        chromeos::switches::kIgnoreUserProfileMappingForTests);
#endif
  }

  virtual bool SetUpUserDataDirectory() OVERRIDE {
    // Do the normal setup in the PRE test and attack preferences in the main
    // test.
    if (IsPRETest())
      return ExtensionBrowserTest::SetUpUserDataDirectory();

#if defined(OS_CHROMEOS)
    // For some reason, the Preferences file does not exist in the location
    // below on Chrome OS. Since protection is disabled on Chrome OS, it's okay
    // to simply not attack preferences at all (and still assert that no
    // hardening related histogram kicked in in VerifyReactionToPrefAttack()).
    // TODO(gab): Figure out why there is no Preferences file in this location
    // on Chrome OS (and re-enable the section disabled for OS_CHROMEOS further
    // below).
    EXPECT_EQ(PROTECTION_DISABLED_ON_PLATFORM, protection_level_);
    return true;
#endif

    base::FilePath profile_dir;
    EXPECT_TRUE(PathService::Get(chrome::DIR_USER_DATA, &profile_dir));
    profile_dir = profile_dir.AppendASCII(TestingProfile::kTestUserProfileDir);

    // Sanity check that old protected pref file is never present in modern
    // Chromes.
    EXPECT_FALSE(base::PathExists(
        profile_dir.Append(chrome::kProtectedPreferencesFilenameDeprecated)));

    // Read the preferences from disk.

    const base::FilePath unprotected_pref_file =
        profile_dir.Append(chrome::kPreferencesFilename);
    EXPECT_TRUE(base::PathExists(unprotected_pref_file));

    const base::FilePath protected_pref_file =
        profile_dir.Append(chrome::kSecurePreferencesFilename);
    EXPECT_EQ(protection_level_ > PROTECTION_DISABLED_ON_PLATFORM,
              base::PathExists(protected_pref_file));

    scoped_ptr<base::DictionaryValue> unprotected_preferences(
        ReadPrefsDictionary(unprotected_pref_file));
    if (!unprotected_preferences)
      return false;

    scoped_ptr<base::DictionaryValue> protected_preferences;
    if (protection_level_ > PROTECTION_DISABLED_ON_PLATFORM) {
      protected_preferences = ReadPrefsDictionary(protected_pref_file);
      if (!protected_preferences)
        return false;
    }

    // Let the underlying test modify the preferences.
    AttackPreferencesOnDisk(unprotected_preferences.get(),
                            protected_preferences.get());

    // Write the modified preferences back to disk.

    JSONFileValueSerializer unprotected_prefs_serializer(unprotected_pref_file);
    EXPECT_TRUE(
        unprotected_prefs_serializer.Serialize(*unprotected_preferences));

    if (protected_preferences) {
      JSONFileValueSerializer protected_prefs_serializer(protected_pref_file);
      EXPECT_TRUE(protected_prefs_serializer.Serialize(*protected_preferences));
    }

    return true;
  }

  virtual void SetUpInProcessBrowserTestFixture() OVERRIDE {
    ExtensionBrowserTest::SetUpInProcessBrowserTestFixture();

    // Bots are on a domain, turn off the domain check for settings hardening in
    // order to be able to test all SettingsEnforcement groups.
    chrome_prefs::DisableDelaysAndDomainCheckForTesting();
  }

  // In the PRE_ test, find the number of tracked preferences that were
  // initialized and save it to a file to be read back in the main test and used
  // as the total number of tracked preferences.
  virtual void SetUpOnMainThread() OVERRIDE {
    ExtensionBrowserTest::SetUpOnMainThread();

    // File in which the PRE_ test will save the number of tracked preferences
    // on this platform.
    const char kNumTrackedPrefFilename[] = "NumTrackedPrefs";

    base::FilePath num_tracked_prefs_file;
    ASSERT_TRUE(
        PathService::Get(chrome::DIR_USER_DATA, &num_tracked_prefs_file));
    num_tracked_prefs_file =
        num_tracked_prefs_file.AppendASCII(kNumTrackedPrefFilename);

    if (IsPRETest()) {
      num_tracked_prefs_ = GetTrackedPrefHistogramCount(
          "Settings.TrackedPreferenceTrustedInitialized", ALLOW_ANY);
      EXPECT_EQ(protection_level_ > PROTECTION_DISABLED_ON_PLATFORM,
                num_tracked_prefs_ > 0);

      // Split tracked prefs are reported as Unchanged not as TrustedInitialized
      // when an empty dictionary is encountered on first run (this should only
      // hit for pref #5 in the current design).
      int num_split_tracked_prefs = GetTrackedPrefHistogramCount(
          "Settings.TrackedPreferenceUnchanged", BEGIN_ALLOW_SINGLE_BUCKET + 5);
      EXPECT_EQ(protection_level_ > PROTECTION_DISABLED_ON_PLATFORM ? 1 : 0,
                num_split_tracked_prefs);

      num_tracked_prefs_ += num_split_tracked_prefs;

      std::string num_tracked_prefs_str = base::IntToString(num_tracked_prefs_);
      EXPECT_EQ(static_cast<int>(num_tracked_prefs_str.size()),
                base::WriteFile(num_tracked_prefs_file,
                                num_tracked_prefs_str.c_str(),
                                num_tracked_prefs_str.size()));
    } else {
      std::string num_tracked_prefs_str;
      EXPECT_TRUE(base::ReadFileToString(num_tracked_prefs_file,
                                         &num_tracked_prefs_str));
      EXPECT_TRUE(
          base::StringToInt(num_tracked_prefs_str, &num_tracked_prefs_));
    }
  }

 protected:
  // Called from the PRE_ test's body. Overrides should use it to setup
  // preferences through Chrome.
  virtual void SetupPreferences() = 0;

  // Called prior to the main test launching its browser. Overrides should use
  // it to attack preferences. |(un)protected_preferences| represent the state
  // on disk prior to launching the main test, they can be modified by this
  // method and modifications will be flushed back to disk before launching the
  // main test. |unprotected_preferences| is never NULL, |protected_preferences|
  // may be NULL if in PROTECTION_DISABLED_ON_PLATFORM mode.
  virtual void AttackPreferencesOnDisk(
      base::DictionaryValue* unprotected_preferences,
      base::DictionaryValue* protected_preferences) = 0;

  // Called from the body of the main test. Overrides should use it to verify
  // that the browser had the desired reaction when faced when the attack
  // orchestrated in AttackPreferencesOnDisk().
  virtual void VerifyReactionToPrefAttack() = 0;

  int num_tracked_prefs() const { return num_tracked_prefs_; }

  const SettingsProtectionLevel protection_level_;

 private:
  // Returns true if this is the PRE_ phase of the test.
  bool IsPRETest() {
    return StartsWithASCII(
        testing::UnitTest::GetInstance()->current_test_info()->name(),
        "PRE_",
        true /* case_sensitive */);
  }

  SettingsProtectionLevel GetProtectionLevelFromTrialGroup(
      const std::string& trial_group) {
    if (!ProfilePrefStoreManager::kPlatformSupportsPreferenceTracking)
      return PROTECTION_DISABLED_ON_PLATFORM;

// Protection levels can't be adjusted via --force-fieldtrials in official
// builds.
#if defined(OFFICIAL_BUILD)

#if defined(OS_WIN)
    // The strongest mode is enforced on Windows in the absence of a field
    // trial.
    return PROTECTION_ENABLED_ALL;
#else
    return PROTECTION_DISABLED_FOR_GROUP;
#endif

#else  // defined(OFFICIAL_BUILD)

    using namespace chrome_prefs::internals;
    if (trial_group == kSettingsEnforcementGroupNoEnforcement) {
      return PROTECTION_DISABLED_FOR_GROUP;
    } else if (trial_group == kSettingsEnforcementGroupEnforceAlways) {
      return PROTECTION_ENABLED_BASIC;
    } else if (trial_group == kSettingsEnforcementGroupEnforceAlwaysWithDSE) {
      return PROTECTION_ENABLED_DSE;
    } else if (trial_group ==
               kSettingsEnforcementGroupEnforceAlwaysWithExtensionsAndDSE) {
      return PROTECTION_ENABLED_EXTENSIONS;
    } else {
      ADD_FAILURE();
      return static_cast<SettingsProtectionLevel>(-1);
    }

#endif  // defined(OFFICIAL_BUILD)

  }

  int num_tracked_prefs_;
};

}  // namespace

// Verifies that nothing is reset when nothing is tampered with.
// Also sanity checks that the expected preferences files are in place.
class PrefHashBrowserTestUnchangedDefault : public PrefHashBrowserTestBase {
 public:
  virtual void SetupPreferences() OVERRIDE {
    // Default Chrome setup.
  }

  virtual void AttackPreferencesOnDisk(
      base::DictionaryValue* unprotected_preferences,
      base::DictionaryValue* protected_preferences) OVERRIDE {
    // No attack.
  }

  virtual void VerifyReactionToPrefAttack() OVERRIDE {
    // Expect all prefs to be reported as Unchanged with no resets.
    EXPECT_EQ(protection_level_ > PROTECTION_DISABLED_ON_PLATFORM
                  ? num_tracked_prefs() : 0,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceUnchanged", ALLOW_ANY));
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceWantedReset", ALLOW_NONE));
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount("Settings.TrackedPreferenceReset",
                                           ALLOW_NONE));

    // Nothing else should have triggered.
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount("Settings.TrackedPreferenceChanged",
                                           ALLOW_NONE));
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount("Settings.TrackedPreferenceCleared",
                                           ALLOW_NONE));
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceInitialized", ALLOW_NONE));
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceTrustedInitialized", ALLOW_NONE));
    EXPECT_EQ(
        0,
        GetTrackedPrefHistogramCount(
            "Settings.TrackedPreferenceMigratedLegacyDeviceId", ALLOW_NONE));
  }
};

PREF_HASH_BROWSER_TEST(PrefHashBrowserTestUnchangedDefault, UnchangedDefault);

// Augments PrefHashBrowserTestUnchangedDefault to confirm that nothing is reset
// when nothing is tampered with, even if Chrome itself wrote custom prefs in
// its last run.
class PrefHashBrowserTestUnchangedCustom
    : public PrefHashBrowserTestUnchangedDefault {
 public:
  virtual void SetupPreferences() OVERRIDE {
    profile()->GetPrefs()->SetString(prefs::kHomePage, "http://example.com");

    InstallExtensionWithUIAutoConfirm(
        test_data_dir_.AppendASCII("good.crx"), 1, browser());
  }

  virtual void VerifyReactionToPrefAttack() OVERRIDE {
    // Make sure the settings written in the last run stuck.
    EXPECT_EQ("http://example.com",
              profile()->GetPrefs()->GetString(prefs::kHomePage));

    EXPECT_TRUE(extension_service()->GetExtensionById(kGoodCrxId, false));

    // Reaction should be identical to unattacked default prefs.
    PrefHashBrowserTestUnchangedDefault::VerifyReactionToPrefAttack();
  }
};

PREF_HASH_BROWSER_TEST(PrefHashBrowserTestUnchangedCustom, UnchangedCustom);

// Verifies that cleared prefs are reported.
class PrefHashBrowserTestClearedAtomic : public PrefHashBrowserTestBase {
 public:
  virtual void SetupPreferences() OVERRIDE {
    profile()->GetPrefs()->SetString(prefs::kHomePage, "http://example.com");
  }

  virtual void AttackPreferencesOnDisk(
      base::DictionaryValue* unprotected_preferences,
      base::DictionaryValue* protected_preferences) OVERRIDE {
    base::DictionaryValue* selected_prefs =
        protection_level_ >= PROTECTION_ENABLED_BASIC ? protected_preferences
                                                      : unprotected_preferences;
    // |selected_prefs| should never be NULL under the protection level picking
    // it.
    EXPECT_TRUE(selected_prefs);
    EXPECT_TRUE(selected_prefs->Remove(prefs::kHomePage, NULL));
  }

  virtual void VerifyReactionToPrefAttack() OVERRIDE {
    // The clearance of homepage should have been noticed (as pref #2 being
    // cleared), but shouldn't have triggered a reset (as there is nothing we
    // can do when the pref is already gone).
    EXPECT_EQ(protection_level_ > PROTECTION_DISABLED_ON_PLATFORM ? 1 : 0,
              GetTrackedPrefHistogramCount("Settings.TrackedPreferenceCleared",
                                           BEGIN_ALLOW_SINGLE_BUCKET + 2));
    EXPECT_EQ(protection_level_ > PROTECTION_DISABLED_ON_PLATFORM
                  ? num_tracked_prefs() - 1 : 0,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceUnchanged", ALLOW_ANY));
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceWantedReset", ALLOW_NONE));
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount("Settings.TrackedPreferenceReset",
                                           ALLOW_NONE));

    // Nothing else should have triggered.
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount("Settings.TrackedPreferenceChanged",
                                           ALLOW_NONE));
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceInitialized", ALLOW_NONE));
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceTrustedInitialized", ALLOW_NONE));
    EXPECT_EQ(
        0,
        GetTrackedPrefHistogramCount(
            "Settings.TrackedPreferenceMigratedLegacyDeviceId", ALLOW_NONE));
  }
};

PREF_HASH_BROWSER_TEST(PrefHashBrowserTestClearedAtomic, ClearedAtomic);

// Verifies that clearing the MACs results in untrusted Initialized pings for
// non-null protected prefs.
class PrefHashBrowserTestUntrustedInitialized : public PrefHashBrowserTestBase {
 public:
  virtual void SetupPreferences() OVERRIDE {
    // Explicitly set the DSE (it's otherwise NULL by default, preventing
    // thorough testing of the PROTECTION_ENABLED_DSE level).
    DefaultSearchManager default_search_manager(
        profile()->GetPrefs(), DefaultSearchManager::ObserverCallback());
    DefaultSearchManager::Source dse_source =
        static_cast<DefaultSearchManager::Source>(-1);

    const TemplateURLData* default_template_url_data =
        default_search_manager.GetDefaultSearchEngine(&dse_source);
    EXPECT_EQ(DefaultSearchManager::FROM_FALLBACK, dse_source);

    default_search_manager.SetUserSelectedDefaultSearchEngine(
        *default_template_url_data);

    default_search_manager.GetDefaultSearchEngine(&dse_source);
    EXPECT_EQ(DefaultSearchManager::FROM_USER, dse_source);

    // Also explicitly set an atomic pref that falls under
    // PROTECTION_ENABLED_BASIC.
    profile()->GetPrefs()->SetInteger(prefs::kRestoreOnStartup,
                                      SessionStartupPref::URLS);
  }

  virtual void AttackPreferencesOnDisk(
      base::DictionaryValue* unprotected_preferences,
      base::DictionaryValue* protected_preferences) OVERRIDE {
    EXPECT_TRUE(unprotected_preferences->Remove("protection.macs", NULL));
    if (protected_preferences)
      EXPECT_TRUE(protected_preferences->Remove("protection.macs", NULL));
  }

  virtual void VerifyReactionToPrefAttack() OVERRIDE {
    // Preferences that are NULL by default will be TrustedInitialized.
    int num_null_values = GetTrackedPrefHistogramCount(
        "Settings.TrackedPreferenceTrustedInitialized", ALLOW_ANY);
    EXPECT_EQ(protection_level_ > PROTECTION_DISABLED_ON_PLATFORM,
              num_null_values > 0);
    if (num_null_values > 0) {
      // This test requires that at least 3 prefs be non-null (extensions, DSE,
      // and 1 atomic pref explictly set for this test above).
      EXPECT_LT(num_null_values, num_tracked_prefs() - 3);
    }

    // Expect all non-null prefs to be reported as Initialized (with
    // accompanying resets or wanted resets based on the current protection
    // level).
    EXPECT_EQ(num_tracked_prefs() - num_null_values,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceInitialized", ALLOW_ANY));

    int num_protected_prefs = 0;
    // A switch statement falling through each protection level in decreasing
    // levels of protection to add expectations for each level which augments
    // the previous one.
    switch (protection_level_) {
      case PROTECTION_ENABLED_ALL:
        // Falls through.
      case PROTECTION_ENABLED_EXTENSIONS:
        ++num_protected_prefs;
        // Falls through.
      case PROTECTION_ENABLED_DSE:
        ++num_protected_prefs;
        // Falls through.
      case PROTECTION_ENABLED_BASIC:
        num_protected_prefs += num_tracked_prefs() - num_null_values - 2;
        // Falls through.
      case PROTECTION_DISABLED_FOR_GROUP:
        // No protection. Falls through.
      case PROTECTION_DISABLED_ON_PLATFORM:
        // No protection.
        break;
    }

    EXPECT_EQ(num_tracked_prefs() - num_null_values - num_protected_prefs,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceWantedReset", ALLOW_ANY));
    EXPECT_EQ(num_protected_prefs,
              GetTrackedPrefHistogramCount("Settings.TrackedPreferenceReset",
                                           ALLOW_ANY));

    // Explicitly verify the result of reported resets.

    DefaultSearchManager default_search_manager(
        profile()->GetPrefs(), DefaultSearchManager::ObserverCallback());
    DefaultSearchManager::Source dse_source =
        static_cast<DefaultSearchManager::Source>(-1);
    default_search_manager.GetDefaultSearchEngine(&dse_source);
    EXPECT_EQ(protection_level_ < PROTECTION_ENABLED_DSE
                  ? DefaultSearchManager::FROM_USER
                  : DefaultSearchManager::FROM_FALLBACK,
              dse_source);

    EXPECT_EQ(protection_level_ < PROTECTION_ENABLED_BASIC,
              profile()->GetPrefs()->GetInteger(prefs::kRestoreOnStartup) ==
                  SessionStartupPref::URLS);

    // Nothing else should have triggered.
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceUnchanged", ALLOW_NONE));
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount("Settings.TrackedPreferenceChanged",
                                           ALLOW_NONE));
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount("Settings.TrackedPreferenceCleared",
                                           ALLOW_NONE));
    EXPECT_EQ(
        0,
        GetTrackedPrefHistogramCount(
            "Settings.TrackedPreferenceMigratedLegacyDeviceId", ALLOW_NONE));
  }
};

PREF_HASH_BROWSER_TEST(PrefHashBrowserTestUntrustedInitialized,
                       UntrustedInitialized);

// Verifies that changing an atomic pref results in it being reported (and reset
// if the protection level allows it).
class PrefHashBrowserTestChangedAtomic : public PrefHashBrowserTestBase {
 public:
  virtual void SetupPreferences() OVERRIDE {
    profile()->GetPrefs()->SetInteger(prefs::kRestoreOnStartup,
                                      SessionStartupPref::URLS);

    ListPrefUpdate update(profile()->GetPrefs(),
                          prefs::kURLsToRestoreOnStartup);
    update->AppendString("http://example.com");
  }

  virtual void AttackPreferencesOnDisk(
      base::DictionaryValue* unprotected_preferences,
      base::DictionaryValue* protected_preferences) OVERRIDE {
    base::DictionaryValue* selected_prefs =
        protection_level_ >= PROTECTION_ENABLED_BASIC ? protected_preferences
                                                      : unprotected_preferences;
    // |selected_prefs| should never be NULL under the protection level picking
    // it.
    EXPECT_TRUE(selected_prefs);
    base::ListValue* startup_urls;
    EXPECT_TRUE(
        selected_prefs->GetList(prefs::kURLsToRestoreOnStartup, &startup_urls));
    EXPECT_TRUE(startup_urls);
    EXPECT_EQ(1U, startup_urls->GetSize());
    startup_urls->AppendString("http://example.org");
  }

  virtual void VerifyReactionToPrefAttack() OVERRIDE {
    // Expect a single Changed event for tracked pref #4 (startup URLs).
    EXPECT_EQ(protection_level_ > PROTECTION_DISABLED_ON_PLATFORM ? 1 : 0,
              GetTrackedPrefHistogramCount("Settings.TrackedPreferenceChanged",
                                           BEGIN_ALLOW_SINGLE_BUCKET + 4));
    EXPECT_EQ(protection_level_ > PROTECTION_DISABLED_ON_PLATFORM
                  ? num_tracked_prefs() - 1 : 0,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceUnchanged", ALLOW_ANY));

    EXPECT_EQ(
        (protection_level_ > PROTECTION_DISABLED_ON_PLATFORM &&
         protection_level_ < PROTECTION_ENABLED_BASIC) ? 1 : 0,
        GetTrackedPrefHistogramCount("Settings.TrackedPreferenceWantedReset",
                                     BEGIN_ALLOW_SINGLE_BUCKET + 4));
    EXPECT_EQ(protection_level_ >= PROTECTION_ENABLED_BASIC ? 1 : 0,
              GetTrackedPrefHistogramCount("Settings.TrackedPreferenceReset",
                                           BEGIN_ALLOW_SINGLE_BUCKET + 4));

// TODO(gab): This doesn't work on OS_CHROMEOS because we fail to attack
// Preferences.
#if !defined(OS_CHROMEOS)
    // Explicitly verify the result of reported resets.
    EXPECT_EQ(protection_level_ >= PROTECTION_ENABLED_BASIC ? 0U : 2U,
              profile()
                  ->GetPrefs()
                  ->GetList(prefs::kURLsToRestoreOnStartup)
                  ->GetSize());
#endif

    // Nothing else should have triggered.
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount("Settings.TrackedPreferenceCleared",
                                           ALLOW_NONE));
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceInitialized", ALLOW_NONE));
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceTrustedInitialized", ALLOW_NONE));
    EXPECT_EQ(
        0,
        GetTrackedPrefHistogramCount(
            "Settings.TrackedPreferenceMigratedLegacyDeviceId", ALLOW_NONE));
  }
};

PREF_HASH_BROWSER_TEST(PrefHashBrowserTestChangedAtomic, ChangedAtomic);

// Verifies that changing or adding an entry in a split pref results in both
// items being reported (and remove if the protection level allows it).
class PrefHashBrowserTestChangedSplitPref : public PrefHashBrowserTestBase {
 public:
  virtual void SetupPreferences() OVERRIDE {
    InstallExtensionWithUIAutoConfirm(
        test_data_dir_.AppendASCII("good.crx"), 1, browser());
  }

  virtual void AttackPreferencesOnDisk(
      base::DictionaryValue* unprotected_preferences,
      base::DictionaryValue* protected_preferences) OVERRIDE {
    base::DictionaryValue* selected_prefs =
        protection_level_ >= PROTECTION_ENABLED_EXTENSIONS
            ? protected_preferences
            : unprotected_preferences;
    // |selected_prefs| should never be NULL under the protection level picking
    // it.
    EXPECT_TRUE(selected_prefs);
    base::DictionaryValue* extensions_dict;
    EXPECT_TRUE(selected_prefs->GetDictionary(
        extensions::pref_names::kExtensions, &extensions_dict));
    EXPECT_TRUE(extensions_dict);

    // Tamper with any installed setting for good.crx
    base::DictionaryValue* good_crx_dict;
    EXPECT_TRUE(extensions_dict->GetDictionary(kGoodCrxId, &good_crx_dict));
    int good_crx_state;
    EXPECT_TRUE(good_crx_dict->GetInteger("state", &good_crx_state));
    EXPECT_EQ(extensions::Extension::ENABLED, good_crx_state);
    good_crx_dict->SetInteger("state", extensions::Extension::DISABLED);

    // Drop a fake extension (for the purpose of this test, dropped settings
    // don't need to be valid extension settings).
    base::DictionaryValue* fake_extension = new base::DictionaryValue;
    fake_extension->SetString("name", "foo");
    extensions_dict->Set(std::string(32, 'a'), fake_extension);
  }

  virtual void VerifyReactionToPrefAttack() OVERRIDE {
    // Expect a single split pref changed report with a count of 2 for tracked
    // pref #5 (extensions).
    EXPECT_EQ(protection_level_ > PROTECTION_DISABLED_ON_PLATFORM ? 1 : 0,
              GetTrackedPrefHistogramCount("Settings.TrackedPreferenceChanged",
                                           BEGIN_ALLOW_SINGLE_BUCKET + 5));
    EXPECT_EQ(protection_level_ > PROTECTION_DISABLED_ON_PLATFORM ? 1 : 0,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedSplitPreferenceChanged.extensions.settings",
                  BEGIN_ALLOW_SINGLE_BUCKET + 2));

    // Everything else should have remained unchanged.
    EXPECT_EQ(protection_level_ > PROTECTION_DISABLED_ON_PLATFORM
                  ? num_tracked_prefs() - 1 : 0,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceUnchanged", ALLOW_ANY));

    EXPECT_EQ(
        (protection_level_ > PROTECTION_DISABLED_ON_PLATFORM &&
         protection_level_ < PROTECTION_ENABLED_EXTENSIONS) ? 1 : 0,
        GetTrackedPrefHistogramCount("Settings.TrackedPreferenceWantedReset",
                                     BEGIN_ALLOW_SINGLE_BUCKET + 5));
    EXPECT_EQ(protection_level_ >= PROTECTION_ENABLED_EXTENSIONS ? 1 : 0,
              GetTrackedPrefHistogramCount("Settings.TrackedPreferenceReset",
                                           BEGIN_ALLOW_SINGLE_BUCKET + 5));

    EXPECT_EQ(protection_level_ < PROTECTION_ENABLED_EXTENSIONS,
              extension_service()->GetExtensionById(kGoodCrxId, true) != NULL);

    // Nothing else should have triggered.
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount("Settings.TrackedPreferenceCleared",
                                           ALLOW_NONE));
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceInitialized", ALLOW_NONE));
    EXPECT_EQ(0,
              GetTrackedPrefHistogramCount(
                  "Settings.TrackedPreferenceTrustedInitialized", ALLOW_NONE));
    EXPECT_EQ(
        0,
        GetTrackedPrefHistogramCount(
            "Settings.TrackedPreferenceMigratedLegacyDeviceId", ALLOW_NONE));
  }
};

PREF_HASH_BROWSER_TEST(PrefHashBrowserTestChangedSplitPref, ChangedSplitPref);
