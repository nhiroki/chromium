// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

#include "base/logging.h"
#import "base/test/ios/google_test_runner_delegate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface GoogleTestRunner : XCTestCase
@end

@implementation GoogleTestRunner

- (void)testRunGoogleTests {
  id appDelegate = UIApplication.sharedApplication.delegate;
  DCHECK([appDelegate conformsToProtocol:@protocol(GoogleTestRunnerDelegate)]);

  id<GoogleTestRunnerDelegate> runnerDelegate =
      static_cast<id<GoogleTestRunnerDelegate>>(appDelegate);
  DCHECK(runnerDelegate.supportsRunningGoogleTests);
  XCTAssertTrue([runnerDelegate runGoogleTests] == 0);
}

@end
