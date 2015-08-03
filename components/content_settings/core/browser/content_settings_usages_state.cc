// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/content_settings/core/browser/content_settings_usages_state.h"

#include <string>

#include "base/prefs/pref_service.h"
#include "base/strings/utf_string_conversions.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/url_formatter/url_formatter.h"

ContentSettingsUsagesState::CommittedDetails::CommittedDetails()
    : current_url_valid(false) {
}

ContentSettingsUsagesState::CommittedDetails::~CommittedDetails() {}

ContentSettingsUsagesState::ContentSettingsUsagesState(
    HostContentSettingsMap* host_content_settings_map,
    ContentSettingsType type,
    const std::string& accept_language_pref,
    PrefService* prefs)
    : host_content_settings_map_(host_content_settings_map),
      pref_service_(prefs),
      accept_language_pref_(accept_language_pref),
      type_(type) {
}

ContentSettingsUsagesState::~ContentSettingsUsagesState() {
}

void ContentSettingsUsagesState::OnPermissionSet(
    const GURL& requesting_origin, bool allowed) {
  state_map_[requesting_origin] =
      allowed ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK;
}

void ContentSettingsUsagesState::DidNavigate(const CommittedDetails& details) {
  if (details.current_url_valid)
    embedder_url_ = details.current_url;
  if (state_map_.empty())
    return;
  if (!details.current_url_valid ||
      details.previous_url.GetOrigin() != details.current_url.GetOrigin()) {
    state_map_.clear();
    return;
  }
  // We're in the same origin, check if there's any icon to be displayed.
  unsigned int tab_state_flags = 0;
  GetDetailedInfo(NULL, &tab_state_flags);
  if (!(tab_state_flags & TABSTATE_HAS_ANY_ICON))
    state_map_.clear();
}

void ContentSettingsUsagesState::ClearStateMap() {
  state_map_.clear();
}

void ContentSettingsUsagesState::GetDetailedInfo(
    FormattedHostsPerState* formatted_hosts_per_state,
    unsigned int* tab_state_flags) const {
  DCHECK(tab_state_flags);
  DCHECK(embedder_url_.is_valid());
  ContentSetting default_setting =
      host_content_settings_map_->GetDefaultContentSetting(type_, NULL);
  std::set<std::string> formatted_hosts;
  std::set<std::string> repeated_formatted_hosts;

  // Build a set of repeated formatted hosts
  for (StateMap::const_iterator i(state_map_.begin());
       i != state_map_.end(); ++i) {
    std::string formatted_host = GURLToFormattedHost(i->first);
    if (!formatted_hosts.insert(formatted_host).second) {
      repeated_formatted_hosts.insert(formatted_host);
    }
  }

  for (StateMap::const_iterator i(state_map_.begin());
       i != state_map_.end(); ++i) {
    if (i->second == CONTENT_SETTING_ALLOW)
      *tab_state_flags |= TABSTATE_HAS_ANY_ALLOWED;
    if (formatted_hosts_per_state) {
      std::string formatted_host = GURLToFormattedHost(i->first);
      std::string final_formatted_host =
          repeated_formatted_hosts.find(formatted_host) ==
          repeated_formatted_hosts.end() ?
          formatted_host :
          i->first.spec();
      (*formatted_hosts_per_state)[i->second].insert(final_formatted_host);
    }

    const ContentSetting saved_setting =
        host_content_settings_map_->GetContentSetting(i->first, embedder_url_,
                                                      type_, std::string());
    if (saved_setting != default_setting)
      *tab_state_flags |= TABSTATE_HAS_EXCEPTION;
    if (saved_setting != i->second)
      *tab_state_flags |= TABSTATE_HAS_CHANGED;
    if (saved_setting != CONTENT_SETTING_ASK)
      *tab_state_flags |= TABSTATE_HAS_ANY_ICON;
  }
}

std::string ContentSettingsUsagesState::GURLToFormattedHost(
    const GURL& url) const {
  base::string16 display_host;
  url_formatter::AppendFormattedHost(
      url, pref_service_->GetString(accept_language_pref_), &display_host);
  return base::UTF16ToUTF8(display_host);
}
