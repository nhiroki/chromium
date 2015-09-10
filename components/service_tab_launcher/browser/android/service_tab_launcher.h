// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICE_TAB_LAUNCHER_BROWSER_ANDROID_SERVICE_TAB_LAUNCHER_H_
#define COMPONENTS_SERVICE_TAB_LAUNCHER_BROWSER_ANDROID_SERVICE_TAB_LAUNCHER_H_

#include "base/android/jni_android.h"
#include "base/callback_forward.h"
#include "base/id_map.h"
#include "base/memory/singleton.h"

namespace content {
class BrowserContext;
struct OpenURLParams;
class WebContents;
}

namespace service_tab_launcher {

// Launcher for creating new tabs on Android from a background service, where
// there may not necessarily be an Activity or a tab model at all. When the
// tab has been launched, the user of this class will be informed with the
// content::WebContents instance associated with the tab.
class ServiceTabLauncher {
  using TabLaunchedCallback = base::Callback<void(content::WebContents*)>;

 public:
  // Returns the singleton instance of the service tab launcher.
  static ServiceTabLauncher* GetInstance();

  // Launches a new tab when we're in a Service rather than in an Activity.
  // |callback| will be invoked with the resulting content::WebContents* when
  // the tab is avialable. This method must only be called from the UI thread.
  void LaunchTab(content::BrowserContext* browser_context,
                 const content::OpenURLParams& params,
                 const TabLaunchedCallback& callback);

  // To be called when the tab for |request_id| has launched, with the
  // associated |web_contents|. The WebContents must not yet have started
  // the provisional load for the main frame of the navigation.
  void OnTabLaunched(int request_id, content::WebContents* web_contents);

  static bool RegisterServiceTabLauncher(JNIEnv* env);

 private:
  friend struct base::DefaultSingletonTraits<ServiceTabLauncher>;

  ServiceTabLauncher();
  ~ServiceTabLauncher();

  IDMap<TabLaunchedCallback> tab_launched_callbacks_;

  base::android::ScopedJavaGlobalRef<jobject> java_object_;

  DISALLOW_COPY_AND_ASSIGN(ServiceTabLauncher);
};

}  // namespace service_tab_launcher

#endif  // COMPONENTS_SERVICE_TAB_LAUNCHER_BROWSER_ANDROID_SERVICE_TAB_LAUNCHER_H_
