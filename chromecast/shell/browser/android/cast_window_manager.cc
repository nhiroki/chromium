// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/shell/browser/android/cast_window_manager.h"

#include <jni.h>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/lazy_instance.h"
#include "chromecast/common/chromecast_config.h"
#include "chromecast/common/pref_names.h"
#include "chromecast/shell/browser/android/cast_window_android.h"
#include "chromecast/shell/browser/cast_browser_context.h"
#include "chromecast/shell/browser/cast_browser_main_parts.h"
#include "chromecast/shell/browser/cast_content_browser_client.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "ipc/ipc_channel.h"
#include "jni/CastWindowManager_jni.h"
#include "url/gurl.h"

namespace {

base::LazyInstance<base::android::ScopedJavaGlobalRef<jobject>>
    g_window_manager = LAZY_INSTANCE_INITIALIZER;

content::BrowserContext* g_browser_context = NULL;

}  // namespace

namespace chromecast {
namespace shell {

void SetBrowserContextAndroid(content::BrowserContext* browser_context) {
  g_browser_context = browser_context;
}

base::android::ScopedJavaLocalRef<jobject>
CreateCastWindowView(CastWindowAndroid* shell) {
  JNIEnv* env = base::android::AttachCurrentThread();
  jobject j_window_manager = g_window_manager.Get().obj();
  return Java_CastWindowManager_createCastWindow(env, j_window_manager);
}

void CloseCastWindowView(jobject shell_wrapper) {
  JNIEnv* env = base::android::AttachCurrentThread();
  jobject j_window_manager = g_window_manager.Get().obj();
  Java_CastWindowManager_closeCastWindow(env, j_window_manager, shell_wrapper);
}

// Register native methods
bool RegisterCastWindowManager(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

void Init(JNIEnv* env, jclass clazz, jobject obj) {
  g_window_manager.Get().Reset(
      base::android::ScopedJavaLocalRef<jobject>(env, obj));
}

jlong LaunchCastWindow(JNIEnv* env, jclass clazz, jstring jurl) {
  DCHECK(g_browser_context);
  GURL url(base::android::ConvertJavaStringToUTF8(env, jurl));
  return reinterpret_cast<jlong>(
      CastWindowAndroid::CreateNewWindow(g_browser_context, url));
}

void StopCastWindow(JNIEnv* env, jclass clazz, jlong nativeCastWindow) {
  CastWindowAndroid* window =
      reinterpret_cast<CastWindowAndroid*>(nativeCastWindow);
  DCHECK(window);
  window->Close();
}

void EnableDevTools(JNIEnv* env, jclass clazz, jboolean enable) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // The specific port value doesn't matter since Android uses Unix domain
  // sockets, only whether or not it is zero.
  chromecast::ChromecastConfig::GetInstance()->pref_service()->
      SetInteger(prefs::kRemoteDebuggingPort, enable ? 1 : 0);
}

}  // namespace shell
}  // namespace chromecast
