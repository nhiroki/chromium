// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PREF_NAMES_H_
#define IOS_CHROME_BROWSER_PREF_NAMES_H_

namespace ios {
namespace prefs {

// Preferences in ios::prefs:: are temporary shared with desktop Chrome.
// Non-shared preferences should be in the prefs:: namespace (no ios::).
extern const char kAcceptLanguages[];

}  // namespace prefs
}  // namespace ios

namespace prefs {

extern const char kContextualSearchEnabled[];
extern const char kIosBookmarkFolderDefault[];
extern const char kIosBookmarkPromoAlreadySeen[];
extern const char kIosHandoffToOtherDevices[];
extern const char kMetricsReportingWifiOnly[];
extern const char kNetworkPredictionWifiOnly[];
extern const char kNtpShownBookmarksFolder[];
extern const char kPaymentsPreferredUserId[];
extern const char kShowMemoryDebuggingTools[];

extern const char kVoiceSearchLocale[];
extern const char kVoiceSearchTTS[];

extern const char kSigninLastAccounts[];
extern const char kSigninSharedAuthenticationUserId[];
extern const char kSigninShouldPromptForSigninAgain[];

extern const char kOmniboxGeolocationAuthorizationState[];
extern const char kOmniboxGeolocationLastAuthorizationAlertVersion[];

extern const char kDoodleAltText[];
extern const char kDoodleExpirationTimeInSeconds[];
extern const char kDoodleHref[];
extern const char kDoodleLastRequestTimeInSeconds[];

}  // namespace prefs

#endif  // IOS_CHROME_BROWSER_PREF_NAMES_H_
