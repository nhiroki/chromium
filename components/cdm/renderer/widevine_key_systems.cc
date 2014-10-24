// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cdm/renderer/widevine_key_systems.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "media/base/eme_constants.h"

#include "widevine_cdm_version.h"  // In SHARED_INTERMEDIATE_DIR.

#if defined(WIDEVINE_CDM_AVAILABLE)

using media::KeySystemInfo;
using media::SupportedCodecs;

namespace cdm {

// Return |name|'s parent key system.
static std::string GetDirectParentName(std::string name) {
  size_t last_period = name.find_last_of('.');
  DCHECK_GT(last_period, 0u);
  return name.substr(0u, last_period);
}

void AddWidevineWithCodecs(WidevineCdmType widevine_cdm_type,
                           SupportedCodecs supported_codecs,
                           std::vector<KeySystemInfo>* concrete_key_systems) {
  KeySystemInfo info(kWidevineKeySystem);

  switch (widevine_cdm_type) {
    case WIDEVINE:
      // For standard Widevine, add parent name.
      info.parent_key_system = GetDirectParentName(kWidevineKeySystem);
      break;
#if defined(OS_ANDROID)
    case WIDEVINE_HR_NON_COMPOSITING:
      info.key_system.append(".hrnoncompositing");
      break;
#endif  // defined(OS_ANDROID)
    default:
      NOTREACHED();
  }

  // TODO(xhwang): A container or an initDataType may be supported even though
  // there are no codecs supported in that container. Fix this when we support
  // initDataType.
  info.supported_codecs = supported_codecs;

  // Here we assume that support for a container imples support for the
  // associated initialization data type. KeySystems handles validating
  // |init_data_type| x |container| pairings.
  if (supported_codecs & media::EME_CODEC_WEBM_ALL)
    info.supported_init_data_types |= media::EME_INIT_DATA_TYPE_WEBM;
#if defined(USE_PROPRIETARY_CODECS)
  if (supported_codecs & media::EME_CODEC_MP4_ALL)
    info.supported_init_data_types |= media::EME_INIT_DATA_TYPE_CENC;
#endif  // defined(USE_PROPRIETARY_CODECS)

#if defined(ENABLE_PEPPER_CDMS)
  info.pepper_type = kWidevineCdmPluginMimeType;
#endif  // defined(ENABLE_PEPPER_CDMS)

  concrete_key_systems->push_back(info);
}

}  // namespace cdm

#endif  // defined(WIDEVINE_CDM_AVAILABLE)
