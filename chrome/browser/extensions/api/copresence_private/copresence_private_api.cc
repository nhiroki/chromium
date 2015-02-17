// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/copresence_private/copresence_private_api.h"

#include <vector>

#include "base/lazy_instance.h"
#include "base/stl_util.h"
#include "chrome/browser/copresence/chrome_whispernet_client.h"
#include "chrome/browser/extensions/api/copresence/copresence_api.h"
#include "chrome/common/extensions/api/copresence_private.h"
#include "media/base/audio_bus.h"

namespace extensions {

namespace SendFound = api::copresence_private::SendFound;
namespace SendSamples = api::copresence_private::SendSamples;
namespace SendDetect = api::copresence_private::SendDetect;
namespace SendInitialized = api::copresence_private::SendInitialized;

// Copresence Private functions.

audio_modem::WhispernetClient*
CopresencePrivateFunction::GetWhispernetClient() {
  CopresenceService* service =
      CopresenceService::GetFactoryInstance()->Get(browser_context());
  return service ? service->whispernet_client() : NULL;
}

// CopresenceSendFoundFunction implementation:
ExtensionFunction::ResponseAction CopresencePrivateSendFoundFunction::Run() {
  if (!GetWhispernetClient() ||
      GetWhispernetClient()->GetTokensCallback().is_null()) {
    return RespondNow(NoArguments());
  }

  scoped_ptr<SendFound::Params> params(SendFound::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  std::vector<audio_modem::AudioToken> tokens;
  for (size_t i = 0; i < params->tokens.size(); ++i) {
    tokens.push_back(audio_modem::AudioToken(params->tokens[i]->token,
                                            params->tokens[i]->audible));
  }
  GetWhispernetClient()->GetTokensCallback().Run(tokens);
  return RespondNow(NoArguments());
}

// CopresenceSendEncodedFunction implementation:
ExtensionFunction::ResponseAction CopresencePrivateSendSamplesFunction::Run() {
  if (!GetWhispernetClient() ||
      GetWhispernetClient()->GetSamplesCallback().is_null()) {
    return RespondNow(NoArguments());
  }

  scoped_ptr<SendSamples::Params> params(SendSamples::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  scoped_refptr<media::AudioBusRefCounted> samples =
      media::AudioBusRefCounted::Create(1,  // Mono
                                        params->samples.size() / sizeof(float));

  memcpy(samples->channel(0), vector_as_array(&params->samples),
         params->samples.size());

  GetWhispernetClient()->GetSamplesCallback().Run(
      params->token.audible ? audio_modem::AUDIBLE : audio_modem::INAUDIBLE,
      params->token.token, samples);
  return RespondNow(NoArguments());
}

// CopresenceSendDetectFunction implementation:
ExtensionFunction::ResponseAction CopresencePrivateSendDetectFunction::Run() {
  if (!GetWhispernetClient() ||
      GetWhispernetClient()->GetDetectBroadcastCallback().is_null()) {
    return RespondNow(NoArguments());
  }

  scoped_ptr<SendDetect::Params> params(SendDetect::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  GetWhispernetClient()->GetDetectBroadcastCallback().Run(params->detected);
  return RespondNow(NoArguments());
}

// CopresenceSendInitializedFunction implementation:
ExtensionFunction::ResponseAction
CopresencePrivateSendInitializedFunction::Run() {
  if (!GetWhispernetClient() ||
      GetWhispernetClient()->GetInitializedCallback().is_null()) {
    return RespondNow(NoArguments());
  }

  scoped_ptr<SendInitialized::Params> params(
      SendInitialized::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  GetWhispernetClient()->GetInitializedCallback().Run(params->success);
  return RespondNow(NoArguments());
}

}  // namespace extensions
