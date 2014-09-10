// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/shell_apps_client.h"

#include "extensions/browser/app_window/app_window.h"
#include "extensions/shell/browser/desktop_controller.h"
#include "extensions/shell/browser/shell_native_app_window.h"

namespace extensions {

ShellAppsClient::ShellAppsClient() {
}

ShellAppsClient::~ShellAppsClient() {
}

std::vector<content::BrowserContext*>
ShellAppsClient::GetLoadedBrowserContexts() {
  NOTIMPLEMENTED();
  return std::vector<content::BrowserContext*>();
}

AppWindow* ShellAppsClient::CreateAppWindow(content::BrowserContext* context,
                                            const Extension* extension) {
  return DesktopController::instance()->CreateAppWindow(context, extension);
}

NativeAppWindow* ShellAppsClient::CreateNativeAppWindow(
      AppWindow* window,
      const AppWindow::CreateParams& params) {
  ShellNativeAppWindow* native_app_window =
      new ShellNativeAppWindow(window, params);
  DesktopController::instance()->AddAppWindow(
      native_app_window->GetNativeWindow());
  return native_app_window;
}

void ShellAppsClient::IncrementKeepAliveCount() {
  NOTIMPLEMENTED();
}

void ShellAppsClient::DecrementKeepAliveCount() {
  NOTIMPLEMENTED();
}

void ShellAppsClient::OpenDevToolsWindow(content::WebContents* web_contents,
                                         const base::Closure& callback) {
  NOTIMPLEMENTED();
}

bool ShellAppsClient::IsCurrentChannelOlderThanDev() {
  return false;
}

}  // namespace extensions
