// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/content_setting_combo_model.h"

#include "app/l10n_util.h"
#include "base/command_line.h"
#include "base/string16.h"
#include "chrome/common/chrome_switches.h"
#include "grit/generated_resources.h"

namespace {

// The settings shown in the combobox if show_session_ is false;
const ContentSetting kNoSessionSettings[] = { CONTENT_SETTING_ALLOW,
                                              CONTENT_SETTING_BLOCK };

// The settings shown in the combobox if show_session_ is true;
const ContentSetting kSessionSettings[] = { CONTENT_SETTING_ALLOW,
                                            CONTENT_SETTING_SESSION_ONLY,
                                            CONTENT_SETTING_BLOCK };

// The settings shown in the combobox if show_session_ is true, and we still
// offer the cookie prompt mode.
const ContentSetting kSessionAskSettings[] = { CONTENT_SETTING_ALLOW,
                                               CONTENT_SETTING_ASK,
                                               CONTENT_SETTING_SESSION_ONLY,
                                               CONTENT_SETTING_BLOCK };

}  // namespace

ContentSettingComboModel::ContentSettingComboModel(bool show_session)
    : show_session_(show_session),
      disable_cookie_prompt_(!CommandLine::ForCurrentProcess()->HasSwitch(
                             switches::kEnableCookiePrompt)) {
}

ContentSettingComboModel::~ContentSettingComboModel() {
}

int ContentSettingComboModel::GetItemCount() {
  if (show_session_) {
    return disable_cookie_prompt_ ?
        arraysize(kSessionSettings) : arraysize(kSessionAskSettings);
  } else {
    return arraysize(kNoSessionSettings);
  }
}

string16 ContentSettingComboModel::GetItemAt(int index) {
  switch (SettingForIndex(index)) {
    case CONTENT_SETTING_ALLOW:
      return l10n_util::GetStringUTF16(IDS_EXCEPTIONS_ALLOW_BUTTON);
    case CONTENT_SETTING_BLOCK:
      return l10n_util::GetStringUTF16(IDS_EXCEPTIONS_BLOCK_BUTTON);
    case CONTENT_SETTING_ASK:
      return l10n_util::GetStringUTF16(IDS_EXCEPTIONS_ASK_BUTTON);
    case CONTENT_SETTING_SESSION_ONLY:
      return l10n_util::GetStringUTF16(IDS_EXCEPTIONS_SESSION_ONLY_BUTTON);
    default:
      NOTREACHED();
  }
  return string16();
}

ContentSetting ContentSettingComboModel::SettingForIndex(int index) {
  if (show_session_) {
    return disable_cookie_prompt_ ?
        kSessionSettings[index] : kSessionAskSettings[index];
  } else {
    return kNoSessionSettings[index];
  }
}

int ContentSettingComboModel::IndexForSetting(ContentSetting setting) {
  for (int i = 0; i < GetItemCount(); ++i)
    if (SettingForIndex(i) == setting)
      return i;
  NOTREACHED();
  return 0;
}

