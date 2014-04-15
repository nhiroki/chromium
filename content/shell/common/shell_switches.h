// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines all the "content_shell" command-line switches.

#ifndef CONTENT_SHELL_COMMON_SHELL_SWITCHES_H_
#define CONTENT_SHELL_COMMON_SHELL_SWITCHES_H_

namespace switches {

extern const char kAllowExternalPages[];
extern const char kCheckLayoutTestSysDeps[];
extern const char kContentBrowserTest[];
extern const char kContentShellDataPath[];
extern const char kCrashDumpsDir[];
extern const char kDumpRenderTree[];
extern const char kEnableAccelerated2DCanvas[];
extern const char kEnableFontSmoothing[];
extern const char kEnableLeakDetection[];
extern const char kEncodeBinary[];
extern const char kExposeInternalsForTesting[];
extern const char kRegisterFontFiles[];
extern const char kStableReleaseMode[];

}  // namespace switches

#endif  // CONTENT_SHELL_COMMON_SHELL_SWITCHES_H_
