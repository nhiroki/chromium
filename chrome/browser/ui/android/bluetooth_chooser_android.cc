// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/bluetooth_chooser_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ssl/security_state_model.h"
#include "chrome/browser/ui/android/window_android_helper.h"
#include "content/public/browser/android/content_view_core.h"
#include "jni/BluetoothChooserDialog_jni.h"
#include "ui/android/window_android.h"

using base::android::AttachCurrentThread;
using base::android::ConvertUTF8ToJavaString;
using base::android::ConvertUTF16ToJavaString;
using base::android::ScopedJavaLocalRef;

BluetoothChooserAndroid::BluetoothChooserAndroid(
    content::WebContents* web_contents,
    const EventHandler& event_handler,
    const GURL& origin)
    : event_handler_(event_handler) {
  base::android::ScopedJavaLocalRef<jobject> window_android =
      content::ContentViewCore::FromWebContents(
          web_contents)->GetWindowAndroid()->GetJavaObject();

  SecurityStateModel* security_model =
      SecurityStateModel::FromWebContents(web_contents);
  DCHECK(security_model);

  // Create (and show) the BluetoothChooser dialog.
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> origin_string =
      ConvertUTF8ToJavaString(env, origin.spec());
  java_dialog_.Reset(Java_BluetoothChooserDialog_create(
      env, window_android.obj(), origin_string.obj(),
      security_model->GetSecurityInfo().security_level,
      reinterpret_cast<intptr_t>(this)));
}

BluetoothChooserAndroid::~BluetoothChooserAndroid() {
  Java_BluetoothChooserDialog_closeDialog(AttachCurrentThread(),
                                          java_dialog_.obj());
}

void BluetoothChooserAndroid::SetAdapterPresence(AdapterPresence presence) {
  if (presence != AdapterPresence::POWERED_ON) {
    Java_BluetoothChooserDialog_notifyAdapterTurnedOff(AttachCurrentThread(),
                                                       java_dialog_.obj());
  }
}

void BluetoothChooserAndroid::ShowDiscoveryState(DiscoveryState state) {
  NOTIMPLEMENTED();  // Currently we don't show a 'still searching' state.
}

void BluetoothChooserAndroid::AddDevice(const std::string& device_id,
                                        const base::string16& device_name) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> java_device_id =
      ConvertUTF8ToJavaString(env, device_id);
  ScopedJavaLocalRef<jstring> java_device_name =
      ConvertUTF16ToJavaString(env, device_name);
  Java_BluetoothChooserDialog_addDevice(
      env, java_dialog_.obj(), java_device_id.obj(), java_device_name.obj());
}

void BluetoothChooserAndroid::RemoveDevice(const std::string& device_id) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> java_device_id =
      ConvertUTF16ToJavaString(env, base::UTF8ToUTF16(device_id));
  Java_BluetoothChooserDialog_removeDevice(env, java_dialog_.obj(),
                                           java_device_id.obj());
}

void BluetoothChooserAndroid::OnDeviceSelected(JNIEnv* env,
                                               jobject obj,
                                               jstring device_id) {
  std::string id = base::android::ConvertJavaStringToUTF8(env, device_id);
  if (id.empty()) {
    event_handler_.Run(Event::CANCELLED, "");
  } else {
    event_handler_.Run(Event::SELECTED, id);
  }
}

void BluetoothChooserAndroid::RestartSearch(JNIEnv* env, jobject obj) {
  event_handler_.Run(Event::RESCAN, "");
}

void BluetoothChooserAndroid::ShowBluetoothOverviewLink(JNIEnv* env,
                                                        jobject obj) {
  event_handler_.Run(Event::SHOW_OVERVIEW_HELP, "");
}

void BluetoothChooserAndroid::ShowBluetoothPairingLink(JNIEnv* env,
                                                       jobject obj) {
  event_handler_.Run(Event::SHOW_PAIRING_HELP, "");
}

void BluetoothChooserAndroid::ShowBluetoothAdapterOffLink(JNIEnv* env,
                                                          jobject obj) {
  event_handler_.Run(Event::SHOW_ADAPTER_OFF_HELP, "");
}

// static
bool BluetoothChooserAndroid::Register(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
