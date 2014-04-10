// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/startup/session_crashed_infobar_delegate.h"

#include "base/prefs/pref_registry_simple.h"
#include "base/prefs/testing_pref_service.h"
#include "base/run_loop.h"
#include "chrome/browser/infobars/infobar.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"

#if defined(OS_CHROMEOS)
#include "base/command_line.h"
#include "chrome/common/chrome_switches.h"
#endif

#if defined(OS_CHROMEOS)
// TODO(nkostylev): Cleanup this code once multi-profiles are enabled by
// default on CrOS. http://crbug.com/351655
class SessionCrashedInfoBarDelegateUnitTest :
    public BrowserWithTestWindowTest,
    public testing::WithParamInterface<bool> {
#else
class SessionCrashedInfoBarDelegateUnitTest : public BrowserWithTestWindowTest {
#endif
 public:
  virtual void SetUp() OVERRIDE {
    static_cast<TestingBrowserProcess*>(g_browser_process)
        ->SetLocalState(&pref_service);
    chrome::RegisterLocalState(pref_service.registry());

#if defined(OS_CHROMEOS)
    if (GetParam())
      CommandLine::ForCurrentProcess()->AppendSwitch(switches::kMultiProfiles);
#endif

    // This needs to be called after the local state is set, because it will
    // create a browser which will try to read from the local state.
    BrowserWithTestWindowTest::SetUp();
  }

  virtual void TearDown() OVERRIDE {
    static_cast<TestingBrowserProcess*>(g_browser_process)->SetLocalState(NULL);
    BrowserWithTestWindowTest::TearDown();
  }

 private:
  TestingPrefServiceSimple pref_service;
};

#if defined(OS_CHROMEOS)
TEST_P(SessionCrashedInfoBarDelegateUnitTest, DetachingTabWithCrashedInfoBar) {
#else
TEST_F(SessionCrashedInfoBarDelegateUnitTest, DetachingTabWithCrashedInfoBar) {
#endif
  SessionServiceFactory::SetForTestProfile(
      browser()->profile(),
      static_cast<SessionService*>(
          SessionServiceFactory::GetInstance()->BuildServiceInstanceFor(
              browser()->profile())));

  // Create a browser which we can close during the test.
  Browser::CreateParams params(browser()->profile(),
                               browser()->host_desktop_type());
  scoped_ptr<Browser> first_browser(
      chrome::CreateBrowserWithTestWindowForParams(&params));
  AddTab(first_browser.get(), GURL(chrome::kChromeUINewTabURL));

  // Attach the crashed infobar to it.
  SessionCrashedInfoBarDelegate::Create(first_browser.get());

  TabStripModel* tab_strip = first_browser->tab_strip_model();
  ASSERT_EQ(1, tab_strip->count());
  content::WebContents* web_contents = tab_strip->GetWebContentsAt(0);
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(web_contents);
  EXPECT_EQ(1U, infobar_service->infobar_count());
  ConfirmInfoBarDelegate* infobar =
      infobar_service->infobar_at(0)->delegate()->AsConfirmInfoBarDelegate();
  ASSERT_TRUE(infobar);

  // Open another browser.
  scoped_ptr<Browser> opened_browser(
      chrome::CreateBrowserWithTestWindowForParams(&params));

  // Move the tab which is destroying the crash info bar to the new browser.
  tab_strip->DetachWebContentsAt(0);
  tab_strip = opened_browser->tab_strip_model();
  tab_strip->AppendWebContents(web_contents, true);

  // Close the original browser.
  first_browser->window()->Close();
  first_browser.reset();

  ASSERT_EQ(1, tab_strip->count());
  infobar_service =
      InfoBarService::FromWebContents(tab_strip->GetWebContentsAt(0));
  EXPECT_EQ(1U, infobar_service->infobar_count());

  // This used to crash.
  infobar->Accept();

  // Ramp down the test.
  tab_strip->CloseWebContentsAt(0, TabStripModel::CLOSE_NONE);
}

#if defined(OS_CHROMEOS)
INSTANTIATE_TEST_CASE_P(SessionCrashedInfoBarDelegateUnitTestInstantiation,
                        SessionCrashedInfoBarDelegateUnitTest,
                        testing::Bool());
#endif
