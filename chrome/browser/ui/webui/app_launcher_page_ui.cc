// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/app_launcher_page_ui.h"

#include <string>

#include "base/memory/ref_counted_memory.h"
#include "base/metrics/histogram.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/app_launcher_login_handler.h"
#include "chrome/browser/ui/webui/metrics_handler.h"
#include "chrome/browser/ui/webui/ntp/app_launcher_handler.h"
#include "chrome/browser/ui/webui/ntp/app_resource_cache_factory.h"
#include "chrome/browser/ui/webui/ntp/core_app_launcher_handler.h"
#include "chrome/browser/ui/webui/ntp/favicon_webui_handler.h"
#include "chrome/browser/ui/webui/ntp/ntp_resource_cache.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_ui.h"
#include "extensions/browser/extension_system.h"
#include "grit/theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

#if defined(ENABLE_THEMES)
#include "chrome/browser/ui/webui/theme_handler.h"
#endif

class ExtensionService;

using content::BrowserThread;

///////////////////////////////////////////////////////////////////////////////
// AppLauncherPageUI

AppLauncherPageUI::AppLauncherPageUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  web_ui->OverrideTitle(l10n_util::GetStringUTF16(IDS_APP_LAUNCHER_TAB_TITLE));

  if (!GetProfile()->IsOffTheRecord()) {
    ExtensionService* service =
        extensions::ExtensionSystem::Get(GetProfile())->extension_service();
    // We should not be launched without an ExtensionService.
    DCHECK(service);
    web_ui->AddMessageHandler(new AppLauncherHandler(service));
    web_ui->AddMessageHandler(new CoreAppLauncherHandler());
    web_ui->AddMessageHandler(new FaviconWebUIHandler());
    web_ui->AddMessageHandler(new MetricsHandler());
  }

#if defined(ENABLE_THEMES)
  // The theme handler can require some CPU, so do it after hooking up the most
  // visited handler. This allows the DB query for the new tab thumbs to happen
  // earlier.
  web_ui->AddMessageHandler(new ThemeHandler());
#endif

  scoped_ptr<HTMLSource> html_source(
      new HTMLSource(GetProfile()->GetOriginalProfile()));
  content::URLDataSource::Add(GetProfile(), html_source.release());
}

AppLauncherPageUI::~AppLauncherPageUI() {
}

// static
base::RefCountedMemory* AppLauncherPageUI::GetFaviconResourceBytes(
    ui::ScaleFactor scale_factor) {
  return ui::ResourceBundle::GetSharedInstance().
      LoadDataResourceBytesForScale(IDR_BOOKMARK_BAR_APPS_SHORTCUT,
                                    scale_factor);
}

bool AppLauncherPageUI::OverrideHandleWebUIMessage(
    const GURL& source_url,
    const std::string& message,
    const base::ListValue& args) {
  if (message == "getApps" &&
      AppLauncherLoginHandler::ShouldShow(GetProfile())) {
    web_ui()->AddMessageHandler(new AppLauncherLoginHandler());
  }
  return false;
}


Profile* AppLauncherPageUI::GetProfile() const {
  return Profile::FromWebUI(web_ui());
}

///////////////////////////////////////////////////////////////////////////////
// HTMLSource

AppLauncherPageUI::HTMLSource::HTMLSource(Profile* profile)
    : profile_(profile) {
}

std::string AppLauncherPageUI::HTMLSource::GetSource() const {
  return chrome::kChromeUIAppLauncherPageHost;
}

void AppLauncherPageUI::HTMLSource::StartDataRequest(
    const std::string& path,
    int render_process_id,
    int render_frame_id,
    const content::URLDataSource::GotDataCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  NTPResourceCache* resource = AppResourceCacheFactory::GetForProfile(profile_);
  resource->set_should_show_other_devices_menu(false);

  content::RenderProcessHost* render_host =
      content::RenderProcessHost::FromID(render_process_id);
  NTPResourceCache::WindowType win_type = NTPResourceCache::GetWindowType(
      profile_, render_host);
  scoped_refptr<base::RefCountedMemory> html_bytes(
      resource->GetNewTabHTML(win_type));

  callback.Run(html_bytes.get());
}

std::string AppLauncherPageUI::HTMLSource::GetMimeType(
    const std::string& resource) const {
  return "text/html";
}

bool AppLauncherPageUI::HTMLSource::ShouldReplaceExistingSource() const {
  return false;
}

bool AppLauncherPageUI::HTMLSource::ShouldAddContentSecurityPolicy() const {
  return false;
}

AppLauncherPageUI::HTMLSource::~HTMLSource() {}
