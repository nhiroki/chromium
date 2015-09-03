// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBSITE_SETTINGS_WEBSITE_SETTINGS_INFOBAR_DELEGATE_H_
#define CHROME_BROWSER_UI_WEBSITE_SETTINGS_WEBSITE_SETTINGS_INFOBAR_DELEGATE_H_

#include "components/infobars/core/confirm_infobar_delegate.h"

class InfoBarService;

// This class configures an infobar that is shown when the website settings UI
// is closed and the settings for one or more site permissions have been
// changed. The user is shown a message indicating that a reload of the page is
// required for the changes to take effect, and presented a button to perform
// the reload right from the infobar.
class WebsiteSettingsInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  // Creates a website settings infobar and delegate and adds the infobar to
  // |infobar_service|.
  static void Create(InfoBarService* infobar_service);

 private:
  WebsiteSettingsInfoBarDelegate();
  ~WebsiteSettingsInfoBarDelegate() override;

  // ConfirmInfoBarDelegate:
  Type GetInfoBarType() const override;
  int GetIconId() const override;
  gfx::VectorIconId GetVectorIconId() const override;
  base::string16 GetMessageText() const override;
  int GetButtons() const override;
  base::string16 GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;

  DISALLOW_COPY_AND_ASSIGN(WebsiteSettingsInfoBarDelegate);
};

#endif  // CHROME_BROWSER_UI_WEBSITE_SETTINGS_WEBSITE_SETTINGS_INFOBAR_DELEGATE_H_
