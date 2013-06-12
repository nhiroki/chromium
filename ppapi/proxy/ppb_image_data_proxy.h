// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PPB_IMAGE_DATA_PROXY_H_
#define PPAPI_PPB_IMAGE_DATA_PROXY_H_

#include "base/memory/scoped_ptr.h"
#include "base/shared_memory.h"
#include "build/build_config.h"
#include "ipc/ipc_platform_file.h"
#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/ppb_image_data.h"
#include "ppapi/proxy/interface_proxy.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/proxy/serialized_structs.h"
#include "ppapi/shared_impl/ppb_image_data_shared.h"
#include "ppapi/shared_impl/resource.h"
#include "ppapi/thunk/ppb_image_data_api.h"

class TransportDIB;

namespace ppapi {
namespace proxy {

class SerializedHandle;

// ImageData is an abstract base class for image data resources. Unlike most
// resources, ImageData must be public in the header since a number of other
// resources need to access it.
class PPAPI_PROXY_EXPORT ImageData
    : public ppapi::Resource,
      public NON_EXPORTED_BASE(ppapi::thunk::PPB_ImageData_API),
      public ppapi::PPB_ImageData_Shared {
 public:
  virtual ~ImageData();

  // Resource overrides.
  virtual ppapi::thunk::PPB_ImageData_API* AsPPB_ImageData_API() OVERRIDE;
  virtual void LastPluginRefWasDeleted() OVERRIDE;
  virtual void InstanceWasDeleted() OVERRIDE;

  // PPB_ImageData API.
  virtual PP_Bool Describe(PP_ImageDataDesc* desc) OVERRIDE;
  virtual int32_t GetSharedMemory(int* handle, uint32_t* byte_count) OVERRIDE;
  virtual void SetIsCandidateForReuse() OVERRIDE;

  PPB_ImageData_Shared::ImageDataType type() const { return type_; }
  const PP_ImageDataDesc& desc() const { return desc_; }

  // Prepares this image data to be recycled to the plugin. Clears the contents
  // if zero_contents is true.
  void RecycleToPlugin(bool zero_contents);

 protected:
  ImageData(const ppapi::HostResource& resource,
            PPB_ImageData_Shared::ImageDataType type,
            const PP_ImageDataDesc& desc);

  PPB_ImageData_Shared::ImageDataType type_;
  PP_ImageDataDesc desc_;

  // Set to true when this ImageData is a good candidate for reuse.
  bool is_candidate_for_reuse_;

  DISALLOW_COPY_AND_ASSIGN(ImageData);
};

// PlatformImageData is a full featured image data resource which can access
// the underlying platform-specific canvas and ImageHandle. This can't be used
// by NaCl apps.
#if !defined(OS_NACL)
class PPAPI_PROXY_EXPORT PlatformImageData : public ImageData {
 public:
  PlatformImageData(const ppapi::HostResource& resource,
                    const PP_ImageDataDesc& desc,
                    ImageHandle handle);
  virtual ~PlatformImageData();

  // PPB_ImageData API.
  virtual void* Map() OVERRIDE;
  virtual void Unmap() OVERRIDE;
  virtual SkCanvas* GetPlatformCanvas() OVERRIDE;
  virtual SkCanvas* GetCanvas() OVERRIDE;

  static ImageHandle NullHandle();
  static ImageHandle HandleFromInt(int32_t i);

 private:
  scoped_ptr<TransportDIB> transport_dib_;

  // Null when the image isn't mapped.
  scoped_ptr<SkCanvas> mapped_canvas_;

  DISALLOW_COPY_AND_ASSIGN(PlatformImageData);
};
#endif  // !defined(OS_NACL)

// SimpleImageData is a simple, platform-independent image data resource which
// can be used by NaCl. It can also be used by trusted apps when access to the
// platform canvas isn't needed.
class PPAPI_PROXY_EXPORT SimpleImageData : public ImageData {
 public:
  SimpleImageData(const ppapi::HostResource& resource,
                  const PP_ImageDataDesc& desc,
                  const base::SharedMemoryHandle& handle);
  virtual ~SimpleImageData();

  // PPB_ImageData API.
  virtual void* Map() OVERRIDE;
  virtual void Unmap() OVERRIDE;
  virtual SkCanvas* GetPlatformCanvas() OVERRIDE;
  virtual SkCanvas* GetCanvas() OVERRIDE;

 private:
  base::SharedMemory shm_;
  uint32 size_;
  int map_count_;

  DISALLOW_COPY_AND_ASSIGN(SimpleImageData);
};

class PPB_ImageData_Proxy : public InterfaceProxy {
 public:
  PPB_ImageData_Proxy(Dispatcher* dispatcher);
  virtual ~PPB_ImageData_Proxy();

  static PP_Resource CreateProxyResource(
      PP_Instance instance,
      PPB_ImageData_Shared::ImageDataType type,
      PP_ImageDataFormat format,
      const PP_Size& size,
      PP_Bool init_to_zero);

  // InterfaceProxy implementation.
  virtual bool OnMessageReceived(const IPC::Message& msg);

  // Utility for creating ImageData resources.
  // This can only be called on the host side of the proxy.
  // On failure, will return invalid resource (0). On success it will return a
  // valid resource and the out params will be written.
  // |desc| contains the result of Describe.
  // |image_handle| and |byte_count| contain the result of GetSharedMemory.
  // NOTE: if |init_to_zero| is false, you should write over the entire image
  // to avoid leaking sensitive data to a less privileged process.
  PPAPI_PROXY_EXPORT static PP_Resource CreateImageData(
      PP_Instance instance,
      PPB_ImageData_Shared::ImageDataType type,
      PP_ImageDataFormat format,
      const PP_Size& size,
      bool init_to_zero,
      PP_ImageDataDesc* desc,
      IPC::PlatformFileForTransit* image_handle,
      uint32_t* byte_count);

  static const ApiID kApiID = API_ID_PPB_IMAGE_DATA;

 private:
  // Plugin->Host message handlers.
  void OnHostMsgCreatePlatform(
      PP_Instance instance,
      int32_t format,
      const PP_Size& size,
      PP_Bool init_to_zero,
      HostResource* result,
      PP_ImageDataDesc* desc,
      ImageHandle* result_image_handle);
  void OnHostMsgCreateSimple(
      PP_Instance instance,
      int32_t format,
      const PP_Size& size,
      PP_Bool init_to_zero,
      HostResource* result,
      PP_ImageDataDesc* desc,
      ppapi::proxy::SerializedHandle* result_image_handle);

  // Host->Plugin message handlers.
  void OnPluginMsgNotifyUnusedImageData(const HostResource& old_image_data);

  DISALLOW_COPY_AND_ASSIGN(PPB_ImageData_Proxy);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PPB_IMAGE_DATA_PROXY_H_
