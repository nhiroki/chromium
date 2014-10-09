// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_REMOTING_REMOTE_DESKTOP_BROWSERTEST_H_
#define CHROME_TEST_REMOTING_REMOTE_DESKTOP_BROWSERTEST_H_

#include "base/debug/stack_trace.h"
#include "chrome/browser/apps/app_browsertest_util.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

namespace {
// Command line arguments specific to the chromoting browser tests.
const char kOverrideUserDataDir[] = "override-user-data-dir";
const char kNoCleanup[] = "no-cleanup";
const char kNoInstall[] = "no-install";
const char kWebAppCrx[] = "webapp-crx";
const char kWebAppUnpacked[] = "webapp-unpacked";
const char kUsername[] = "username";
const char kkPassword[] = "password";
const char kMe2MePin[] = "me2me-pin";
const char kRemoteHostName[] = "remote-host-name";
const char kExtensionName[] = "extension-name";
const char kHttpServer[] = "http-server";

// ASSERT_TRUE can only be used in void returning functions. This version
// should be used in non-void-returning functions.
inline void _ASSERT_TRUE(bool condition) {
  if (!condition) {
    // ASSERT_TRUE only prints the first call frame in the error message.
    // In our case, this is the _ASSERT_TRUE wrapper function, which is not
    // useful.  To help with debugging, we will dump the full callstack.
    LOG(ERROR) << "Assertion failed.";
    LOG(ERROR) << base::debug::StackTrace().ToString();
  }
  ASSERT_TRUE(condition);
  return;
}

}  // namespace

using extensions::Extension;

namespace remoting {

class RemoteDesktopBrowserTest : public extensions::PlatformAppBrowserTest {
 public:
  RemoteDesktopBrowserTest();
  virtual ~RemoteDesktopBrowserTest();

  // InProcessBrowserTest Overrides
  virtual void SetUp() override;
  virtual void SetUpOnMainThread() override;

 protected:
  // InProcessBrowserTest Overrides
  virtual void SetUpInProcessBrowserTestFixture() override;

  // InProcessBrowserTest Overrides
  virtual void TearDownInProcessBrowserTestFixture() override;

  // The following helpers each perform a simple task.

  // Verify the test has access to the internet (specifically google.com)
  void VerifyInternetAccess();

  // Open the client page for the browser test to get status of host actions
  void OpenClientBrowserPage();

  // Install the chromoting extension from a crx file.
  void InstallChromotingAppCrx();

  // Install the unpacked chromoting extension.
  void InstallChromotingAppUnpacked();

  // Uninstall the chromoting extension.
  void UninstallChromotingApp();

  // Test whether the chromoting extension is installed.
  void VerifyChromotingLoaded(bool expected);

  // Launch the chromoting app.
  void LaunchChromotingApp();

  // Authorize: grant extended access permission to the user's computer.
  void Authorize();

  // Authenticate: sign in to google using the credentials provided.
  void Authenticate();

  // Approve: grant the chromoting app necessary permissions.
  void Approve();

  // Click on "Get Started" in the Me2Me section and show the host list.
  void ExpandMe2Me();

  // Disconnect the active Me2Me session.
  void DisconnectMe2Me();

  // Simulate a key event.
  void SimulateKeyPressWithCode(ui::KeyboardCode keyCode, const char* code);

  void SimulateKeyPressWithCode(ui::KeyboardCode keyCode,
                                const char* code,
                                bool control,
                                bool shift,
                                bool alt,
                                bool command);

  // Simulate typing a character
  void SimulateCharInput(char c);

  // Simulate typing a string
  void SimulateStringInput(const std::string& input);

  // Helper to simulate a left button mouse click.
  void SimulateMouseLeftClickAt(int x, int y);

  // Helper to simulate a mouse click.
  void SimulateMouseClickAt(
      int modifiers, blink::WebMouseEvent::Button button, int x, int y);

  // The following helpers each perform a composite task.

  // Install the chromoting extension
  void Install();

  // Perform all necessary steps (installation, authorization, authentication,
  // expanding the me2me section) so that the app is ready for a me2me
  // connection.
  void SetUpTestForMe2Me();

  // Clean up after the test.
  void Cleanup();

  // Perform all the auth steps: authorization, authentication, etc.
  // It starts from the chromoting main page unauthenticated and ends up back
  // on the chromoting main page authenticated and ready to go.
  void Auth();

  // Ensures that the host is started locally with |me2me_pin()|.
  // Browser_test.js must be loaded before calling this function.
  void EnsureRemoteConnectionEnabled();

  // Connect to the local host through Me2Me.
  void ConnectToLocalHost(bool remember_pin);

  // Connect to a remote host through Me2Me.
  void ConnectToRemoteHost(const std::string& host_name, bool remember_pin);

  // Enter the pin number and connect.
  void EnterPin(const std::string& name, bool remember_pin);

  // Helper to get the pin number used for me2me authentication.
  std::string me2me_pin() { return me2me_pin_; }

  // Helper to get the name of the remote host to connect to.
  std::string remote_host_name() { return remote_host_name_; }

  // Helper to get the test controller URL.
  std::string http_server() { return http_server_; }

  // Change behavior of the default host resolver to allow DNS lookup
  // to proceed instead of being blocked by the test infrastructure.
  void EnableDNSLookupForThisTest(
    net::RuleBasedHostResolverProc* host_resolver);

  // We need to reset the DNS lookup when we finish, or the test will fail.
  void DisableDNSLookupForThisTest();

  void ParseCommandLine();

  // Accessor methods.

  // Helper to get the path to the crx file of the webapp to be tested.
  base::FilePath WebAppCrxPath() { return webapp_crx_; }

  // Helper to get the extension ID of the installed chromoting webapp.
  std::string ChromotingID() { return extension_->id(); }

  // Is this a appsv2 web app?
  bool is_platform_app() {
    return extension_->GetType() == extensions::Manifest::TYPE_PLATFORM_APP;
  }

  // Are we testing an unpacked extension?
  bool is_unpacked() {
    return !webapp_unpacked_.empty();
  }

  // The "active" WebContents instance the test needs to interact with.
  content::WebContents* active_web_contents() {
    DCHECK(!web_contents_stack_.empty());
    return web_contents_stack_.back();
  }

  // The client WebContents instance the test needs to interact with.
  content::WebContents* client_web_content() {
    return client_web_content_;
  }

  content::WebContents* app_web_content() {
    return app_web_content_;
  }

  // Whether to perform the cleanup tasks (uninstalling chromoting, etc).
  // This is useful for diagnostic purposes.
  bool NoCleanup() { return no_cleanup_; }

  // Whether to install the chromoting extension before running the test cases.
  // This is useful for diagnostic purposes.
  bool NoInstall() { return no_install_; }

  // Helper to construct the starting URL of the installed chromoting webapp.
  GURL Chromoting_Main_URL() {
    return GURL("chrome-extension://" + ChromotingID() + "/main.html");
  }

  // Helper to retrieve the current URL in the active WebContents.
  GURL GetCurrentURL() {
    return active_web_contents()->GetURL();
  }

  // Helpers to execute JavaScript code on a web page.

  // Helper to execute a JavaScript code snippet in the active WebContents.
  void ExecuteScript(const std::string& script);

  // Helper to execute a JavaScript code snippet in the active WebContents
  // and wait for page load to complete.
  void ExecuteScriptAndWaitForAnyPageLoad(const std::string& script);

  // Helper to execute a JavaScript code snippet in the active WebContents
  // and extract the boolean result.
  bool ExecuteScriptAndExtractBool(const std::string& script) {
    return ExecuteScriptAndExtractBool(active_web_contents(), script);
  }

  // Helper to execute a JavaScript code snippet and extract the boolean result.
  static bool ExecuteScriptAndExtractBool(content::WebContents* web_contents,
                                          const std::string& script);

  // Helper to execute a JavaScript code snippet in the active WebContents
  // and extract the int result.
  int ExecuteScriptAndExtractInt(const std::string& script) {
    return ExecuteScriptAndExtractInt(active_web_contents(), script);
  }

  // Helper to execute a JavaScript code snippet and extract the int result.
  static int ExecuteScriptAndExtractInt(content::WebContents* web_contents,
                                        const std::string& script);

  // Helper to execute a JavaScript code snippet in the active WebContents
  // and extract the string result.
  std::string ExecuteScriptAndExtractString(const std::string& script) {
    return ExecuteScriptAndExtractString(active_web_contents(), script);
  }

  // Helper to execute a JavaScript code snippet and extract the string result.
  static std::string ExecuteScriptAndExtractString(
      content::WebContents* web_contents, const std::string& script);

  // Helper to load a JavaScript file from |path| and inject it to
  // current web_content.  The variable |path| is relative to the directory of
  // the |browsertest| executable.
  static bool LoadScript(content::WebContents* web_contents,
                         const base::FilePath::StringType& path);

  // Helper to execute a JavaScript browser test.  It creates an object using
  // the |browserTest.testName| ctor and calls |run| on the created object with
  // |testData|, which can be any arbitrary object literal. The script
  // browser_test.js must be loaded (using LoadScript) before calling this
  // function.
  void RunJavaScriptTest(content::WebContents* web_contents,
                         const std::string& testName,
                         const std::string& testData);

  // Helper to check whether an html element with the given name exists in
  // the active WebContents.
  bool HtmlElementExists(const std::string& name) {
    return ExecuteScriptAndExtractBool(
        "document.getElementById(\"" + name + "\") != null");
  }

  // Helper to check whether a html element with the given name is visible in
  // the active WebContents.
  bool HtmlElementVisible(const std::string& name);

  // Click on the named HTML control in the active WebContents.
  void ClickOnControl(const std::string& name);

  // Wait for the me2me connection to be established.
  void WaitForConnection();

  // Checking whether the localHost has been initialized.
  bool IsLocalHostReady();

  // Callback used by EnterPin to check whether the pin form is visible
  // and to dismiss the host-needs-update dialog.
  bool IsPinFormVisible();

  // Callback used by WaitForConnection to check whether the connection
  // has been established.
  bool IsSessionConnected();

  // Callback used by Approve to check whether the chromoting app has
  // successfully authenticated with the Google services.
  bool IsAuthenticated() {
      return IsAuthenticatedInWindow(active_web_contents());
  }

  // Callback used by Approve to check whether the Accept button is enabled
  // and ready to receive a click.
  static bool IsEnabled(
      content::WebContents* web_contents, const std::string& name);

  // If the "Host version out-of-date" form is visible, dismiss it.
  void DismissHostVersionWarningIfVisible();

  // Callback used by Approve to check whether the chromoting app has
  // successfully authenticated with the Google services.
  static bool IsAuthenticatedInWindow(content::WebContents* web_contents);

  // Callback used to check whether a host action is completed.
  // Used by browser tests while conditionally waiting for host actions.
  static bool IsHostActionComplete(
      content::WebContents* client_web_content, std::string host_action_var);

 private:
  // Fields

  // This test needs to make live DNS requests for access to
  // GAIA and sync server URLs under google.com. We use a scoped version
  // to override the default resolver while the test is active.
  scoped_ptr<net::ScopedDefaultHostResolverProc> mock_host_resolver_override_;

  // Stores all the WebContents instance in a stack so that we can easily
  // return to the previous instance.
  // The active WebContents instance is always stored at the top of the stack.
  // Initially the stack contains the WebContents instance created by
  // InProcessBrowserTest as the initial context to run test in.
  // Whenever a WebContents instance is spawned and needs attention we
  // push it onto the stack and that becomes the active instance.
  // And once we are done with the current WebContents instance
  // we pop it off the stack, returning to the previous instance.
  std::vector<content::WebContents*> web_contents_stack_;

  // WebContent of the client page that facilitates communication with
  // the HTTP server. This is how the remoting browser tests
  // will get acknowledgments of actions completed on the host.
  content::WebContents* client_web_content_;

  // WebContent of the landing page in the chromoting app.
  content::WebContents* app_web_content_;

  bool no_cleanup_;
  bool no_install_;
  const Extension* extension_;
  base::FilePath webapp_crx_;
  base::FilePath webapp_unpacked_;
  std::string username_;
  std::string password_;
  std::string me2me_pin_;
  std::string remote_host_name_;
  std::string extension_name_;
  std::string http_server_;
};

}  // namespace remoting

#endif  // CHROME_TEST_REMOTING_REMOTE_DESKTOP_BROWSERTEST_H_
