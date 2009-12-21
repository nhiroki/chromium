// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include <string>

#include "app/l10n_util_mac.h"
#include "app/resource_bundle.h"
#include "base/sys_string_conversions.h"
#include "chrome/browser/extensions/extension_install_ui.h"
#include "chrome/common/extensions/extension.h"
#include "grit/browser_resources.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "skia/ext/skia_utils_mac.h"

class Profile;

void ExtensionInstallUI::ShowExtensionInstallUIPromptImpl(
    Profile* profile, Delegate* delegate, Extension* extension, SkBitmap* icon,
    const string16& warning_text, bool is_uninstall) {
  NSAlert* alert = [[[NSAlert alloc] init] autorelease];
  [alert addButtonWithTitle:l10n_util::GetNSString(
      is_uninstall ? IDS_EXTENSION_PROMPT_UNINSTALL_BUTTON :
                     IDS_EXTENSION_PROMPT_INSTALL_BUTTON)];
  [alert addButtonWithTitle:l10n_util::GetNSString(
      IDS_EXTENSION_PROMPT_CANCEL_BUTTON)];
  [alert setMessageText:l10n_util::GetNSStringF(
       is_uninstall ? IDS_EXTENSION_UNINSTALL_PROMPT_HEADING :
                      IDS_EXTENSION_INSTALL_PROMPT_HEADING,
       UTF8ToUTF16(extension->name()))];
  [alert setInformativeText:base::SysUTF16ToNSString(warning_text)];
  [alert setAlertStyle:NSWarningAlertStyle];
  [alert setIcon:gfx::SkBitmapToNSImage(*icon)];

  if ([alert runModal] == NSAlertFirstButtonReturn) {
    delegate->InstallUIProceed();
  } else {
    delegate->InstallUIAbort();
  }
}

void ExtensionInstallUI::ShowExtensionInstallError(const std::string& error) {
  NSAlert* alert = [[[NSAlert alloc] init] autorelease];
  [alert addButtonWithTitle:l10n_util::GetNSString(IDS_OK)];
  [alert setMessageText:l10n_util::GetNSString(
      IDS_EXTENSION_INSTALL_FAILURE_TITLE)];
  [alert setInformativeText:base::SysUTF8ToNSString(error)];
  [alert setAlertStyle:NSWarningAlertStyle];
  [alert runModal];
}
