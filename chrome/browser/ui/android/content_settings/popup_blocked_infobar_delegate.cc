// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/content_settings/popup_blocked_infobar_delegate.h"

#include "base/prefs/pref_service.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/blocked_content/popup_blocker_tab_helper.h"
#include "chrome/grit/generated_resources.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/infobars/core/infobar.h"
#include "grit/theme_resources.h"
#include "ui/base/l10n/l10n_util.h"


// static
void PopupBlockedInfoBarDelegate::Create(content::WebContents* web_contents,
                                         int num_popups) {
  const GURL& url = web_contents->GetURL();
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(web_contents);
  scoped_ptr<infobars::InfoBar> infobar(infobar_service->CreateConfirmInfoBar(
      scoped_ptr<ConfirmInfoBarDelegate>(new PopupBlockedInfoBarDelegate(
          num_popups, url, profile->GetHostContentSettingsMap()))));

  // See if there is an existing popup infobar already.
  // TODO(dfalcantara) When triggering more than one popup the infobar
  // will be shown once, then hide then be shown again.
  // This will be fixed once we have an in place replace infobar mechanism.
  for (size_t i = 0; i < infobar_service->infobar_count(); ++i) {
    infobars::InfoBar* existing_infobar = infobar_service->infobar_at(i);
    if (existing_infobar->delegate()->AsPopupBlockedInfoBarDelegate()) {
      infobar_service->ReplaceInfoBar(existing_infobar, infobar.Pass());
      return;
    }
  }

  infobar_service->AddInfoBar(infobar.Pass());
}

PopupBlockedInfoBarDelegate::~PopupBlockedInfoBarDelegate() {
}

int PopupBlockedInfoBarDelegate::GetIconId() const {
  return IDR_BLOCKED_POPUPS;
}

PopupBlockedInfoBarDelegate*
    PopupBlockedInfoBarDelegate::AsPopupBlockedInfoBarDelegate() {
  return this;
}

PopupBlockedInfoBarDelegate::PopupBlockedInfoBarDelegate(
    int num_popups,
    const GURL& url,
    HostContentSettingsMap* map)
    : ConfirmInfoBarDelegate(), num_popups_(num_popups), url_(url), map_(map) {
  content_settings::SettingInfo setting_info;
  scoped_ptr<base::Value> setting = map->GetWebsiteSetting(
      url, url, CONTENT_SETTINGS_TYPE_POPUPS, std::string(), &setting_info);
  can_show_popups_ =
      setting_info.source != content_settings::SETTING_SOURCE_POLICY;
}

base::string16 PopupBlockedInfoBarDelegate::GetMessageText() const {
  return l10n_util::GetStringFUTF16Int(IDS_POPUPS_BLOCKED_INFOBAR_TEXT,
                                       num_popups_);
}

int PopupBlockedInfoBarDelegate::GetButtons() const {
  if (!can_show_popups_)
    return 0;

  return BUTTON_OK;
}

base::string16 PopupBlockedInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  return l10n_util::GetStringUTF16(IDS_POPUPS_BLOCKED_INFOBAR_BUTTON_SHOW);
}

bool PopupBlockedInfoBarDelegate::Accept() {
  DCHECK(can_show_popups_);

  // Create exceptions.
  map_->AddExceptionForURL(
      url_, url_, CONTENT_SETTINGS_TYPE_POPUPS, CONTENT_SETTING_ALLOW);

  // Launch popups.
  content::WebContents* web_contents =
      InfoBarService::WebContentsFromInfoBar(infobar());
  PopupBlockerTabHelper* popup_blocker_helper =
      PopupBlockerTabHelper::FromWebContents(web_contents);
  DCHECK(popup_blocker_helper);
  PopupBlockerTabHelper::PopupIdMap blocked_popups =
      popup_blocker_helper->GetBlockedPopupRequests();
  for (PopupBlockerTabHelper::PopupIdMap::iterator it = blocked_popups.begin();
      it != blocked_popups.end(); ++it)
    popup_blocker_helper->ShowBlockedPopup(it->first);

  return true;
}
