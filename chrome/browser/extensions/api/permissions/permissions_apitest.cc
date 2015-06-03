// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/permissions/permissions_api.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_management_test_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/common/permissions/permission_set.h"
#include "extensions/common/switches.h"
#include "net/dns/mock_host_resolver.h"

namespace extensions {

namespace {

static void AddPattern(URLPatternSet* extent, const std::string& pattern) {
  int schemes = URLPattern::SCHEME_ALL;
  extent->AddPattern(URLPattern(schemes, pattern));
}

}  // namespace

class ExperimentalApiTest : public ExtensionApiTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionApiTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kEnableExperimentalExtensionApis);
  }
};

class ExtensionApiTestWithManagementPolicy : public ExtensionApiTest {
 public:
  void SetUpInProcessBrowserTestFixture() override {
    ExtensionApiTest::SetUpInProcessBrowserTestFixture();
    EXPECT_CALL(policy_provider_, IsInitializationComplete(testing::_))
        .WillRepeatedly(testing::Return(true));
    policy_provider_.SetAutoRefresh();
    policy::BrowserPolicyConnector::SetPolicyProviderForTesting(
        &policy_provider_);
  }

 protected:
  policy::MockConfigurationPolicyProvider policy_provider_;
};

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, PermissionsFail) {
  ASSERT_TRUE(RunExtensionTest("permissions/disabled")) << message_;

  // Since the experimental APIs require a flag, this will fail even though
  // it's enabled.
  // TODO(erikkay) This test is currently broken because LoadExtension in
  // ExtensionBrowserTest doesn't actually fail, it just times out.  To fix this
  // I'll need to add an EXTENSION_LOAD_ERROR notification, which is probably
  // too much for the branch.  I'll enable this on trunk later.
  //ASSERT_FALSE(RunExtensionTest("permissions/enabled"))) << message_;
}

IN_PROC_BROWSER_TEST_F(ExperimentalApiTest, PermissionsSucceed) {
  ASSERT_TRUE(RunExtensionTest("permissions/enabled")) << message_;
}

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, ExperimentalPermissionsFail) {
  // At the time this test is being created, there is no experimental
  // function that will not be graduating soon, and does not require a
  // tab id as an argument.  So, we need the tab permission to get
  // a tab id.
  ASSERT_TRUE(RunExtensionTest("permissions/experimental_disabled"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, FaviconPermission) {
  ASSERT_TRUE(RunExtensionTest("permissions/favicon")) << message_;
}

// Test functions and APIs that are always allowed (even if you ask for no
// permissions).
// Disabled: http://crbug.com/125193
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, DISABLED_AlwaysAllowed) {
  ASSERT_TRUE(RunExtensionTest("permissions/always_allowed")) << message_;
}

// Tests that the optional permissions API works correctly.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, OptionalPermissionsGranted) {
  // Mark all the tested APIs as granted to bypass the confirmation UI.
  APIPermissionSet apis;
  apis.insert(APIPermission::kBookmark);
  ManifestPermissionSet manifest_permissions;
  URLPatternSet explicit_hosts;
  AddPattern(&explicit_hosts, "http://*.c.com/*");
  scoped_refptr<PermissionSet> granted_permissions =
      new PermissionSet(apis, manifest_permissions,
                        explicit_hosts, URLPatternSet());

  ExtensionPrefs* prefs = ExtensionPrefs::Get(browser()->profile());
  prefs->AddGrantedPermissions("kjmkgkdkpedkejedfhmfcenooemhbpbo",
                               granted_permissions.get());

  PermissionsRequestFunction::SetIgnoreUserGestureForTests(true);
  host_resolver()->AddRule("*.com", "127.0.0.1");
  ASSERT_TRUE(StartEmbeddedTestServer());
  EXPECT_TRUE(RunExtensionTest("permissions/optional")) << message_;
}

// Tests that the optional permissions API works correctly.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, OptionalPermissionsAutoConfirm) {
  // Rather than setting the granted permissions, set the UI autoconfirm flag
  // and run the same tests.
  PermissionsRequestFunction::SetAutoConfirmForTests(true);
  PermissionsRequestFunction::SetIgnoreUserGestureForTests(true);
  host_resolver()->AddRule("*.com", "127.0.0.1");
  ASSERT_TRUE(StartEmbeddedTestServer());
  EXPECT_TRUE(RunExtensionTest("permissions/optional")) << message_;
}

// Test that denying the optional permissions confirmation dialog works.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, OptionalPermissionsDeny) {
  PermissionsRequestFunction::SetAutoConfirmForTests(false);
  PermissionsRequestFunction::SetIgnoreUserGestureForTests(true);
  host_resolver()->AddRule("*.com", "127.0.0.1");
  ASSERT_TRUE(StartEmbeddedTestServer());
  EXPECT_TRUE(RunExtensionTest("permissions/optional_deny")) << message_;
}

// Tests that the permissions.request function must be called from within a
// user gesture.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, OptionalPermissionsGesture) {
  PermissionsRequestFunction::SetIgnoreUserGestureForTests(false);
  host_resolver()->AddRule("*.com", "127.0.0.1");
  ASSERT_TRUE(StartEmbeddedTestServer());
  EXPECT_TRUE(RunExtensionTest("permissions/optional_gesture")) << message_;
}

// Tests that the user gesture is retained in the permissions.request function
// callback.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, OptionalPermissionsRetainGesture) {
  PermissionsRequestFunction::SetAutoConfirmForTests(true);
  PermissionsRequestFunction::SetIgnoreUserGestureForTests(false);
  host_resolver()->AddRule("*.com", "127.0.0.1");
  ASSERT_TRUE(StartEmbeddedTestServer());
  EXPECT_TRUE(RunExtensionTest("permissions/optional_retain_gesture"))
      << message_;
}

// Test that optional permissions blocked by enterprise policy will be denied
// automatically.
IN_PROC_BROWSER_TEST_F(ExtensionApiTestWithManagementPolicy,
                       OptionalPermissionsPolicyBlocked) {
  // Set enterprise policy to block some API permissions.
  {
    ExtensionManagementPolicyUpdater pref(&policy_provider_);
    pref.AddBlockedPermission("*", "management");
  }
  // Set auto confirm UI flag.
  PermissionsRequestFunction::SetAutoConfirmForTests(true);
  PermissionsRequestFunction::SetIgnoreUserGestureForTests(true);
  EXPECT_TRUE(RunExtensionTest("permissions/optional_policy_blocked"))
      << message_;
}

// Tests that an extension can't gain access to file: URLs without the checkbox
// entry in prefs. There shouldn't be a warning either.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, OptionalPermissionsFileAccess) {
  // There shouldn't be a warning, so we shouldn't need to autoconfirm.
  PermissionsRequestFunction::SetAutoConfirmForTests(false);
  PermissionsRequestFunction::SetIgnoreUserGestureForTests(true);

  ExtensionPrefs* prefs = ExtensionPrefs::Get(browser()->profile());

  EXPECT_TRUE(
      RunExtensionTestNoFileAccess("permissions/file_access_no")) << message_;
  EXPECT_FALSE(prefs->AllowFileAccess("dgloelfbnddbdacakahpogklfdcccbib"));

  EXPECT_TRUE(RunExtensionTest("permissions/file_access_yes")) << message_;
  // TODO(kalman): ugh, it would be nice to test this condition, but it seems
  // like there's somehow a race here where the prefs aren't updated in time
  // with the "allow file access" bit, so we'll just have to trust that
  // RunExtensionTest (unlike RunExtensionTestNoFileAccess) does indeed
  // not set the allow file access bit. Otherwise this test doesn't mean
  // a whole lot (i.e. file access works - but it'd better not be the case
  // that the extension actually has file access, since that'd be the bug
  // that this is supposed to be testing).
  //EXPECT_TRUE(prefs->AllowFileAccess("hlonmbgfjccgolnaboonlakjckinmhmd"));
}

// Test requesting, querying, and removing host permissions for host
// permissions that are a subset of the optional permissions.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, HostSubsets) {
  PermissionsRequestFunction::SetAutoConfirmForTests(true);
  PermissionsRequestFunction::SetIgnoreUserGestureForTests(true);
  EXPECT_TRUE(RunExtensionTest("permissions/host_subsets")) << message_;
}

// Tests that requesting an optional permission from a background page, with
// another window open, grants the permission and updates the bindings
// (chrome.whatever, in this case chrome.alarms). Regression test for
// crbug.com/435141, see details there for trickiness.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, OptionalPermissionsUpdatesBindings) {
  ASSERT_TRUE(RunExtensionTest("permissions/optional_updates_bindings"))
      << message_;
}

}  // namespace extensions
