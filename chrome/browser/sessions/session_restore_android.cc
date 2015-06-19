// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sessions/session_restore.h"

#include <vector>

#include "base/memory/scoped_vector.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/sessions/content/content_serialized_navigation_builder.h"
#include "components/sessions/session_types.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"

// The android implementation does not do anything "foreign session" specific.
// We use it to restore tabs from "recently closed" too.
// static
content::WebContents* SessionRestore::RestoreForeignSessionTab(
    content::WebContents* web_contents,
    const sessions::SessionTab& session_tab,
    WindowOpenDisposition disposition) {
  DCHECK(session_tab.navigations.size() > 0);
  content::BrowserContext* context = web_contents->GetBrowserContext();
  Profile* profile = Profile::FromBrowserContext(context);
  TabModel* tab_model = TabModelList::GetTabModelForWebContents(web_contents);
  DCHECK(tab_model);
  ScopedVector<content::NavigationEntry> scoped_entries =
      sessions::ContentSerializedNavigationBuilder::ToNavigationEntries(
          session_tab.navigations, profile);
  // NavigationController::Restore() expects to take ownership of the entries.
  std::vector<content::NavigationEntry*> entries;
  scoped_entries.release(&entries);
  content::WebContents* new_web_contents = content::WebContents::Create(
      content::WebContents::CreateParams(context));
  int selected_index = session_tab.normalized_navigation_index();
  new_web_contents->GetController().Restore(
      selected_index,
      content::NavigationController::RESTORE_LAST_SESSION_EXITED_CLEANLY,
      &entries);

  TabAndroid* current_tab = TabAndroid::FromWebContents(web_contents);
  DCHECK(current_tab);
  if (disposition == CURRENT_TAB) {
    current_tab->SwapTabContents(web_contents, new_web_contents, false, false);
    delete web_contents;
  } else {
    DCHECK(disposition == NEW_FOREGROUND_TAB ||
        disposition == NEW_BACKGROUND_TAB);
    tab_model->CreateTab(new_web_contents, current_tab->GetAndroidId());
  }
  return new_web_contents;
}

// static
std::vector<Browser*> SessionRestore::RestoreForeignSessionWindows(
    Profile* profile,
    chrome::HostDesktopType host_desktop_type,
    std::vector<const sessions::SessionWindow*>::const_iterator begin,
    std::vector<const sessions::SessionWindow*>::const_iterator end) {
  NOTREACHED();
  return std::vector<Browser*>();
}
