// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/ui/ui_test.h"

#include "base/command_line.h"
#include "base/file_util.h"
#include "net/base/net_util.h"

typedef UITest ChromeMainTest;

// Launch the app, then close the app.
TEST_F(ChromeMainTest, AppLaunch) {
  // If we make it here at all, we've succeeded in retrieving the app window
  // in UITest::SetUp()--otherwise we'd fail with an exception in SetUp().

  if (UITest::in_process_renderer()) {
    EXPECT_EQ(1, UITest::GetBrowserProcessCount());
  } else {
#if defined(OS_LINUX)
    // On Linux we'll have four processes: browser, renderer, zygote and
    // sandbox helper.
    EXPECT_EQ(4, UITest::GetBrowserProcessCount());
#else
    // We should have two instances of the browser process alive -
    // one is the Browser and the other is the Renderer.
    EXPECT_EQ(2, UITest::GetBrowserProcessCount());
#endif
  }
}

// Make sure that the testing interface is there and giving reasonable answers.
TEST_F(ChromeMainTest, AppTestingInterface) {
  int window_count = 0;
  EXPECT_TRUE(automation()->GetBrowserWindowCount(&window_count));
  EXPECT_EQ(1, window_count);

  EXPECT_EQ(1, GetTabCount());
}

#if defined(OS_MACOSX)
// Fails an SQL assertion on Mac: http://crbug.com/45108
#define SecondLaunch DISABLED_SecondLaunch
#endif
// Make sure that the second invocation creates a new window.
TEST_F(ChromeMainTest, SecondLaunch) {
  include_testing_id_ = false;

  ASSERT_TRUE(LaunchAnotherBrowserBlockUntilClosed(
                  CommandLine(CommandLine::ARGUMENTS_ONLY)));

  ASSERT_TRUE(automation()->WaitForWindowCountToBecome(2));
}

#if defined(OS_MACOSX)
// Fails an SQL assertion on Mac: http://crbug.com/45108
#define ReuseBrowserInstanceWhenOpeningFile \
    DISABLED_ReuseBrowserInstanceWhenOpeningFile
#endif
TEST_F(ChromeMainTest, ReuseBrowserInstanceWhenOpeningFile) {
  include_testing_id_ = false;

  FilePath test_file = test_data_directory_.AppendASCII("empty.html");

  CommandLine command_line(CommandLine::ARGUMENTS_ONLY);
  command_line.AppendArgPath(test_file);
  ASSERT_TRUE(LaunchAnotherBrowserBlockUntilClosed(command_line));

  ASSERT_TRUE(automation()->IsURLDisplayed(net::FilePathToFileURL(test_file)));
}
