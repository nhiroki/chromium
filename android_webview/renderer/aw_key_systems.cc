// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/renderer/aw_key_systems.h"
#include "components/cdm/renderer/android_key_systems.h"

namespace android_webview {

void AwAddKeySystems(
    std::vector<media::KeySystemInfo>* key_systems_info) {
  cdm::AddAndroidWidevine(key_systems_info);
  cdm::AddAndroidPlatformKeySystems(key_systems_info);
}

}  // namespace android_webview
