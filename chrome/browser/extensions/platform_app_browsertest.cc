// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/test/test_timeouts.h"
#include "base/threading/platform_thread.h"
#include "base/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/automation/automation_util.h"
#include "chrome/browser/tab_contents/render_view_context_menu.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_test_message_listener.h"
#include "chrome/browser/extensions/platform_app_browsertest_util.h"
#include "chrome/browser/extensions/platform_app_launcher.h"
#include "chrome/browser/extensions/shell_window_registry.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/browser/ui/extensions/shell_window.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_intents_dispatcher.h"
#include "googleurl/src/gurl.h"
#include "webkit/glue/web_intent_data.h"

using content::WebContents;

namespace extensions {

namespace {

// Non-abstract RenderViewContextMenu class.
class PlatformAppContextMenu : public RenderViewContextMenu {
 public:
  PlatformAppContextMenu(WebContents* web_contents,
                         const content::ContextMenuParams& params)
      : RenderViewContextMenu(web_contents, params) {}

  bool HasCommandWithId(int command_id) {
    return menu_model_.GetIndexOfCommandId(command_id) != -1;
  }

 protected:
  // RenderViewContextMenu implementation.
  virtual bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* accelerator) OVERRIDE {
    return false;
  }
  virtual void PlatformInit() OVERRIDE {}
  virtual void PlatformCancel() OVERRIDE {}
};

// State holder for the LaunchReply test. This provides an WebIntentsDispatcher
// that will, when used to launch a Web Intent, will return its reply via this
// class. The result may then be waited on via WaitUntilReply().
class LaunchReplyHandler {
 public:
  explicit LaunchReplyHandler(webkit_glue::WebIntentData& data)
      : data_(data),
        replied_(false),
        weak_ptr_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)) {
    intents_dispatcher_ = content::WebIntentsDispatcher::Create(data);
    intents_dispatcher_->RegisterReplyNotification(base::Bind(
        &LaunchReplyHandler::OnReply, weak_ptr_factory_.GetWeakPtr()));
  }

  content::WebIntentsDispatcher* intents_dispatcher() {
    return intents_dispatcher_;
  }

  // Waits until a reply to this Web Intent is provided via the
  // WebIntentsDispatcher.
  bool WaitUntilReply() {
    if (replied_)
      return true;
    waiting_ = true;
    content::RunMessageLoop();
    waiting_ = false;
    return replied_;
  }

 private:
  void OnReply(webkit_glue::WebIntentReplyType reply) {
    // Note that the ReplyNotification registered on WebIntentsDispatcher does
    // not include the result data: this is reserved for the source page (which
    // we don't care about).
    replied_ = true;
    if (waiting_)
      MessageLoopForUI::current()->Quit();
  }

  webkit_glue::WebIntentData data_;
  bool replied_;
  bool waiting_;
  content::WebIntentsDispatcher* intents_dispatcher_;
  base::WeakPtrFactory<LaunchReplyHandler> weak_ptr_factory_;
};

const char kTestFilePath[] = "platform_apps/launch_files/test.txt";

}  // namespace

// Tests that CreateShellWindow doesn't crash if you close it straight away.
// LauncherPlatformAppBrowserTest relies on this behaviour, but is only run for
// ash, so we test that it works here.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, CreateAndCloseShellWindow) {
  const Extension* extension = LoadAndLaunchPlatformApp("minimal");
  ShellWindow* window = CreateShellWindow(extension);
  CloseShellWindow(window);
}

// Tests that platform apps received the "launch" event when launched.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, OnLaunchedEvent) {
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/launch")) << message_;
}

// Tests that platform apps can reply to "launch" events that contain a Web
// Intent. This test does not test the mechanics of invoking a Web Intent
// from a source page, and short-circuits to LaunchPlatformAppWithWebIntent.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, LaunchReply) {
  FilePath path = test_data_dir_.AppendASCII("platform_apps/launch_reply");
  const extensions::Extension* extension = LoadExtension(path);
  ASSERT_TRUE(extension) << "Failed to load extension.";

  webkit_glue::WebIntentData data(
      UTF8ToUTF16("http://webintents.org/view"),
      UTF8ToUTF16("text/plain"),
      UTF8ToUTF16("irrelevant unserialized string data"));
  LaunchReplyHandler handler(data);

  // Navigate to a boring page: we don't care what it is, but we require some
  // source WebContents to launch the Web Intent "from".
  ui_test_utils::NavigateToURL(browser(), GURL("about:blank"));
  WebContents* web_contents = chrome::GetActiveWebContents(browser());
  ASSERT_TRUE(web_contents);

  extensions::LaunchPlatformAppWithWebIntent(
      browser()->profile(),
      extension,
      handler.intents_dispatcher(),
      web_contents);

  ASSERT_TRUE(handler.WaitUntilReply());
}

// Tests that platform apps cannot use certain disabled window properties, but
// can override them and then use them.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, DisabledWindowProperties) {
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/disabled_window_properties"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, EmptyContextMenu) {
  ExtensionTestMessageListener launched_listener("Launched", false);
  LoadAndLaunchPlatformApp("minimal");

  ASSERT_TRUE(launched_listener.WaitUntilSatisfied());

  // The empty app doesn't add any context menu items, so its menu should
  // only include the developer tools.
  WebContents* web_contents = GetFirstShellWindowWebContents();
  ASSERT_TRUE(web_contents);
  WebKit::WebContextMenuData data;
  content::ContextMenuParams params(data);
  scoped_ptr<PlatformAppContextMenu> menu;
  menu.reset(new PlatformAppContextMenu(web_contents, params));
  menu->Init();
  ASSERT_TRUE(menu->HasCommandWithId(IDC_CONTENT_CONTEXT_INSPECTELEMENT));
  ASSERT_TRUE(menu->HasCommandWithId(IDC_RELOAD));
  ASSERT_FALSE(menu->HasCommandWithId(IDC_BACK));
  ASSERT_FALSE(menu->HasCommandWithId(IDC_SAVE_PAGE));
}

IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, AppWithContextMenu) {
  ExtensionTestMessageListener launched_listener("Launched", false);
  LoadAndLaunchPlatformApp("context_menu");

  // Wait for the extension to tell us it's initialized its context menus and
  // launched a window.
  ASSERT_TRUE(launched_listener.WaitUntilSatisfied());

  // The context_menu app has two context menu items. These, along with a
  // separator and the developer tools, is all that should be in the menu.
  WebContents* web_contents = GetFirstShellWindowWebContents();
  ASSERT_TRUE(web_contents);
  WebKit::WebContextMenuData data;
  content::ContextMenuParams params(data);
  scoped_ptr<PlatformAppContextMenu> menu;
  menu.reset(new PlatformAppContextMenu(web_contents, params));
  menu->Init();
  ASSERT_TRUE(menu->HasCommandWithId(IDC_EXTENSIONS_CONTEXT_CUSTOM_FIRST));
  ASSERT_TRUE(menu->HasCommandWithId(IDC_EXTENSIONS_CONTEXT_CUSTOM_FIRST + 1));
  ASSERT_TRUE(menu->HasCommandWithId(IDC_CONTENT_CONTEXT_INSPECTELEMENT));
  ASSERT_TRUE(menu->HasCommandWithId(IDC_RELOAD));
  ASSERT_FALSE(menu->HasCommandWithId(IDC_BACK));
  ASSERT_FALSE(menu->HasCommandWithId(IDC_SAVE_PAGE));
  ASSERT_FALSE(menu->HasCommandWithId(IDC_CONTENT_CONTEXT_UNDO));
}

IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, AppWithContextMenuTextField) {
  ExtensionTestMessageListener launched_listener("Launched", false);
  LoadAndLaunchPlatformApp("context_menu");

  // Wait for the extension to tell us it's initialized its context menus and
  // launched a window.
  ASSERT_TRUE(launched_listener.WaitUntilSatisfied());

  // The context_menu app has one context menu item. This, along with a
  // separator and the developer tools, is all that should be in the menu.
  WebContents* web_contents = GetFirstShellWindowWebContents();
  ASSERT_TRUE(web_contents);
  WebKit::WebContextMenuData data;
  content::ContextMenuParams params(data);
  params.is_editable = true;
  scoped_ptr<PlatformAppContextMenu> menu;
  menu.reset(new PlatformAppContextMenu(web_contents, params));
  menu->Init();
  ASSERT_TRUE(menu->HasCommandWithId(IDC_EXTENSIONS_CONTEXT_CUSTOM_FIRST));
  ASSERT_TRUE(menu->HasCommandWithId(IDC_CONTENT_CONTEXT_INSPECTELEMENT));
  ASSERT_TRUE(menu->HasCommandWithId(IDC_RELOAD));
  ASSERT_TRUE(menu->HasCommandWithId(IDC_CONTENT_CONTEXT_UNDO));
  ASSERT_TRUE(menu->HasCommandWithId(IDC_CONTENT_CONTEXT_COPY));
  ASSERT_FALSE(menu->HasCommandWithId(IDC_BACK));
  ASSERT_FALSE(menu->HasCommandWithId(IDC_SAVE_PAGE));
}

IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, AppWithContextMenuSelection) {
  ExtensionTestMessageListener launched_listener("Launched", false);
  LoadAndLaunchPlatformApp("context_menu");

  // Wait for the extension to tell us it's initialized its context menus and
  // launched a window.
  ASSERT_TRUE(launched_listener.WaitUntilSatisfied());

  // The context_menu app has one context menu item. This, along with a
  // separator and the developer tools, is all that should be in the menu.
  WebContents* web_contents = GetFirstShellWindowWebContents();
  ASSERT_TRUE(web_contents);
  WebKit::WebContextMenuData data;
  content::ContextMenuParams params(data);
  params.selection_text = ASCIIToUTF16("Hello World");
  scoped_ptr<PlatformAppContextMenu> menu;
  menu.reset(new PlatformAppContextMenu(web_contents, params));
  menu->Init();
  ASSERT_TRUE(menu->HasCommandWithId(IDC_EXTENSIONS_CONTEXT_CUSTOM_FIRST));
  ASSERT_TRUE(menu->HasCommandWithId(IDC_CONTENT_CONTEXT_INSPECTELEMENT));
  ASSERT_TRUE(menu->HasCommandWithId(IDC_RELOAD));
  ASSERT_FALSE(menu->HasCommandWithId(IDC_CONTENT_CONTEXT_UNDO));
  ASSERT_TRUE(menu->HasCommandWithId(IDC_CONTENT_CONTEXT_COPY));
  ASSERT_FALSE(menu->HasCommandWithId(IDC_BACK));
  ASSERT_FALSE(menu->HasCommandWithId(IDC_SAVE_PAGE));
}

IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, AppWithContextMenuClicked) {
  ExtensionTestMessageListener launched_listener("Launched", false);
  LoadAndLaunchPlatformApp("context_menu_click");

  // Wait for the extension to tell us it's initialized its context menus and
  // launched a window.
  ASSERT_TRUE(launched_listener.WaitUntilSatisfied());

  // Test that the menu item shows up
  WebContents* web_contents = GetFirstShellWindowWebContents();
  ASSERT_TRUE(web_contents);
  WebKit::WebContextMenuData data;
  content::ContextMenuParams params(data);
  params.page_url = GURL("http://foo.bar");
  scoped_ptr<PlatformAppContextMenu> menu;
  menu.reset(new PlatformAppContextMenu(web_contents, params));
  menu->Init();
  ASSERT_TRUE(menu->HasCommandWithId(IDC_EXTENSIONS_CONTEXT_CUSTOM_FIRST));

  // Execute the menu item
  ExtensionTestMessageListener onclicked_listener("onClicked fired for id1",
                                                  false);
  menu->ExecuteCommand(IDC_EXTENSIONS_CONTEXT_CUSTOM_FIRST);

  ASSERT_TRUE(onclicked_listener.WaitUntilSatisfied());
}

IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, DisallowNavigation) {
  ASSERT_TRUE(StartTestServer());
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/navigation")) << message_;
}

IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, Iframes) {
  ASSERT_TRUE(StartTestServer());
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/iframes")) << message_;
}

// Tests that localStorage and WebSQL are disabled for platform apps.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, DisallowStorage) {
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/storage")) << message_;
}

IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, Restrictions) {
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/restrictions")) << message_;
}

// Tests that platform apps can use the chrome.app.window.* API.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, WindowsApi) {
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/windows_api")) << message_;
}

// Tests that extensions can't use platform-app-only APIs.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, PlatformAppsOnly) {
  ASSERT_TRUE(RunExtensionTestIgnoreManifestWarnings(
      "platform_apps/apps_only")) << message_;
}

// Tests that platform apps have isolated storage by default.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, Isolation) {
  ASSERT_TRUE(StartTestServer());

  // Load a (non-app) page under the "localhost" origin that sets a cookie.
  GURL set_cookie_url = test_server()->GetURL(
      "files/extensions/platform_apps/isolation/set_cookie.html");
  GURL::Replacements replace_host;
  std::string host_str("localhost");  // Must stay in scope with replace_host.
  replace_host.SetHostStr(host_str);
  set_cookie_url = set_cookie_url.ReplaceComponents(replace_host);

  ui_test_utils::NavigateToURLWithDisposition(
      browser(), set_cookie_url,
      CURRENT_TAB, ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Make sure the cookie is set.
  int cookie_size;
  std::string cookie_value;
  automation_util::GetCookies(
      set_cookie_url,
      chrome::GetWebContentsAt(browser(), 0),
      &cookie_size,
      &cookie_value);
  ASSERT_EQ("testCookie=1", cookie_value);

  // Let the platform app request the same URL, and make sure that it doesn't
  // see the cookie.
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/isolation")) << message_;
}

IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, ExtensionWindowingApis) {
  // Initially there should be just the one browser window visible to the
  // extensions API.
  const Extension* extension = LoadExtension(
      test_data_dir_.AppendASCII("common/background_page"));
  ASSERT_EQ(1U, RunGetWindowsFunctionForExtension(extension));

  // And no shell windows.
  ASSERT_EQ(0U, GetShellWindowCount());

  // Launch a platform app that shows a window.
  ExtensionTestMessageListener launched_listener("Launched", false);
  LoadAndLaunchPlatformApp("minimal");
  ASSERT_TRUE(launched_listener.WaitUntilSatisfied());
  ASSERT_EQ(1U, GetShellWindowCount());
  ShellWindowRegistry::ShellWindowSet shell_windows =
      ShellWindowRegistry::Get(browser()->profile())->shell_windows();
  int shell_window_id = (*shell_windows.begin())->session_id().id();

  // But it's not visible to the extensions API, it still thinks there's just
  // one browser window.
  ASSERT_EQ(1U, RunGetWindowsFunctionForExtension(extension));
  // It can't look it up by ID either
  ASSERT_FALSE(RunGetWindowFunctionForExtension(shell_window_id, extension));

  // The app can also only see one window (its own).
  // TODO(jeremya): add an extension function to get a shell window by ID, and
  // to get a list of all the shell windows, so we can test this.

  // Launch another platform app that also shows a window.
  ExtensionTestMessageListener launched_listener2("Launched", false);
  LoadAndLaunchPlatformApp("context_menu");
  ASSERT_TRUE(launched_listener2.WaitUntilSatisfied());

  // There are two total shell windows, but each app can only see its own.
  ASSERT_EQ(2U, GetShellWindowCount());
  // TODO(jeremya): as above, this requires more extension functions.
}

// TODO(benwells): fix these tests for ChromeOS.
#if !defined(OS_CHROMEOS)
// Tests that command line parameters get passed through to platform apps
// via launchData correctly when launching with a file.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, LaunchWithFile) {
  SetCommandLineArg(kTestFilePath);
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/launch_file"))
      << message_;
}

// Tests that relative paths can be passed through to the platform app.
// This test doesn't use the normal test infrastructure as it needs to open
// the application differently to all other platform app tests, by setting
// the application_launch::LaunchParams.current_directory field.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, LaunchWithRelativeFile) {
  // Setup the command line
  ClearCommandLineArgs();
  CommandLine* command_line = CommandLine::ForCurrentProcess();
  FilePath relative_test_doc = FilePath::FromUTF8Unsafe(kTestFilePath);
  relative_test_doc = relative_test_doc.NormalizePathSeparators();
  command_line->AppendArgPath(relative_test_doc);

  // Load the extension
  ResultCatcher catcher;
  const Extension* extension = LoadExtension(
      test_data_dir_.AppendASCII("platform_apps/launch_file"));
  ASSERT_TRUE(extension);

  // Run the test
  application_launch::LaunchParams params(browser()->profile(), extension,
                                          extension_misc::LAUNCH_NONE,
                                          NEW_WINDOW);
  params.command_line = CommandLine::ForCurrentProcess();
  params.current_directory = test_data_dir_;
  application_launch::OpenApplication(params);

  if (!catcher.GetNextResult()) {
    message_ = catcher.message();
    ASSERT_TRUE(0);
  }
}

// Tests that no launch data is sent through if the platform app provides
// an intent with the wrong action.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, LaunchWithWrongIntent) {
  SetCommandLineArg(kTestFilePath);
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/launch_wrong_intent"))
      << message_;
}

// Tests that no launch data is sent through if the file is of the wrong MIME
// type.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, LaunchWithWrongType) {
  SetCommandLineArg(kTestFilePath);
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/launch_wrong_type"))
      << message_;
}

// Tests that no launch data is sent through if the platform app does not
// provide an intent.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, LaunchWithNoIntent) {
  SetCommandLineArg(kTestFilePath);
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/launch_no_intent"))
      << message_;
}

// Tests that no launch data is sent through if the file MIME type cannot
// be read.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, LaunchNoType) {
  SetCommandLineArg("platform_apps/launch_files/test.unknownextension");
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/launch_invalid"))
      << message_;
}

// Tests that no launch data is sent through if the file does not exist.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, LaunchNoFile) {
  SetCommandLineArg("platform_apps/launch_files/doesnotexist.txt");
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/launch_invalid"))
      << message_;
}

// Tests that no launch data is sent through if the argument is a directory.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, LaunchWithDirectory) {
  SetCommandLineArg("platform_apps/launch_files");
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/launch_invalid"))
      << message_;
}

// Tests that no launch data is sent through if there are no arguments passed
// on the command line
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, LaunchWithNothing) {
  ClearCommandLineArgs();
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/launch_nothing"))
      << message_;
}

// Test that platform apps can use the chrome.fileSystem.getDisplayPath
// function to get the native file system path of a file they are launched with.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, GetDisplayPath) {
  SetCommandLineArg(kTestFilePath);
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/get_display_path"))
      << message_;
}

#endif  // defined(OS_CHROMEOS)

IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, OpenLink) {
  ASSERT_TRUE(StartTestServer());
  content::WindowedNotificationObserver observer(
      chrome::NOTIFICATION_TAB_ADDED,
      content::Source<content::WebContentsDelegate>(browser()));
  LoadAndLaunchPlatformApp("open_link");
  observer.Wait();
  ASSERT_EQ(2, browser()->tab_count());
}

IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, MutationEventsDisabled) {
  ASSERT_TRUE(RunPlatformAppTest("platform_apps/mutation_events")) << message_;
}

// Test that windows created with an id will remember and restore their
// geometry when opening new windows.
IN_PROC_BROWSER_TEST_F(PlatformAppBrowserTest, ShellWindowRestorePosition) {
  ExtensionTestMessageListener page2_listener("WaitForPage2", true);
  ExtensionTestMessageListener page3_listener("WaitForPage3", true);
  ExtensionTestMessageListener done_listener("Done1", false);
  ExtensionTestMessageListener done2_listener("Done2", false);
  ExtensionTestMessageListener done3_listener("Done3", false);

  ASSERT_TRUE(LoadAndLaunchPlatformApp("geometry"));

  // Wait for the app to be launched (although this is mostly to have a
  // message to reply to to let the script know it should create its second
  // window.
  ASSERT_TRUE(page2_listener.WaitUntilSatisfied());

  // Wait for the first window to verify its geometry was correctly set
  // from the default* attributes passed to the create function.
  ASSERT_TRUE(done_listener.WaitUntilSatisfied());

  // Programatically move and resize the window.
  ShellWindow* window = GetFirstShellWindow();
  ASSERT_TRUE(window);
  gfx::Rect bounds(137, 143, 203, 187);
  window->GetBaseWindow()->SetBounds(bounds);

#if defined(TOOLKIT_GTK)
  // TODO(mek): On GTK we have to wait for a roundtrip to the X server before
  // a resize actually happens:
  // "if you call gtk_window_resize() then immediately call
  //  gtk_window_get_size(), the size won't have taken effect yet. After the
  //  window manager processes the resize request, GTK+ receives notification
  //  that the size has changed via a configure event, and the size of the
  //  window gets updated."
  // Because of this we have to wait for an unknown time for the resize to
  // actually take effect. So wait some time or until the resize got
  // handled.
  base::TimeTicks end_time = base::TimeTicks::Now() +
                             TestTimeouts::action_timeout();
  while (base::TimeTicks::Now() < end_time &&
         bounds != window->GetBaseWindow()->GetBounds()) {
    content::RunAllPendingInMessageLoop();
  }

  // In the GTK ShellWindow implementation there also is a delay between
  // getting the correct bounds and it calling SaveWindowPosition, so call that
  // method explicitly to make sure the value was stored.
  window->SaveWindowPosition();
#endif  // defined(TOOLKIT_GTK)

  // Make sure the window was properly moved&resized.
  ASSERT_EQ(bounds, window->GetBaseWindow()->GetBounds());

  // Tell javascript to open a second window.
  page2_listener.Reply("continue");

  // Wait for javascript to verify that the second window got the updated
  // coordinates, ignoring the default coordinates passed to the create method.
  ASSERT_TRUE(done2_listener.WaitUntilSatisfied());

  // Tell javascript to open a third window.
  page3_listener.Reply("continue");

  // Wait for javascript to verify that the third window got the restored size
  // and explicitly specified coordinates.
  ASSERT_TRUE(done3_listener.WaitUntilSatisfied());
}

}  // namespace extensions
