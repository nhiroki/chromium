// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/pepper/pepper_in_process_resource_creation.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop.h"
#include "content/renderer/pepper/pepper_in_process_router.h"
#include "content/renderer/pepper/renderer_ppapi_host_impl.h"
#include "content/renderer/render_view_impl.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_message_macros.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/browser_font_resource_trusted.h"
#include "ppapi/proxy/ext_crx_file_system_private_resource.h"
#include "ppapi/proxy/file_chooser_resource.h"
#include "ppapi/proxy/file_io_resource.h"
#include "ppapi/proxy/file_system_resource.h"
#include "ppapi/proxy/graphics_2d_resource.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/printing_resource.h"
#include "ppapi/proxy/url_request_info_resource.h"
#include "ppapi/proxy/url_response_info_resource.h"
#include "ppapi/proxy/websocket_resource.h"
#include "ppapi/shared_impl/ppapi_globals.h"
#include "ppapi/shared_impl/ppapi_permissions.h"
#include "ppapi/shared_impl/resource_tracker.h"
#include "ppapi/shared_impl/var.h"
#include "webkit/plugins/ppapi/ppapi_plugin_instance.h"

// Note that the code in the creation functions in this file should generally
// be the same as that in ppapi/proxy/resource_creation_proxy.cc. See
// pepper_in_process_resource_creation.h for what this file is for.

namespace content {

// PepperInProcessResourceCreation --------------------------------------------

PepperInProcessResourceCreation::PepperInProcessResourceCreation(
    RendererPpapiHostImpl* host_impl,
    webkit::ppapi::PluginInstance* instance)
    : ResourceCreationImpl(instance),
      host_impl_(host_impl) {
}

PepperInProcessResourceCreation::~PepperInProcessResourceCreation() {
}

PP_Resource PepperInProcessResourceCreation::CreateBrowserFont(
    PP_Instance instance,
    const PP_BrowserFont_Trusted_Description* description) {
  if (!ppapi::proxy::BrowserFontResource_Trusted::IsPPFontDescriptionValid(
      *description))
    return 0;
  ppapi::Preferences prefs(
      host_impl_->GetRenderViewForInstance(instance)->GetWebkitPreferences());
  return (new ppapi::proxy::BrowserFontResource_Trusted(
      host_impl_->in_process_router()->GetPluginConnection(),
      instance,
      *description,
      prefs))->GetReference();
}

PP_Resource PepperInProcessResourceCreation::CreateFileChooser(
    PP_Instance instance,
    PP_FileChooserMode_Dev mode,
    const PP_Var& accept_types) {
  scoped_refptr<ppapi::StringVar> string_var =
      ppapi::StringVar::FromPPVar(accept_types);
  std::string str = string_var.get() ? string_var->value() : std::string();
  return (new ppapi::proxy::FileChooserResource(
      host_impl_->in_process_router()->GetPluginConnection(),
      instance,
      mode,
      str.c_str()))->GetReference();
}

PP_Resource PepperInProcessResourceCreation::CreateFileIO(
    PP_Instance instance) {
  return (new ppapi::proxy::FileIOResource(
      host_impl_->in_process_router()->GetPluginConnection(),
      instance))->GetReference();
}

PP_Resource PepperInProcessResourceCreation::CreateFileSystem(
    PP_Instance instance,
    PP_FileSystemType type) {
  return (new ppapi::proxy::FileSystemResource(
      host_impl_->in_process_router()->GetPluginConnection(),
      instance, type))->GetReference();
}

PP_Resource PepperInProcessResourceCreation::CreateGraphics2D(
    PP_Instance instance,
    const PP_Size* size,
    PP_Bool is_always_opaque) {
  return (new ppapi::proxy::Graphics2DResource(
          host_impl_->in_process_router()->GetPluginConnection(),
          instance, *size, is_always_opaque))->GetReference();
}

PP_Resource PepperInProcessResourceCreation::CreatePrinting(
    PP_Instance instance) {
  return (new ppapi::proxy::PrintingResource(
      host_impl_->in_process_router()->GetPluginConnection(),
      instance))->GetReference();
}

PP_Resource PepperInProcessResourceCreation::CreateTrueTypeFont(
    PP_Instance instance,
    const PP_TrueTypeFontDesc_Dev* desc) {
  NOTIMPLEMENTED();
  return 0;
}

PP_Resource PepperInProcessResourceCreation::CreateURLRequestInfo(
    PP_Instance instance) {
  return (new ppapi::proxy::URLRequestInfoResource(
      host_impl_->in_process_router()->GetPluginConnection(),
      instance, ::ppapi::URLRequestInfoData()))->GetReference();
}

PP_Resource PepperInProcessResourceCreation::CreateURLResponseInfo(
    PP_Instance instance,
    const ::ppapi::URLResponseInfoData& data,
    PP_Resource file_ref_resource) {
  return (new ppapi::proxy::URLResponseInfoResource(
      host_impl_->in_process_router()->GetPluginConnection(),
      instance, data, file_ref_resource))->GetReference();
}

PP_Resource PepperInProcessResourceCreation::CreateWebSocket(
    PP_Instance instance) {
  return (new ppapi::proxy::WebSocketResource(
      host_impl_->in_process_router()->GetPluginConnection(),
      instance))->GetReference();
}

}  // namespace content
