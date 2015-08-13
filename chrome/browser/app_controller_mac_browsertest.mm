// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <Foundation/NSAppleEventDescriptor.h>
#import <objc/message.h>
#import <objc/runtime.h>

#include "base/command_line.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/prefs/pref_service.h"
#include "base/run_loop.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#import "chrome/browser/app_controller_mac.h"
#include "chrome/browser/apps/app_browsertest_util.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/cocoa/bookmarks/bookmark_menu_bridge.h"
#include "chrome/browser/ui/cocoa/history_menu_bridge.h"
#include "chrome/browser/ui/cocoa/run_loop_testing.h"
#include "chrome/browser/ui/host_desktop.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/user_manager.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/signin/core/common/profile_management_switches.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "extensions/browser/app_window/app_window_registry.h"
#include "extensions/common/extension.h"
#include "extensions/test/extension_test_message_listener.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace {

GURL g_open_shortcut_url = GURL::EmptyGURL();

// Returns an Apple Event that instructs the application to open |url|.
NSAppleEventDescriptor* AppleEventToOpenUrl(const GURL& url) {
  NSAppleEventDescriptor* shortcut_event = [[[NSAppleEventDescriptor alloc]
      initWithEventClass:kASAppleScriptSuite
                 eventID:kASSubroutineEvent
        targetDescriptor:nil
                returnID:kAutoGenerateReturnID
           transactionID:kAnyTransactionID] autorelease];
  NSString* url_string = [NSString stringWithUTF8String:url.spec().c_str()];
  [shortcut_event setParamDescriptor:[NSAppleEventDescriptor
                                         descriptorWithString:url_string]
                          forKeyword:keyDirectObject];
  return shortcut_event;
}

// Instructs the NSApp's delegate to open |url|.
void SendAppleEventToOpenUrlToAppController(const GURL& url) {
  AppController* controller =
      base::mac::ObjCCast<AppController>([NSApp delegate]);
  Method get_url =
      class_getInstanceMethod([controller class], @selector(getUrl:withReply:));

  ASSERT_TRUE(get_url);

  NSAppleEventDescriptor* shortcut_event = AppleEventToOpenUrl(url);

  method_invoke(controller, get_url, shortcut_event, NULL);
}

void RunClosureWhenProfileInitialized(const base::Closure& closure,
                                      Profile* profile,
                                      Profile::CreateStatus status) {
  if (status == Profile::CREATE_STATUS_INITIALIZED)
    closure.Run();
}

}  // namespace

@interface TestOpenShortcutOnStartup : NSObject
- (void)applicationWillFinishLaunching:(NSNotification*)notification;
@end

@implementation TestOpenShortcutOnStartup

- (void)applicationWillFinishLaunching:(NSNotification*)notification {
  if (!g_open_shortcut_url.is_valid())
    return;

  SendAppleEventToOpenUrlToAppController(g_open_shortcut_url);
}

@end

namespace {

class AppControllerPlatformAppBrowserTest
    : public extensions::PlatformAppBrowserTest {
 protected:
  AppControllerPlatformAppBrowserTest()
      : active_browser_list_(BrowserList::GetInstance(
                                chrome::GetActiveDesktop())) {
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    PlatformAppBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(switches::kAppId,
                                    "1234");
  }

  const BrowserList* active_browser_list_;
};

// Test that if only a platform app window is open and no browser windows are
// open then a reopen event does nothing.
IN_PROC_BROWSER_TEST_F(AppControllerPlatformAppBrowserTest,
                       PlatformAppReopenWithWindows) {
  base::scoped_nsobject<AppController> ac([[AppController alloc] init]);
  NSUInteger old_window_count = [[NSApp windows] count];
  EXPECT_EQ(1u, active_browser_list_->size());
  [ac applicationShouldHandleReopen:NSApp hasVisibleWindows:YES];
  // We do not EXPECT_TRUE the result here because the method
  // deminiaturizes windows manually rather than return YES and have
  // AppKit do it.

  EXPECT_EQ(old_window_count, [[NSApp windows] count]);
  EXPECT_EQ(1u, active_browser_list_->size());
}

IN_PROC_BROWSER_TEST_F(AppControllerPlatformAppBrowserTest,
                       ActivationFocusesBrowserWindow) {
  base::scoped_nsobject<AppController> app_controller(
      [[AppController alloc] init]);

  ExtensionTestMessageListener listener("Launched", false);
  const extensions::Extension* app =
      InstallAndLaunchPlatformApp("minimal");
  ASSERT_TRUE(listener.WaitUntilSatisfied());

  NSWindow* app_window = extensions::AppWindowRegistry::Get(profile())
                             ->GetAppWindowsForApp(app->id())
                             .front()
                             ->GetNativeWindow();
  NSWindow* browser_window = browser()->window()->GetNativeWindow();

  chrome::testing::NSRunLoopRunAllPending();
  EXPECT_LE([[NSApp orderedWindows] indexOfObject:app_window],
            [[NSApp orderedWindows] indexOfObject:browser_window]);
  [app_controller applicationShouldHandleReopen:NSApp
                              hasVisibleWindows:YES];
  chrome::testing::NSRunLoopRunAllPending();
  EXPECT_LE([[NSApp orderedWindows] indexOfObject:browser_window],
            [[NSApp orderedWindows] indexOfObject:app_window]);
}

class AppControllerWebAppBrowserTest : public InProcessBrowserTest {
 protected:
  AppControllerWebAppBrowserTest()
      : active_browser_list_(BrowserList::GetInstance(
                                chrome::GetActiveDesktop())) {
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(switches::kApp, GetAppURL());
  }

  std::string GetAppURL() const {
    return "http://example.com/";
  }

  const BrowserList* active_browser_list_;
};

// Test that in web app mode a reopen event opens the app URL.
IN_PROC_BROWSER_TEST_F(AppControllerWebAppBrowserTest,
                       WebAppReopenWithNoWindows) {
  base::scoped_nsobject<AppController> ac([[AppController alloc] init]);
  EXPECT_EQ(1u, active_browser_list_->size());
  BOOL result = [ac applicationShouldHandleReopen:NSApp hasVisibleWindows:NO];

  EXPECT_FALSE(result);
  EXPECT_EQ(2u, active_browser_list_->size());

  Browser* browser = active_browser_list_->get(0);
  GURL current_url =
      browser->tab_strip_model()->GetActiveWebContents()->GetURL();
  EXPECT_EQ(GetAppURL(), current_url.spec());
}

// Called when the ProfileManager has created a profile.
void CreateProfileCallback(const base::Closure& quit_closure,
                           Profile* profile,
                           Profile::CreateStatus status) {
  EXPECT_TRUE(profile);
  EXPECT_NE(Profile::CREATE_STATUS_LOCAL_FAIL, status);
  EXPECT_NE(Profile::CREATE_STATUS_REMOTE_FAIL, status);
  // This will be called multiple times. Wait until the profile is initialized
  // fully to quit the loop.
  if (status == Profile::CREATE_STATUS_INITIALIZED)
    quit_closure.Run();
}

void CreateAndWaitForSystemProfile() {
  ProfileManager::CreateCallback create_callback =
      base::Bind(&CreateProfileCallback,
                 base::MessageLoop::current()->QuitClosure());
  g_browser_process->profile_manager()->CreateProfileAsync(
      ProfileManager::GetSystemProfilePath(),
      create_callback,
      base::string16(),
      base::string16(),
      std::string());
  base::RunLoop().Run();
}

class AppControllerNewProfileManagementBrowserTest
    : public InProcessBrowserTest {
 protected:
  AppControllerNewProfileManagementBrowserTest()
      : active_browser_list_(BrowserList::GetInstance(
                                chrome::GetActiveDesktop())) {
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    switches::EnableNewProfileManagementForTesting(command_line);
  }

  const BrowserList* active_browser_list_;
};

// Test that for a regular last profile, a reopen event opens a browser.
IN_PROC_BROWSER_TEST_F(AppControllerNewProfileManagementBrowserTest,
                       RegularProfileReopenWithNoWindows) {
  base::scoped_nsobject<AppController> ac([[AppController alloc] init]);
  EXPECT_EQ(1u, active_browser_list_->size());
  BOOL result = [ac applicationShouldHandleReopen:NSApp hasVisibleWindows:NO];

  EXPECT_FALSE(result);
  EXPECT_EQ(2u, active_browser_list_->size());
  EXPECT_FALSE(UserManager::IsShowing());
}

// Test that for a locked last profile, a reopen event opens the User Manager.
IN_PROC_BROWSER_TEST_F(AppControllerNewProfileManagementBrowserTest,
                       LockedProfileReopenWithNoWindows) {
  // The User Manager uses the system profile as its underlying profile. To
  // minimize flakiness due to the scheduling/descheduling of tasks on the
  // different threads, pre-initialize the guest profile before it is needed.
  CreateAndWaitForSystemProfile();
  base::scoped_nsobject<AppController> ac([[AppController alloc] init]);

  // Lock the active profile.
  Profile* profile = [ac lastProfile];
  ProfileInfoCache& cache =
      g_browser_process->profile_manager()->GetProfileInfoCache();
  size_t profile_index = cache.GetIndexOfProfileWithPath(profile->GetPath());
  cache.SetProfileSigninRequiredAtIndex(profile_index, true);
  EXPECT_TRUE(cache.ProfileIsSigninRequiredAtIndex(profile_index));

  EXPECT_EQ(1u, active_browser_list_->size());
  BOOL result = [ac applicationShouldHandleReopen:NSApp hasVisibleWindows:NO];
  EXPECT_FALSE(result);

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1u, active_browser_list_->size());
  EXPECT_TRUE(UserManager::IsShowing());
  UserManager::Hide();
}

// Test that for a guest last profile, a reopen event opens the User Manager.
IN_PROC_BROWSER_TEST_F(AppControllerNewProfileManagementBrowserTest,
                       GuestProfileReopenWithNoWindows) {
  // Create the system profile. Set the guest as the last used profile so the
  // app controller can use it on init.
  CreateAndWaitForSystemProfile();
  PrefService* local_state = g_browser_process->local_state();
  local_state->SetString(prefs::kProfileLastUsed, chrome::kGuestProfileDir);

  base::scoped_nsobject<AppController> ac([[AppController alloc] init]);

  Profile* profile = [ac lastProfile];
  EXPECT_EQ(ProfileManager::GetGuestProfilePath(), profile->GetPath());
  EXPECT_TRUE(profile->IsGuestSession());

  EXPECT_EQ(1u, active_browser_list_->size());
  BOOL result = [ac applicationShouldHandleReopen:NSApp hasVisibleWindows:NO];
  EXPECT_FALSE(result);

  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(1u, active_browser_list_->size());
  EXPECT_TRUE(UserManager::IsShowing());
  UserManager::Hide();
}

IN_PROC_BROWSER_TEST_F(AppControllerNewProfileManagementBrowserTest,
                       AboutChromeForcesUserManager) {
  base::scoped_nsobject<AppController> ac([[AppController alloc] init]);

  // Create the guest profile, and set it as the last used profile so the
  // app controller can use it on init.
  CreateAndWaitForSystemProfile();
  PrefService* local_state = g_browser_process->local_state();
  local_state->SetString(prefs::kProfileLastUsed, chrome::kGuestProfileDir);

  // Prohibiting guest mode forces the user manager flow for About Chrome.
  local_state->SetBoolean(prefs::kBrowserGuestModeEnabled, false);

  Profile* guest_profile = [ac lastProfile];
  EXPECT_EQ(ProfileManager::GetGuestProfilePath(), guest_profile->GetPath());
  EXPECT_TRUE(guest_profile->IsGuestSession());

  // Tell the browser to open About Chrome.
  EXPECT_EQ(1u, active_browser_list_->size());
  [ac orderFrontStandardAboutPanel:NSApp];

  base::RunLoop().RunUntilIdle();

  // No new browser is opened; the User Manager opens instead.
  EXPECT_EQ(1u, active_browser_list_->size());
  EXPECT_TRUE(UserManager::IsShowing());

  UserManager::Hide();
}

class AppControllerOpenShortcutBrowserTest : public InProcessBrowserTest {
 protected:
  AppControllerOpenShortcutBrowserTest() {
  }

  void SetUpInProcessBrowserTestFixture() override {
    // In order to mimic opening shortcut during browser startup, we need to
    // send the event before -applicationDidFinishLaunching is called, but
    // after AppController is loaded.
    //
    // Since -applicationWillFinishLaunching does nothing now, we swizzle it to
    // our function to send the event. We need to do this early before running
    // the main message loop.
    //
    // NSApp does not exist yet. We need to get the AppController using
    // reflection.
    Class appControllerClass = NSClassFromString(@"AppController");
    Class openShortcutClass = NSClassFromString(@"TestOpenShortcutOnStartup");

    ASSERT_TRUE(appControllerClass != nil);
    ASSERT_TRUE(openShortcutClass != nil);

    SEL targetMethod = @selector(applicationWillFinishLaunching:);
    Method original = class_getInstanceMethod(appControllerClass,
        targetMethod);
    Method destination = class_getInstanceMethod(openShortcutClass,
        targetMethod);

    ASSERT_TRUE(original != NULL);
    ASSERT_TRUE(destination != NULL);

    method_exchangeImplementations(original, destination);

    ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());
    g_open_shortcut_url = embedded_test_server()->GetURL("/simple.html");
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // If the arg is empty, PrepareTestCommandLine() after this function will
    // append about:blank as default url.
    command_line->AppendArg(chrome::kChromeUINewTabURL);
  }
};

IN_PROC_BROWSER_TEST_F(AppControllerOpenShortcutBrowserTest,
                       OpenShortcutOnStartup) {
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_EQ(g_open_shortcut_url,
      browser()->tab_strip_model()->GetActiveWebContents()
          ->GetLastCommittedURL());
}

class AppControllerReplaceNTPBrowserTest : public InProcessBrowserTest {
 protected:
  AppControllerReplaceNTPBrowserTest() {}

  void SetUpInProcessBrowserTestFixture() override {
    ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // If the arg is empty, PrepareTestCommandLine() after this function will
    // append about:blank as default url.
    command_line->AppendArg(chrome::kChromeUINewTabURL);
  }
};

// Tests that when a GURL is opened after startup, it replaces the NTP.
IN_PROC_BROWSER_TEST_F(AppControllerReplaceNTPBrowserTest,
                       ReplaceNTPAfterStartup) {
  // Ensure that there is exactly 1 tab showing, and the tab is the NTP.
  GURL ntp(chrome::kChromeUINewTabURL);
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_EQ(ntp,
            browser()
                ->tab_strip_model()
                ->GetActiveWebContents()
                ->GetLastCommittedURL());

  GURL simple(embedded_test_server()->GetURL("/simple.html"));
  SendAppleEventToOpenUrlToAppController(simple);

  // Wait for one navigation on the active web contents.
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  content::TestNavigationObserver obs(
      browser()->tab_strip_model()->GetActiveWebContents(), 1);
  obs.Wait();

  EXPECT_EQ(simple,
            browser()
                ->tab_strip_model()
                ->GetActiveWebContents()
                ->GetLastCommittedURL());
}

class AppControllerMainMenuBrowserTest : public InProcessBrowserTest {
 protected:
  AppControllerMainMenuBrowserTest() {
  }
};

IN_PROC_BROWSER_TEST_F(AppControllerMainMenuBrowserTest,
    HistoryMenuResetAfterProfileDeletion) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  AppController* ac = [NSApp delegate];

  // Use the existing profile as profile 1.
  Profile* profile1 = browser()->profile();

  // Create profile 2.
  base::FilePath profile2_path =
      profile_manager->GenerateNextProfileDirectoryPath();
  base::RunLoop run_loop;
  profile_manager->CreateProfileAsync(
      profile2_path,
      base::Bind(&RunClosureWhenProfileInitialized,
                 run_loop.QuitClosure()),
                 base::string16(),
                 base::string16(),
                 std::string());
  run_loop.Run();
  Profile* profile2 = profile_manager->GetProfileByPath(profile2_path);
  ASSERT_TRUE(profile2);

  // Switch the controller to profile1.
  [ac windowChangedToProfile:profile1];
  base::RunLoop().RunUntilIdle();

  // Verify the controller's History Menu corresponds to profile1.
  EXPECT_TRUE([ac historyMenuBridge]->service());
  EXPECT_EQ([ac historyMenuBridge]->service(),
      HistoryServiceFactory::GetForProfile(profile1,
                                           ServiceAccessType::EXPLICIT_ACCESS));

  // Load profile2's History Service backend so it will be assigned to the
  // HistoryMenuBridge when windowChangedToProfile is called, or else this test
  // will fail flaky.
  ui_test_utils::WaitForHistoryToLoad(
      HistoryServiceFactory::GetForProfile(profile2,
                                           ServiceAccessType::EXPLICIT_ACCESS));
  // Switch the controller to profile2.
  [ac windowChangedToProfile:profile2];
  base::RunLoop().RunUntilIdle();

  // Verify the controller's History Menu has changed.
  EXPECT_TRUE([ac historyMenuBridge]->service());
  EXPECT_EQ([ac historyMenuBridge]->service(),
      HistoryServiceFactory::GetForProfile(profile2,
                                           ServiceAccessType::EXPLICIT_ACCESS));
  EXPECT_NE(
      HistoryServiceFactory::GetForProfile(profile1,
                                           ServiceAccessType::EXPLICIT_ACCESS),
      HistoryServiceFactory::GetForProfile(profile2,
                                           ServiceAccessType::EXPLICIT_ACCESS));

  // Delete profile2.
  profile_manager->ScheduleProfileForDeletion(
      profile2->GetPath(), ProfileManager::CreateCallback());
  base::RunLoop().RunUntilIdle();

  // Verify the controller's history is back to profile1.
  EXPECT_EQ([ac historyMenuBridge]->service(),
      HistoryServiceFactory::GetForProfile(profile1,
                                           ServiceAccessType::EXPLICIT_ACCESS));
}

IN_PROC_BROWSER_TEST_F(AppControllerMainMenuBrowserTest,
    BookmarksMenuIsRestoredAfterProfileSwitch) {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  base::scoped_nsobject<AppController> ac([[AppController alloc] init]);
  [ac awakeFromNib];

  // Constants for bookmarks that we will create later.
  const base::string16 title1(base::ASCIIToUTF16("Dinosaur Comics"));
  const GURL url1("http://qwantz.com//");

  const base::string16 title2(base::ASCIIToUTF16("XKCD"));
  const GURL url2("https://www.xkcd.com/");

  // Use the existing profile as profile 1.
  Profile* profile1 = browser()->profile();
  bookmarks::test::WaitForBookmarkModelToLoad(
      BookmarkModelFactory::GetForProfile(profile1));

  // Create profile 2.
  base::FilePath path2 = profile_manager->GenerateNextProfileDirectoryPath();
  Profile* profile2 =
      Profile::CreateProfile(path2, NULL, Profile::CREATE_MODE_SYNCHRONOUS);
  profile_manager->RegisterTestingProfile(profile2, false, true);
  bookmarks::test::WaitForBookmarkModelToLoad(
      BookmarkModelFactory::GetForProfile(profile2));

  // Switch to profile 1, create bookmark 1 and force the menu to build.
  [ac windowChangedToProfile:profile1];
  [ac bookmarkMenuBridge]->GetBookmarkModel()->AddURL(
      [ac bookmarkMenuBridge]->GetBookmarkModel()->bookmark_bar_node(),
      0, title1, url1);
  [ac bookmarkMenuBridge]->BuildMenu();

  // Switch to profile 2, create bookmark 2 and force the menu to build.
  [ac windowChangedToProfile:profile2];
  [ac bookmarkMenuBridge]->GetBookmarkModel()->AddURL(
      [ac bookmarkMenuBridge]->GetBookmarkModel()->bookmark_bar_node(),
      0, title2, url2);
  [ac bookmarkMenuBridge]->BuildMenu();

  // Test that only bookmark 2 is shown.
  EXPECT_FALSE([[ac bookmarkMenuBridge]->BookmarkMenu() itemWithTitle:
      SysUTF16ToNSString(title1)]);
  EXPECT_TRUE([[ac bookmarkMenuBridge]->BookmarkMenu() itemWithTitle:
      SysUTF16ToNSString(title2)]);

  // Switch *back* to profile 1 and *don't* force the menu to build.
  [ac windowChangedToProfile:profile1];

  // Test that only bookmark 1 is shown in the restored menu.
  EXPECT_TRUE([[ac bookmarkMenuBridge]->BookmarkMenu() itemWithTitle:
      SysUTF16ToNSString(title1)]);
  EXPECT_FALSE([[ac bookmarkMenuBridge]->BookmarkMenu() itemWithTitle:
      SysUTF16ToNSString(title2)]);
}

}  // namespace

//--------------------------AppControllerHandoffBrowserTest---------------------

static GURL g_handoff_url;

@interface AppController (BrowserTest)
- (BOOL)new_shouldUseHandoff;
- (void)new_passURLToHandoffManager:(const GURL&)handoffURL;
@end

@implementation AppController (BrowserTest)
- (BOOL)new_shouldUseHandoff {
  return YES;
}

- (void)new_passURLToHandoffManager:(const GURL&)handoffURL {
  g_handoff_url = handoffURL;
}
@end

namespace {

class AppControllerHandoffBrowserTest : public InProcessBrowserTest {
 protected:
  AppControllerHandoffBrowserTest() {}

  // Exchanges the implementations of the two selectors on the class
  // AppController.
  void ExchangeSelectors(SEL originalMethod, SEL newMethod) {
    Class appControllerClass = NSClassFromString(@"AppController");

    ASSERT_TRUE(appControllerClass != nil);

    Method original =
        class_getInstanceMethod(appControllerClass, originalMethod);
    Method destination = class_getInstanceMethod(appControllerClass, newMethod);

    ASSERT_TRUE(original != NULL);
    ASSERT_TRUE(destination != NULL);

    method_exchangeImplementations(original, destination);
  }

  // Swizzle Handoff related implementations.
  void SetUpInProcessBrowserTestFixture() override {
    // Handoff is only available on OSX 10.10+. This swizzle makes the logic
    // run on all OSX versions.
    SEL originalMethod = @selector(shouldUseHandoff);
    SEL newMethod = @selector(new_shouldUseHandoff);
    ExchangeSelectors(originalMethod, newMethod);

    // This swizzle intercepts the URL that would be sent to the Handoff
    // Manager, and instead puts it into a variable accessible to this test.
    originalMethod = @selector(passURLToHandoffManager:);
    newMethod = @selector(new_passURLToHandoffManager:);
    ExchangeSelectors(originalMethod, newMethod);
  }

  // Closes the tab, and waits for the close to finish.
  void CloseTab(Browser* browser, int index) {
    content::WebContentsDestroyedWatcher destroyed_watcher(
        browser->tab_strip_model()->GetWebContentsAt(index));
    browser->tab_strip_model()->CloseWebContentsAt(
        index, TabStripModel::CLOSE_CREATE_HISTORICAL_TAB);
    destroyed_watcher.Wait();
  }
};

// Tests that as a user switches between tabs, navigates within a tab, and
// switches between browser windows, the correct URL is being passed to the
// Handoff.
IN_PROC_BROWSER_TEST_F(AppControllerHandoffBrowserTest, TestHandoffURLs) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());
  EXPECT_EQ(g_handoff_url, GURL(url::kAboutBlankURL));

  // Test that navigating to a URL updates the handoff URL.
  GURL test_url1 = embedded_test_server()->GetURL("/title1.html");
  ui_test_utils::NavigateToURL(browser(), test_url1);
  EXPECT_EQ(g_handoff_url, test_url1);

  // Test that opening a new tab updates the handoff URL.
  GURL test_url2 = embedded_test_server()->GetURL("/title2.html");
  chrome::NavigateParams params(browser(), test_url2, ui::PAGE_TRANSITION_LINK);
  params.disposition = NEW_FOREGROUND_TAB;
  ui_test_utils::NavigateToURL(&params);
  EXPECT_EQ(g_handoff_url, test_url2);

  // Test that switching tabs updates the handoff URL.
  browser()->tab_strip_model()->ActivateTabAt(0, true);
  EXPECT_EQ(g_handoff_url, test_url1);

  // Test that closing the current tab updates the handoff URL.
  CloseTab(browser(), 0);
  EXPECT_EQ(g_handoff_url, test_url2);

  // Test that opening a new browser window updates the handoff URL.
  GURL test_url3 = embedded_test_server()->GetURL("/title3.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(test_url3), NEW_WINDOW,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_BROWSER);
  EXPECT_EQ(g_handoff_url, test_url3);

  // Check that there are exactly 2 browsers.
  BrowserList* active_browser_list =
      BrowserList::GetInstance(chrome::GetActiveDesktop());
  EXPECT_EQ(2u, active_browser_list->size());

  // Close the second browser window (which only has 1 tab left).
  Browser* browser2 = active_browser_list->get(1);
  CloseBrowserSynchronously(browser2);
  EXPECT_EQ(g_handoff_url, test_url2);

  // The URLs of incognito windows should not be passed to Handoff.
  GURL test_url4 = embedded_test_server()->GetURL("/simple.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(test_url4), OFF_THE_RECORD,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_BROWSER);
  EXPECT_EQ(g_handoff_url, GURL());

  // Open a new tab in the incognito window.
  EXPECT_EQ(2u, active_browser_list->size());
  Browser* browser3 = active_browser_list->get(1);
  ui_test_utils::NavigateToURLWithDisposition(
      browser3, test_url4, NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB);
  EXPECT_EQ(g_handoff_url, GURL());

  // Navigate the current tab in the incognito window.
  ui_test_utils::NavigateToURL(browser3, test_url1);
  EXPECT_EQ(g_handoff_url, GURL());

  // Activate the original browser window.
  Browser* browser1 = active_browser_list->get(0);
  browser1->window()->Show();
  EXPECT_EQ(g_handoff_url, test_url2);
}

}  // namespace
