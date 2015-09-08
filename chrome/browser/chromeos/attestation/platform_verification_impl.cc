// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/attestation/platform_verification_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"

namespace chromeos {
namespace attestation {

using media::interfaces::PlatformVerification;

// static
void PlatformVerificationImpl::Create(
    content::RenderFrameHost* render_frame_host,
    mojo::InterfaceRequest<PlatformVerification> request) {
  DVLOG(2) << __FUNCTION__;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(render_frame_host);

  // The created object is strongly bound to (and owned by) the pipe.
  new PlatformVerificationImpl(render_frame_host, request.Pass());
}

PlatformVerificationImpl::PlatformVerificationImpl(
    content::RenderFrameHost* render_frame_host,
    mojo::InterfaceRequest<PlatformVerification> request)
    : binding_(this, request.Pass()),
      render_frame_host_(render_frame_host),
      weak_factory_(this) {
  DCHECK(render_frame_host);
}

PlatformVerificationImpl::~PlatformVerificationImpl() {
}

void PlatformVerificationImpl::ChallengePlatform(
    const mojo::String& service_id,
    const mojo::String& challenge,
    const ChallengePlatformCallback& callback) {
  DVLOG(2) << __FUNCTION__;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!platform_verification_flow_.get())
    platform_verification_flow_ = new PlatformVerificationFlow();

  platform_verification_flow_->ChallengePlatformKey(
      content::WebContents::FromRenderFrameHost(render_frame_host_), service_id,
      challenge, base::Bind(&PlatformVerificationImpl::OnPlatformChallenged,
                            weak_factory_.GetWeakPtr(), callback));
}

void PlatformVerificationImpl::OnPlatformChallenged(
    const ChallengePlatformCallback& callback,
    Result result,
    const std::string& signed_data,
    const std::string& signature,
    const std::string& platform_key_certificate) {
  DVLOG(2) << __FUNCTION__ << ": " << result;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (result != PlatformVerificationFlow::SUCCESS) {
    DCHECK(signed_data.empty());
    DCHECK(signature.empty());
    DCHECK(platform_key_certificate.empty());
    LOG(ERROR) << "Platform verification failed.";
    callback.Run(false, "", "", "");
    return;
  }

  DCHECK(!signed_data.empty());
  DCHECK(!signature.empty());
  DCHECK(!platform_key_certificate.empty());
  callback.Run(true, signed_data, signature, platform_key_certificate);
}

}  // namespace attestation
}  // namespace chromeos
