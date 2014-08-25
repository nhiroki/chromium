// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_PAGE_ACTION_CONTROLLER_H_
#define CHROME_BROWSER_EXTENSIONS_PAGE_ACTION_CONTROLLER_H_

#include <string>

#include "chrome/browser/extensions/api/extension_action/extension_action_api.h"
#include "chrome/browser/extensions/location_bar_controller.h"

class Profile;

namespace extensions {
class ExtensionRegistry;

// A LocationBarControllerProvider which populates the location bar with icons
// based on the page_action extension API.
// TODO(rdevlin.cronin): This isn't really a controller.
class PageActionController : public LocationBarController::ActionProvider,
                             public ExtensionActionAPI::Observer {
 public:
  explicit PageActionController(content::WebContents* web_contents);
  virtual ~PageActionController();

  // LocationBarController::Provider implementation.
  virtual ExtensionAction* GetActionForExtension(const Extension* extension)
      OVERRIDE;
  virtual void OnNavigated() OVERRIDE;

 private:
  // ExtensionActionAPI::Observer implementation.
  virtual void OnExtensionActionUpdated(
      ExtensionAction* extension_action,
      content::WebContents* web_contents,
      content::BrowserContext* browser_context) OVERRIDE;

  // Returns the associated Profile.
  Profile* GetProfile();

  // The associated WebContents.
  content::WebContents* web_contents_;

  ScopedObserver<ExtensionActionAPI, ExtensionActionAPI::Observer>
      extension_action_observer_;

  DISALLOW_COPY_AND_ASSIGN(PageActionController);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_PAGE_ACTION_CONTROLLER_H_
