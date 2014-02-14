// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/saml/saml_offline_signin_limiter.h"

#include "base/memory/ref_counted.h"
#include "base/prefs/pref_service.h"
#include "base/test/simple_test_clock.h"
#include "base/test/test_simple_task_runner.h"
#include "base/thread_task_runner_handle.h"
#include "base/time/clock.h"
#include "chrome/browser/chromeos/login/mock_user_manager.h"
#include "chrome/browser/chromeos/login/saml/saml_offline_signin_limiter_factory.h"
#include "chrome/browser/chromeos/login/user_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/user_prefs/pref_registry_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Mock;
using testing::ReturnRef;
using testing::Sequence;

namespace chromeos {

namespace {
const char kTestUser[] = "user@example.com";
}

class SAMLOfflineSigninLimiterTest : public testing::Test {
 protected:
  SAMLOfflineSigninLimiterTest();
  virtual ~SAMLOfflineSigninLimiterTest();

  // testing::Test:
  virtual void SetUp() OVERRIDE;
  virtual void TearDown() OVERRIDE;

  void DestroyLimiter();
  void CreateLimiter();

  scoped_refptr<base::TestSimpleTaskRunner> runner_;
  base::ThreadTaskRunnerHandle runner_handle_;

  MockUserManager* user_manager_;  // Not owned.
  ScopedUserManagerEnabler user_manager_enabler_;

  scoped_ptr<TestingProfile> profile_;
  base::SimpleTestClock clock_;

  SAMLOfflineSigninLimiter* limiter_;  // Owned.

  DISALLOW_COPY_AND_ASSIGN(SAMLOfflineSigninLimiterTest);
};

SAMLOfflineSigninLimiterTest::SAMLOfflineSigninLimiterTest()
    : runner_(new base::TestSimpleTaskRunner),
      runner_handle_(runner_),
      user_manager_(new MockUserManager),
      user_manager_enabler_(user_manager_),
      limiter_(NULL) {
}

SAMLOfflineSigninLimiterTest::~SAMLOfflineSigninLimiterTest() {
  DestroyLimiter();
  Mock::VerifyAndClearExpectations(user_manager_);
  EXPECT_CALL(*user_manager_, Shutdown()).Times(1);
}

void SAMLOfflineSigninLimiterTest::DestroyLimiter() {
  if (limiter_) {
    limiter_->Shutdown();
    delete limiter_;
    limiter_ = NULL;
  }
}

void SAMLOfflineSigninLimiterTest::CreateLimiter() {
  DestroyLimiter();
  limiter_ = new SAMLOfflineSigninLimiter(profile_.get(), &clock_);
}

void SAMLOfflineSigninLimiterTest::SetUp() {
  const UserList user_list;
  EXPECT_CALL(*user_manager_, GetLoggedInUsers())
      .Times(1)
      .WillOnce(ReturnRef(user_list));
  profile_.reset(new TestingProfile);
  Mock::VerifyAndClearExpectations(user_manager_);

  SAMLOfflineSigninLimiterFactory::SetClockForTesting(&clock_);
  user_manager_->AddUser(kTestUser);
  profile_->set_profile_name(kTestUser);
  clock_.Advance(base::TimeDelta::FromHours(1));
}

void SAMLOfflineSigninLimiterTest::TearDown() {
  SAMLOfflineSigninLimiterFactory::SetClockForTesting(NULL);
}

TEST_F(SAMLOfflineSigninLimiterTest, NoSAMLDefaultLimit) {
  PrefService* prefs = profile_->GetPrefs();

  // Set the time of last login with SAML.
  prefs->SetInt64(prefs::kSAMLLastGAIASignInTime,
                  clock_.Now().ToInternalValue());

  // Authenticate against GAIA without SAML. Verify that the flag enforcing
  // online login and the time of last login with SAML are cleared.
  CreateLimiter();
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(1);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_GAIA_WITHOUT_SAML);

  const PrefService::Preference* pref =
      prefs->FindPreference(prefs::kSAMLLastGAIASignInTime);
  ASSERT_TRUE(pref);
  EXPECT_FALSE(pref->HasUserSetting());

  // Verify that no timer is running.
  EXPECT_FALSE(runner_->HasPendingTask());

  // Log out. Verify that the flag enforcing online login is not set.
  DestroyLimiter();

  // Authenticate offline. Verify that the flag enforcing online login is not
  // changed and the time of last login with SAML is not set.
  CreateLimiter();
  Mock::VerifyAndClearExpectations(user_manager_);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(0);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_OFFLINE);

  pref = prefs->FindPreference(prefs::kSAMLLastGAIASignInTime);
  ASSERT_TRUE(pref);
  EXPECT_FALSE(pref->HasUserSetting());

  // Verify that no timer is running.
  EXPECT_FALSE(runner_->HasPendingTask());
}

TEST_F(SAMLOfflineSigninLimiterTest, NoSAMLNoLimit) {
  PrefService* prefs = profile_->GetPrefs();

  // Remove the time limit.
  prefs->SetInteger(prefs::kSAMLOfflineSigninTimeLimit, -1);

  // Set the time of last login with SAML.
  prefs->SetInt64(prefs::kSAMLLastGAIASignInTime,
                  clock_.Now().ToInternalValue());

  // Authenticate against GAIA without SAML. Verify that the flag enforcing
  // online login and the time of last login with SAML are cleared.
  CreateLimiter();
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(1);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_GAIA_WITHOUT_SAML);

  const PrefService::Preference* pref =
      prefs->FindPreference(prefs::kSAMLLastGAIASignInTime);
  ASSERT_TRUE(pref);
  EXPECT_FALSE(pref->HasUserSetting());

  // Verify that no timer is running.
  EXPECT_FALSE(runner_->HasPendingTask());

  // Log out. Verify that the flag enforcing online login is not set.
  DestroyLimiter();

  // Authenticate offline. Verify that the flag enforcing online login is not
  // changed and the time of last login with SAML is not set.
  CreateLimiter();
  Mock::VerifyAndClearExpectations(user_manager_);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(0);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_OFFLINE);

  pref = prefs->FindPreference(prefs::kSAMLLastGAIASignInTime);
  ASSERT_TRUE(pref);
  EXPECT_FALSE(pref->HasUserSetting());

  // Verify that no timer is running.
  EXPECT_FALSE(runner_->HasPendingTask());
}

TEST_F(SAMLOfflineSigninLimiterTest, NoSAMLZeroLimit) {
  PrefService* prefs = profile_->GetPrefs();

  // Set a zero time limit.
  prefs->SetInteger(prefs::kSAMLOfflineSigninTimeLimit, 0);

  // Set the time of last login with SAML.
  prefs->SetInt64(prefs::kSAMLLastGAIASignInTime,
                  clock_.Now().ToInternalValue());

  // Authenticate against GAIA without SAML. Verify that the flag enforcing
  // online login and the time of last login with SAML are cleared.
  CreateLimiter();
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(1);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_GAIA_WITHOUT_SAML);

  const PrefService::Preference* pref =
      prefs->FindPreference(prefs::kSAMLLastGAIASignInTime);
  ASSERT_TRUE(pref);
  EXPECT_FALSE(pref->HasUserSetting());

  // Verify that no timer is running.
  EXPECT_FALSE(runner_->HasPendingTask());

  // Log out. Verify that the flag enforcing online login is not set.
  DestroyLimiter();

  // Authenticate offline. Verify that the flag enforcing online login is not
  // changed and the time of last login with SAML is not set.
  CreateLimiter();
  Mock::VerifyAndClearExpectations(user_manager_);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(0);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_OFFLINE);

  pref = prefs->FindPreference(prefs::kSAMLLastGAIASignInTime);
  ASSERT_TRUE(pref);
  EXPECT_FALSE(pref->HasUserSetting());

  // Verify that no timer is running.
  EXPECT_FALSE(runner_->HasPendingTask());
}

TEST_F(SAMLOfflineSigninLimiterTest, NoSAMLSetLimitWhileLoggedIn) {
  PrefService* prefs = profile_->GetPrefs();

  // Remove the time limit.
  prefs->SetInteger(prefs::kSAMLOfflineSigninTimeLimit, -1);

  // Set the time of last login with SAML.
  prefs->SetInt64(prefs::kSAMLLastGAIASignInTime,
                  clock_.Now().ToInternalValue());

  // Authenticate against GAIA without SAML. Verify that the flag enforcing
  // online login and the time of last login with SAML are cleared.
  CreateLimiter();
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(1);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_GAIA_WITHOUT_SAML);

  const PrefService::Preference* pref =
      prefs->FindPreference(prefs::kSAMLLastGAIASignInTime);
  ASSERT_TRUE(pref);
  EXPECT_FALSE(pref->HasUserSetting());

  // Verify that no timer is running.
  EXPECT_FALSE(runner_->HasPendingTask());

  // Set a zero time limit.
  prefs->SetInteger(prefs::kSAMLOfflineSigninTimeLimit, 0);

  // Verify that no timer is running.
  EXPECT_FALSE(runner_->HasPendingTask());
}

TEST_F(SAMLOfflineSigninLimiterTest, NoSAMLRemoveLimitWhileLoggedIn) {
  PrefService* prefs = profile_->GetPrefs();

  // Set the time of last login with SAML.
  prefs->SetInt64(prefs::kSAMLLastGAIASignInTime,
                  clock_.Now().ToInternalValue());

  // Authenticate against GAIA without SAML. Verify that the flag enforcing
  // online login and the time of last login with SAML are cleared.
  CreateLimiter();
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(1);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_GAIA_WITHOUT_SAML);

  const PrefService::Preference* pref =
      prefs->FindPreference(prefs::kSAMLLastGAIASignInTime);
  ASSERT_TRUE(pref);
  EXPECT_FALSE(pref->HasUserSetting());

  // Verify that no timer is running.
  EXPECT_FALSE(runner_->HasPendingTask());

  // Remove the time limit.
  prefs->SetInteger(prefs::kSAMLOfflineSigninTimeLimit, -1);

  // Verify that no timer is running.
  EXPECT_FALSE(runner_->HasPendingTask());
}

TEST_F(SAMLOfflineSigninLimiterTest, NoSAMLLogInWithExpiredLimit) {
  PrefService* prefs = profile_->GetPrefs();

  // Set the time of last login with SAML.
  prefs->SetInt64(prefs::kSAMLLastGAIASignInTime,
                  clock_.Now().ToInternalValue());

  // Advance time by four weeks.
  clock_.Advance(base::TimeDelta::FromDays(28));  // 4 weeks.

  // Authenticate against GAIA without SAML. Verify that the flag enforcing
  // online login and the time of last login with SAML are cleared.
  CreateLimiter();
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(1);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_GAIA_WITHOUT_SAML);

  const PrefService::Preference* pref =
      prefs->FindPreference(prefs::kSAMLLastGAIASignInTime);
  ASSERT_TRUE(pref);
  EXPECT_FALSE(pref->HasUserSetting());

  // Verify that no timer is running.
  EXPECT_FALSE(runner_->HasPendingTask());
}

TEST_F(SAMLOfflineSigninLimiterTest, SAMLDefaultLimit) {
  PrefService* prefs = profile_->GetPrefs();

  // Authenticate against GAIA with SAML. Verify that the flag enforcing online
  // login is cleared and the time of last login with SAML is set.
  CreateLimiter();
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(1);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_GAIA_WITH_SAML);

  base::Time last_gaia_signin_time = base::Time::FromInternalValue(
      prefs->GetInt64(prefs::kSAMLLastGAIASignInTime));
  EXPECT_EQ(clock_.Now(), last_gaia_signin_time);

  // Verify that the timer is running.
  EXPECT_TRUE(runner_->HasPendingTask());

  // Log out. Verify that the flag enforcing online login is not set.
  DestroyLimiter();

  // Advance time by an hour.
  clock_.Advance(base::TimeDelta::FromHours(1));

  // Authenticate against GAIA with SAML. Verify that the flag enforcing online
  // login is cleared and the time of last login with SAML is updated.
  CreateLimiter();
  Mock::VerifyAndClearExpectations(user_manager_);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(1);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_GAIA_WITH_SAML);

  last_gaia_signin_time = base::Time::FromInternalValue(
      prefs->GetInt64(prefs::kSAMLLastGAIASignInTime));
  EXPECT_EQ(clock_.Now(), last_gaia_signin_time);

  // Verify that the timer is running.
  EXPECT_TRUE(runner_->HasPendingTask());

  // Log out. Verify that the flag enforcing online login is not set.
  DestroyLimiter();

  // Advance time by an hour.
  const base::Time gaia_signin_time = clock_.Now();
  clock_.Advance(base::TimeDelta::FromHours(1));

  // Authenticate offline. Verify that the flag enforcing online login and the
  // time of last login with SAML are not changed.
  CreateLimiter();
  Mock::VerifyAndClearExpectations(user_manager_);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(0);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_OFFLINE);

  last_gaia_signin_time = base::Time::FromInternalValue(
      prefs->GetInt64(prefs::kSAMLLastGAIASignInTime));
  EXPECT_EQ(gaia_signin_time, last_gaia_signin_time);

  // Verify that the timer is running.
  EXPECT_TRUE(runner_->HasPendingTask());

  // Advance time by four weeks.
  clock_.Advance(base::TimeDelta::FromDays(28));  // 4 weeks.

  // Allow the timer to fire. Verify that the flag enforcing online login is
  // set.
  Mock::VerifyAndClearExpectations(user_manager_);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(0);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(1);
  runner_->RunPendingTasks();
}

TEST_F(SAMLOfflineSigninLimiterTest, SAMLNoLimit) {
  PrefService* prefs = profile_->GetPrefs();

  // Remove the time limit.
  prefs->SetInteger(prefs::kSAMLOfflineSigninTimeLimit, -1);

  // Authenticate against GAIA with SAML. Verify that the flag enforcing online
  // login is cleared and the time of last login with SAML is set.
  CreateLimiter();
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(1);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_GAIA_WITH_SAML);

  base::Time last_gaia_signin_time = base::Time::FromInternalValue(
      prefs->GetInt64(prefs::kSAMLLastGAIASignInTime));
  EXPECT_EQ(clock_.Now(), last_gaia_signin_time);

  // Verify that no timer is running.
  EXPECT_FALSE(runner_->HasPendingTask());

  // Log out. Verify that the flag enforcing online login is not set.
  DestroyLimiter();

  // Advance time by an hour.
  clock_.Advance(base::TimeDelta::FromHours(1));

  // Authenticate against GAIA with SAML. Verify that the flag enforcing online
  // login is cleared and the time of last login with SAML is updated.
  CreateLimiter();
  Mock::VerifyAndClearExpectations(user_manager_);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(1);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_GAIA_WITH_SAML);

  last_gaia_signin_time = base::Time::FromInternalValue(
      prefs->GetInt64(prefs::kSAMLLastGAIASignInTime));
  EXPECT_EQ(clock_.Now(), last_gaia_signin_time);

  // Verify that no timer is running.
  EXPECT_FALSE(runner_->HasPendingTask());

  // Log out. Verify that the flag enforcing online login is not set.
  DestroyLimiter();

  // Advance time by an hour.
  const base::Time gaia_signin_time = clock_.Now();
  clock_.Advance(base::TimeDelta::FromHours(1));

  // Authenticate offline. Verify that the flag enforcing online login and the
  // time of last login with SAML are not changed.
  CreateLimiter();
  Mock::VerifyAndClearExpectations(user_manager_);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(0);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_OFFLINE);

  last_gaia_signin_time = base::Time::FromInternalValue(
      prefs->GetInt64(prefs::kSAMLLastGAIASignInTime));
  EXPECT_EQ(gaia_signin_time, last_gaia_signin_time);

  // Verify that no timer is running.
  EXPECT_FALSE(runner_->HasPendingTask());
}

TEST_F(SAMLOfflineSigninLimiterTest, SAMLZeroLimit) {
  PrefService* prefs = profile_->GetPrefs();

  // Set a zero time limit.
  prefs->SetInteger(prefs::kSAMLOfflineSigninTimeLimit, 0);

  // Authenticate against GAIA with SAML. Verify that the flag enforcing online
  // login is cleared and then set immediately. Also verify that the time of
  // last login with SAML is set.
  CreateLimiter();
  Sequence sequence;
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false))
      .Times(1)
      .InSequence(sequence);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true))
      .Times(1)
      .InSequence(sequence);
  limiter_->SignedIn(UserContext::AUTH_FLOW_GAIA_WITH_SAML);

  const base::Time last_gaia_signin_time = base::Time::FromInternalValue(
      prefs->GetInt64(prefs::kSAMLLastGAIASignInTime));
  EXPECT_EQ(clock_.Now(), last_gaia_signin_time);
}

TEST_F(SAMLOfflineSigninLimiterTest, SAMLSetLimitWhileLoggedIn) {
  PrefService* prefs = profile_->GetPrefs();

  // Remove the time limit.
  prefs->SetInteger(prefs::kSAMLOfflineSigninTimeLimit, -1);

  // Authenticate against GAIA with SAML. Verify that the flag enforcing online
  // login is cleared and the time of last login with SAML is set.
  CreateLimiter();
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(1);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_GAIA_WITH_SAML);

  const base::Time last_gaia_signin_time = base::Time::FromInternalValue(
      prefs->GetInt64(prefs::kSAMLLastGAIASignInTime));
  EXPECT_EQ(clock_.Now(), last_gaia_signin_time);

  // Verify that no timer is running.
  EXPECT_FALSE(runner_->HasPendingTask());

  // Set a zero time limit. Verify that the flag enforcing online login is set.
  Mock::VerifyAndClearExpectations(user_manager_);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(0);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(1);
  prefs->SetInteger(prefs::kSAMLOfflineSigninTimeLimit, 0);
}

TEST_F(SAMLOfflineSigninLimiterTest, SAMLRemoveLimit) {
  PrefService* prefs = profile_->GetPrefs();

  // Authenticate against GAIA with SAML. Verify that the flag enforcing online
  // login is cleared and the time of last login with SAML is set.
  CreateLimiter();
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(1);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_GAIA_WITH_SAML);

  const base::Time last_gaia_signin_time = base::Time::FromInternalValue(
      prefs->GetInt64(prefs::kSAMLLastGAIASignInTime));
  EXPECT_EQ(clock_.Now(), last_gaia_signin_time);

  // Verify that the timer is running.
  EXPECT_TRUE(runner_->HasPendingTask());

  // Remove the time limit.
  prefs->SetInteger(prefs::kSAMLOfflineSigninTimeLimit, -1);

  // Allow the timer to fire. Verify that the flag enforcing online login is not
  // changed.
  Mock::VerifyAndClearExpectations(user_manager_);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(0);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  runner_->RunUntilIdle();
}

TEST_F(SAMLOfflineSigninLimiterTest, SAMLLogInWithExpiredLimit) {
  PrefService* prefs = profile_->GetPrefs();

  // Set the time of last login with SAML.
  prefs->SetInt64(prefs::kSAMLLastGAIASignInTime,
                  clock_.Now().ToInternalValue());

  // Advance time by four weeks.
  clock_.Advance(base::TimeDelta::FromDays(28));  // 4 weeks.

  // Authenticate against GAIA with SAML. Verify that the flag enforcing online
  // login is cleared and the time of last login with SAML is updated.
  CreateLimiter();
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(1);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(0);
  limiter_->SignedIn(UserContext::AUTH_FLOW_GAIA_WITH_SAML);

  const base::Time last_gaia_signin_time = base::Time::FromInternalValue(
      prefs->GetInt64(prefs::kSAMLLastGAIASignInTime));
  EXPECT_EQ(clock_.Now(), last_gaia_signin_time);

  // Verify that the timer is running.
  EXPECT_TRUE(runner_->HasPendingTask());
}

TEST_F(SAMLOfflineSigninLimiterTest, SAMLLogInOfflineWithExpiredLimit) {
  PrefService* prefs = profile_->GetPrefs();

  // Set the time of last login with SAML.
  prefs->SetInt64(prefs::kSAMLLastGAIASignInTime,
                  clock_.Now().ToInternalValue());

  // Advance time by four weeks.
  const base::Time gaia_signin_time = clock_.Now();
  clock_.Advance(base::TimeDelta::FromDays(28));  // 4 weeks.

  // Authenticate offline. Verify that the flag enforcing online login is
  // set and the time of last login with SAML is not changed.
  CreateLimiter();
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, false)).Times(0);
  EXPECT_CALL(*user_manager_, SaveForceOnlineSignin(kTestUser, true)).Times(1);
  limiter_->SignedIn(UserContext::AUTH_FLOW_OFFLINE);

  const base::Time last_gaia_signin_time = base::Time::FromInternalValue(
      prefs->GetInt64(prefs::kSAMLLastGAIASignInTime));
  EXPECT_EQ(gaia_signin_time, last_gaia_signin_time);
}

} //  namespace chromeos
