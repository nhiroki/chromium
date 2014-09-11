// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_BROWSER_DEFAULT_SHELL_BROWSER_MAIN_DELEGATE_H_
#define EXTENSIONS_SHELL_BROWSER_DEFAULT_SHELL_BROWSER_MAIN_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "extensions/shell/browser/shell_browser_main_delegate.h"

namespace extensions {

// A ShellBrowserMainDelegate that starts an application specified
// by the "--app" command line. This is used only in the browser process.
class DefaultShellBrowserMainDelegate : public ShellBrowserMainDelegate {
 public:
  DefaultShellBrowserMainDelegate();
  virtual ~DefaultShellBrowserMainDelegate();

  // ShellBrowserMainDelegate:
  virtual void Start(content::BrowserContext* context) OVERRIDE;
  virtual void Shutdown() OVERRIDE;
  virtual DesktopController* CreateDesktopController() OVERRIDE;
  virtual AppsClient* CreateAppsClient() OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(DefaultShellBrowserMainDelegate);
};

}  // namespace extensions

#endif  // DEFAULT_SHELL_BROWSER_MAIN_DELEGATE_H_
