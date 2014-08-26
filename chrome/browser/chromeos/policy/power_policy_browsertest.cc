// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/policy/device_policy_builder.h"
#include "chrome/browser/chromeos/policy/device_policy_cros_browser_test.h"
#include "chrome/browser/chromeos/policy/proto/chrome_device_policy.pb.h"
#include "chrome/browser/chromeos/policy/user_cloud_policy_manager_chromeos.h"
#include "chrome/browser/chromeos/policy/user_cloud_policy_manager_factory_chromeos.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/policy/profile_policy_connector_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/testing_profile.h"
#include "chromeos/chromeos_paths.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/fake_dbus_thread_manager.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "chromeos/dbus/fake_session_manager_client.h"
#include "chromeos/dbus/power_manager/policy.pb.h"
#include "chromeos/dbus/power_policy_controller.h"
#include "chromeos/login/user_names.h"
#include "components/policy/core/common/cloud/cloud_policy_core.h"
#include "components/policy/core/common/cloud/cloud_policy_store.h"
#include "components/policy/core/common/cloud/policy_builder.h"
#include "components/policy/core/common/external_data_fetcher.h"
#include "components/policy/core/common/mock_policy_service.h"
#include "components/policy/core/common/policy_service.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/api/power/power_api_manager.h"
#include "extensions/common/api/power.h"
#include "policy/proto/device_management_backend.pb.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace em = enterprise_management;
namespace pm = power_manager;

using ::testing::AnyNumber;
using ::testing::InvokeWithoutArgs;
using ::testing::_;

namespace policy {

namespace {

const char kLoginScreenPowerManagementPolicy[] =
    "{"
    "  \"AC\": {"
    "    \"Delays\": {"
    "      \"ScreenDim\": 5000,"
    "      \"ScreenOff\": 7000,"
    "      \"Idle\": 9000"
    "    },"
    "    \"IdleAction\": \"DoNothing\""
    "  },"
    "  \"Battery\": {"
    "    \"Delays\": {"
    "      \"ScreenDim\": 1000,"
    "      \"ScreenOff\": 3000,"
    "      \"Idle\": 4000"
    "    },"
    "    \"IdleAction\": \"DoNothing\""
    "  },"
    "  \"LidCloseAction\": \"DoNothing\","
    "  \"UserActivityScreenDimDelayScale\": 300"
    "}";

const char kPowerManagementIdleSettingsPolicy[] =
    "{"
    "  \"AC\": {"
    "    \"Delays\": {"
    "      \"ScreenDim\": 5000,"
    "      \"ScreenOff\": 7000,"
    "      \"IdleWarning\": 8000,"
    "      \"Idle\": 9000"
    "    },"
    "    \"IdleAction\": \"Logout\""
    "  },"
    "  \"Battery\": {"
    "    \"Delays\": {"
    "      \"ScreenDim\": 1000,"
    "      \"ScreenOff\": 3000,"
    "      \"IdleWarning\": 4000,"
    "      \"Idle\": 5000"
    "    },"
    "    \"IdleAction\": \"Logout\""
    "  }"
    "}";

const char kScreenLockDelayPolicy[] =
    "{"
    "  \"AC\": 6000,"
    "  \"Battery\": 2000"
    "}";

}  // namespace

class PowerPolicyBrowserTestBase : public DevicePolicyCrosBrowserTest {
 protected:
  PowerPolicyBrowserTestBase();

  // DevicePolicyCrosBrowserTest:
  virtual void SetUpInProcessBrowserTestFixture() OVERRIDE;
  virtual void SetUpOnMainThread() OVERRIDE;

  void InstallUserKey();
  void StoreAndReloadUserPolicy();

  void StoreAndReloadDevicePolicyAndWaitForLoginProfileChange();

  // Returns a string describing |policy|.
  std::string GetDebugString(const pm::PowerManagementPolicy& policy);

  UserPolicyBuilder user_policy_;

  chromeos::FakePowerManagerClient* power_manager_client_;

 private:
  // Runs |closure| and waits for |profile|'s user policy to be updated as a
  // result.
  void RunClosureAndWaitForUserPolicyUpdate(const base::Closure& closure,
                                            Profile* profile);

  // Reloads user policy for |profile| from session manager client.
  void ReloadUserPolicy(Profile* profile);

  DISALLOW_COPY_AND_ASSIGN(PowerPolicyBrowserTestBase);
};

class PowerPolicyLoginScreenBrowserTest : public PowerPolicyBrowserTestBase {
 protected:
  PowerPolicyLoginScreenBrowserTest();

  // PowerPolicyBrowserTestBase:
  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE;
  virtual void SetUpOnMainThread() OVERRIDE;
  virtual void TearDownOnMainThread() OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(PowerPolicyLoginScreenBrowserTest);
};

class PowerPolicyInSessionBrowserTest : public PowerPolicyBrowserTestBase {
 protected:
  PowerPolicyInSessionBrowserTest();

  // PowerPolicyBrowserTestBase:
  virtual void SetUpOnMainThread() OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(PowerPolicyInSessionBrowserTest);
};

PowerPolicyBrowserTestBase::PowerPolicyBrowserTestBase()
    : power_manager_client_(NULL) {
}

void PowerPolicyBrowserTestBase::SetUpInProcessBrowserTestFixture() {
  power_manager_client_ = new chromeos::FakePowerManagerClient;
  fake_dbus_thread_manager()->SetPowerManagerClient(
      scoped_ptr<chromeos::PowerManagerClient>(power_manager_client_));

  DevicePolicyCrosBrowserTest::SetUpInProcessBrowserTestFixture();

  // Initialize device policy.
  InstallOwnerKey();
  MarkAsEnterpriseOwned();
}

void PowerPolicyBrowserTestBase::SetUpOnMainThread() {
  DevicePolicyCrosBrowserTest::SetUpOnMainThread();

  // Initialize user policy.
  InstallUserKey();
  user_policy_.policy_data().set_username(chromeos::login::kStubUser);
}

void PowerPolicyBrowserTestBase::InstallUserKey() {
  base::FilePath user_keys_dir;
  ASSERT_TRUE(PathService::Get(chromeos::DIR_USER_POLICY_KEYS, &user_keys_dir));
  std::string sanitized_username =
      chromeos::CryptohomeClient::GetStubSanitizedUsername(
          chromeos::login::kStubUser);
  base::FilePath user_key_file =
      user_keys_dir.AppendASCII(sanitized_username)
                   .AppendASCII("policy.pub");
  std::vector<uint8> user_key_bits;
  ASSERT_TRUE(user_policy_.GetSigningKey()->ExportPublicKey(&user_key_bits));
  ASSERT_TRUE(base::CreateDirectory(user_key_file.DirName()));
  ASSERT_EQ(base::WriteFile(
                user_key_file,
                reinterpret_cast<const char*>(user_key_bits.data()),
                user_key_bits.size()),
            static_cast<int>(user_key_bits.size()));
}

void PowerPolicyBrowserTestBase::StoreAndReloadUserPolicy() {
  // Install the new user policy blob in session manager client.
  user_policy_.Build();
  session_manager_client()->set_user_policy(
      user_policy_.policy_data().username(),
      user_policy_.GetBlob());

  // Reload user policy from session manager client and wait for the update to
  // take effect.
  RunClosureAndWaitForUserPolicyUpdate(
      base::Bind(&PowerPolicyBrowserTestBase::ReloadUserPolicy, this,
                 browser()->profile()),
      browser()->profile());
}

void PowerPolicyBrowserTestBase::
    StoreAndReloadDevicePolicyAndWaitForLoginProfileChange() {
  Profile* profile = chromeos::ProfileHelper::GetSigninProfile();
  ASSERT_TRUE(profile);

  // Install the new device policy blob in session manager client, reload device
  // policy from session manager client and wait for a change in the login
  // profile's policy to be observed.
  RunClosureAndWaitForUserPolicyUpdate(
      base::Bind(&PowerPolicyBrowserTestBase::RefreshDevicePolicy, this),
      profile);
}

std::string PowerPolicyBrowserTestBase::GetDebugString(
    const pm::PowerManagementPolicy& policy) {
  return chromeos::PowerPolicyController::GetPolicyDebugString(policy);
}

void PowerPolicyBrowserTestBase::RunClosureAndWaitForUserPolicyUpdate(
    const base::Closure& closure,
    Profile* profile) {
  base::RunLoop run_loop;
  MockPolicyServiceObserver observer;
  EXPECT_CALL(observer, OnPolicyUpdated(_, _, _))
      .WillOnce(InvokeWithoutArgs(&run_loop, &base::RunLoop::Quit));
  EXPECT_CALL(observer, OnPolicyServiceInitialized(_)).Times(AnyNumber());
  PolicyService* policy_service =
      ProfilePolicyConnectorFactory::GetForProfile(profile)->policy_service();
  ASSERT_TRUE(policy_service);
  policy_service->AddObserver(POLICY_DOMAIN_CHROME, &observer);
  closure.Run();
  run_loop.Run();
  policy_service->RemoveObserver(POLICY_DOMAIN_CHROME, &observer);
}

void PowerPolicyBrowserTestBase::ReloadUserPolicy(Profile* profile) {
  UserCloudPolicyManagerChromeOS* policy_manager =
      UserCloudPolicyManagerFactoryChromeOS::GetForProfile(profile);
  ASSERT_TRUE(policy_manager);
  policy_manager->core()->store()->Load();
}

PowerPolicyLoginScreenBrowserTest::PowerPolicyLoginScreenBrowserTest() {
}

void PowerPolicyLoginScreenBrowserTest::SetUpCommandLine(
    CommandLine* command_line) {
  PowerPolicyBrowserTestBase::SetUpCommandLine(command_line);
  command_line->AppendSwitch(chromeos::switches::kLoginManager);
  command_line->AppendSwitch(chromeos::switches::kForceLoginManagerInTests);
}

void PowerPolicyLoginScreenBrowserTest::SetUpOnMainThread() {
  PowerPolicyBrowserTestBase::SetUpOnMainThread();

  // Wait for the login screen to be shown.
  content::WindowedNotificationObserver(
      chrome::NOTIFICATION_LOGIN_OR_LOCK_WEBUI_VISIBLE,
      content::NotificationService::AllSources()).Wait();
}

void PowerPolicyLoginScreenBrowserTest::TearDownOnMainThread() {
  base::MessageLoop::current()->PostTask(FROM_HERE,
                                         base::Bind(&chrome::AttemptExit));
  base::RunLoop().RunUntilIdle();
  PowerPolicyBrowserTestBase::TearDownOnMainThread();
}

PowerPolicyInSessionBrowserTest::PowerPolicyInSessionBrowserTest() {
}

void PowerPolicyInSessionBrowserTest::SetUpOnMainThread() {
  PowerPolicyBrowserTestBase::SetUpOnMainThread();
}

// Verifies that device policy is applied on the login screen.
IN_PROC_BROWSER_TEST_F(PowerPolicyLoginScreenBrowserTest, SetDevicePolicy) {
  pm::PowerManagementPolicy power_management_policy =
      power_manager_client_->policy();
  power_management_policy.mutable_ac_delays()->set_screen_dim_ms(5000);
  power_management_policy.mutable_ac_delays()->set_screen_off_ms(7000);
  power_management_policy.mutable_ac_delays()->set_idle_ms(9000);
  power_management_policy.mutable_battery_delays()->set_screen_dim_ms(1000);
  power_management_policy.mutable_battery_delays()->set_screen_off_ms(3000);
  power_management_policy.mutable_battery_delays()->set_idle_ms(4000);
  power_management_policy.set_ac_idle_action(
      pm::PowerManagementPolicy::DO_NOTHING);
  power_management_policy.set_battery_idle_action(
      pm::PowerManagementPolicy::DO_NOTHING);
  power_management_policy.set_lid_closed_action(
      pm::PowerManagementPolicy::DO_NOTHING);
  power_management_policy.set_user_activity_screen_dim_delay_factor(3.0);

  em::ChromeDeviceSettingsProto& proto(device_policy()->payload());
  proto.mutable_login_screen_power_management()->
      set_login_screen_power_management(kLoginScreenPowerManagementPolicy);
  StoreAndReloadDevicePolicyAndWaitForLoginProfileChange();
  EXPECT_EQ(GetDebugString(power_management_policy),
            GetDebugString(power_manager_client_->policy()));
}

// Verifies that device policy is ignored during a session.
IN_PROC_BROWSER_TEST_F(PowerPolicyInSessionBrowserTest, SetDevicePolicy) {
  pm::PowerManagementPolicy power_management_policy =
      power_manager_client_->policy();

  em::ChromeDeviceSettingsProto& proto(device_policy()->payload());
  proto.mutable_login_screen_power_management()->
      set_login_screen_power_management(kLoginScreenPowerManagementPolicy);
  StoreAndReloadDevicePolicyAndWaitForLoginProfileChange();
  EXPECT_EQ(GetDebugString(power_management_policy),
            GetDebugString(power_manager_client_->policy()));
}

// Verifies that legacy user policy is applied during a session.
IN_PROC_BROWSER_TEST_F(PowerPolicyInSessionBrowserTest, SetLegacyUserPolicy) {
  pm::PowerManagementPolicy power_management_policy =
      power_manager_client_->policy();
  power_management_policy.mutable_ac_delays()->set_screen_dim_ms(5000);
  power_management_policy.mutable_ac_delays()->set_screen_lock_ms(6000);
  power_management_policy.mutable_ac_delays()->set_screen_off_ms(7000);
  power_management_policy.mutable_ac_delays()->set_idle_warning_ms(8000);
  power_management_policy.mutable_ac_delays()->set_idle_ms(9000);
  power_management_policy.mutable_battery_delays()->set_screen_dim_ms(1000);
  power_management_policy.mutable_battery_delays()->set_screen_lock_ms(2000);
  power_management_policy.mutable_battery_delays()->set_screen_off_ms(3000);
  power_management_policy.mutable_battery_delays()->set_idle_warning_ms(4000);
  power_management_policy.mutable_battery_delays()->set_idle_ms(5000);
  power_management_policy.set_use_audio_activity(false);
  power_management_policy.set_use_video_activity(false);
  power_management_policy.set_ac_idle_action(
      pm::PowerManagementPolicy::STOP_SESSION);
  power_management_policy.set_battery_idle_action(
      pm::PowerManagementPolicy::STOP_SESSION);
  power_management_policy.set_lid_closed_action(
      pm::PowerManagementPolicy::STOP_SESSION);
  power_management_policy.set_presentation_screen_dim_delay_factor(3.0);
  power_management_policy.set_user_activity_screen_dim_delay_factor(3.0);
  power_management_policy.set_wait_for_initial_user_activity(true);

  user_policy_.payload().mutable_screendimdelayac()->set_value(5000);
  user_policy_.payload().mutable_screenlockdelayac()->set_value(6000);
  user_policy_.payload().mutable_screenoffdelayac()->set_value(7000);
  user_policy_.payload().mutable_idlewarningdelayac()->set_value(8000);
  user_policy_.payload().mutable_idledelayac()->set_value(9000);
  user_policy_.payload().mutable_screendimdelaybattery()->set_value(1000);
  user_policy_.payload().mutable_screenlockdelaybattery()->set_value(2000);
  user_policy_.payload().mutable_screenoffdelaybattery()->set_value(3000);
  user_policy_.payload().mutable_idlewarningdelaybattery()->set_value(4000);
  user_policy_.payload().mutable_idledelaybattery()->set_value(5000);
  user_policy_.payload().mutable_powermanagementusesaudioactivity()->set_value(
      false);
  user_policy_.payload().mutable_powermanagementusesvideoactivity()->set_value(
      false);
  user_policy_.payload().mutable_idleactionac()->set_value(
      chromeos::PowerPolicyController::ACTION_STOP_SESSION);
  user_policy_.payload().mutable_idleactionbattery()->set_value(
      chromeos::PowerPolicyController::ACTION_STOP_SESSION);
  user_policy_.payload().mutable_lidcloseaction()->set_value(
      chromeos::PowerPolicyController::ACTION_STOP_SESSION);
  user_policy_.payload().mutable_presentationscreendimdelayscale()->set_value(
      300);
  user_policy_.payload().mutable_useractivityscreendimdelayscale()->set_value(
      300);
  user_policy_.payload().mutable_waitforinitialuseractivity()->set_value(true);
  StoreAndReloadUserPolicy();
  EXPECT_EQ(GetDebugString(power_management_policy),
            GetDebugString(power_manager_client_->policy()));
}

// Verifies that user policy is applied during a session.
IN_PROC_BROWSER_TEST_F(PowerPolicyInSessionBrowserTest, SetUserPolicy) {
  pm::PowerManagementPolicy power_management_policy =
      power_manager_client_->policy();
  power_management_policy.mutable_ac_delays()->set_screen_dim_ms(5000);
  power_management_policy.mutable_ac_delays()->set_screen_lock_ms(6000);
  power_management_policy.mutable_ac_delays()->set_screen_off_ms(7000);
  power_management_policy.mutable_ac_delays()->set_idle_warning_ms(8000);
  power_management_policy.mutable_ac_delays()->set_idle_ms(9000);
  power_management_policy.mutable_battery_delays()->set_screen_dim_ms(1000);
  power_management_policy.mutable_battery_delays()->set_screen_lock_ms(2000);
  power_management_policy.mutable_battery_delays()->set_screen_off_ms(3000);
  power_management_policy.mutable_battery_delays()->set_idle_warning_ms(4000);
  power_management_policy.mutable_battery_delays()->set_idle_ms(5000);
  power_management_policy.set_use_audio_activity(false);
  power_management_policy.set_use_video_activity(false);
  power_management_policy.set_ac_idle_action(
      pm::PowerManagementPolicy::STOP_SESSION);
  power_management_policy.set_battery_idle_action(
      pm::PowerManagementPolicy::STOP_SESSION);
  power_management_policy.set_lid_closed_action(
      pm::PowerManagementPolicy::STOP_SESSION);
  power_management_policy.set_presentation_screen_dim_delay_factor(3.0);
  power_management_policy.set_user_activity_screen_dim_delay_factor(3.0);
  power_management_policy.set_wait_for_initial_user_activity(true);

  // Set legacy policies which are expected to be ignored.
  user_policy_.payload().mutable_screendimdelayac()->set_value(5555);
  user_policy_.payload().mutable_screenlockdelayac()->set_value(6666);
  user_policy_.payload().mutable_screenoffdelayac()->set_value(7777);
  user_policy_.payload().mutable_idlewarningdelayac()->set_value(8888);
  user_policy_.payload().mutable_idledelayac()->set_value(9999);
  user_policy_.payload().mutable_screendimdelaybattery()->set_value(1111);
  user_policy_.payload().mutable_screenlockdelaybattery()->set_value(2222);
  user_policy_.payload().mutable_screenoffdelaybattery()->set_value(3333);
  user_policy_.payload().mutable_idlewarningdelaybattery()->set_value(4444);
  user_policy_.payload().mutable_idledelaybattery()->set_value(5555);
  user_policy_.payload().mutable_idleactionac()->set_value(
      chromeos::PowerPolicyController::ACTION_SHUT_DOWN);
  user_policy_.payload().mutable_idleactionbattery()->set_value(
      chromeos::PowerPolicyController::ACTION_DO_NOTHING);

  // Set current policies which are expected to be honored.
  user_policy_.payload().mutable_powermanagementusesaudioactivity()->set_value(
      false);
  user_policy_.payload().mutable_powermanagementusesvideoactivity()->set_value(
      false);
  user_policy_.payload().mutable_lidcloseaction()->set_value(
      chromeos::PowerPolicyController::ACTION_STOP_SESSION);
  user_policy_.payload().mutable_presentationscreendimdelayscale()->set_value(
      300);
  user_policy_.payload().mutable_useractivityscreendimdelayscale()->set_value(
      300);
  user_policy_.payload().mutable_waitforinitialuseractivity()->set_value(true);

  user_policy_.payload().mutable_powermanagementidlesettings()->set_value(
      kPowerManagementIdleSettingsPolicy);
  user_policy_.payload().mutable_screenlockdelays()->set_value(
      kScreenLockDelayPolicy);

  StoreAndReloadUserPolicy();
  EXPECT_EQ(GetDebugString(power_management_policy),
            GetDebugString(power_manager_client_->policy()));
}

// Verifies that screen wake locks can be enabled and disabled by extensions and
// user policy during a session.
IN_PROC_BROWSER_TEST_F(PowerPolicyInSessionBrowserTest, AllowScreenWakeLocks) {
  pm::PowerManagementPolicy baseline_policy =
      power_manager_client_->policy();

  // Default settings should have delays.
  pm::PowerManagementPolicy power_management_policy = baseline_policy;
  EXPECT_NE(0, baseline_policy.ac_delays().screen_dim_ms());
  EXPECT_NE(0, baseline_policy.ac_delays().screen_off_ms());
  EXPECT_NE(0, baseline_policy.battery_delays().screen_dim_ms());
  EXPECT_NE(0, baseline_policy.battery_delays().screen_off_ms());

  // Pretend an extension grabs a screen wake lock.
  const char kExtensionId[] = "abcdefghijklmnopabcdefghijlkmnop";
  extensions::PowerApiManager::Get(browser()->profile())->AddRequest(
      kExtensionId, extensions::core_api::power::LEVEL_DISPLAY);
  base::RunLoop().RunUntilIdle();

  // Check that the lock is in effect (ignoring ac_idle_action,
  // battery_idle_action and reason).
  pm::PowerManagementPolicy policy = baseline_policy;
  policy.mutable_ac_delays()->set_screen_dim_ms(0);
  policy.mutable_ac_delays()->set_screen_off_ms(0);
  policy.mutable_battery_delays()->set_screen_dim_ms(0);
  policy.mutable_battery_delays()->set_screen_off_ms(0);
  policy.set_ac_idle_action(
      power_manager_client_->policy().ac_idle_action());
  policy.set_battery_idle_action(
      power_manager_client_->policy().battery_idle_action());
  policy.set_reason(power_manager_client_->policy().reason());
  EXPECT_EQ(GetDebugString(policy),
            GetDebugString(power_manager_client_->policy()));

  // Engage the user policy and verify that the defaults take effect again.
  user_policy_.payload().mutable_allowscreenwakelocks()->set_value(false);
  StoreAndReloadUserPolicy();
  policy = baseline_policy;
  policy.set_ac_idle_action(power_manager_client_->policy().ac_idle_action());
  policy.set_battery_idle_action(
      power_manager_client_->policy().battery_idle_action());
  policy.set_reason(power_manager_client_->policy().reason());
  EXPECT_EQ(GetDebugString(policy),
            GetDebugString(power_manager_client_->policy()));
}

}  // namespace policy
