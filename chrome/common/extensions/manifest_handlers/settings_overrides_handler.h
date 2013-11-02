// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_EXTENSIONS_MANIFEST_HANDLERS_SETTINGS_OVERRIDES_HANDLER_H_
#define CHROME_COMMON_EXTENSIONS_MANIFEST_HANDLERS_SETTINGS_OVERRIDES_HANDLER_H_

#include "chrome/common/extensions/api/manifest_types.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/manifest_handler.h"

namespace extensions {

// SettingsOverride is associated with "chrome_settings_overrides" manifest key.
// An extension can add a search engine as default or non-default, overwrite the
// homepage and append a startup page to the list.
struct SettingsOverride : public Extension::ManifestData {
  SettingsOverride();
  virtual ~SettingsOverride();

  scoped_ptr<api::manifest_types::ChromeSettingsOverrides::Search_provider>
      search_engine;
  scoped_ptr<GURL> homepage;
  scoped_ptr<GURL> startup_page;
 private:
  DISALLOW_COPY_AND_ASSIGN(SettingsOverride);
};

class SettingsOverrideHandler : public ManifestHandler {
 public:
  SettingsOverrideHandler();
  virtual ~SettingsOverrideHandler();

  virtual bool Parse(Extension* extension, string16* error) OVERRIDE;

 private:
  virtual const std::vector<std::string> Keys() const OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(SettingsOverrideHandler);
};

}  // namespace extensions
#endif  // CHROME_COMMON_EXTENSIONS_MANIFEST_HANDLERS_SETTINGS_OVERRIDES_HANDLER_H_
