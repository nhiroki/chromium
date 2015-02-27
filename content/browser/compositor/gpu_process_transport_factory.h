// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_GPU_PROCESS_TRANSPORT_FACTORY_H_
#define CONTENT_BROWSER_COMPOSITOR_GPU_PROCESS_TRANSPORT_FACTORY_H_

#include <map>

#include "base/id_map.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "content/browser/compositor/image_transport_factory.h"
#include "content/common/gpu/client/gpu_channel_host.h"
#include "ui/compositor/compositor.h"

namespace base {
class Thread;
}

namespace cc {
class SurfaceManager;
}

namespace content {
class BrowserCompositorOutputSurface;
class CompositorSwapClient;
class ContextProviderCommandBuffer;
class ReflectorImpl;
class WebGraphicsContext3DCommandBufferImpl;

class GpuProcessTransportFactory
    : public ui::ContextFactory,
      public ImageTransportFactory {
 public:
  GpuProcessTransportFactory();

  ~GpuProcessTransportFactory() override;

  scoped_ptr<WebGraphicsContext3DCommandBufferImpl>
  CreateOffscreenCommandBufferContext();

  // ui::ContextFactory implementation.
  void CreateOutputSurface(base::WeakPtr<ui::Compositor> compositor) override;
  scoped_ptr<ui::Reflector> CreateReflector(ui::Compositor* source,
                                            ui::Layer* target) override;
  void RemoveReflector(ui::Reflector* reflector) override;
  void RemoveCompositor(ui::Compositor* compositor) override;
  scoped_refptr<cc::ContextProvider> SharedMainThreadContextProvider() override;
  bool DoesCreateTestContexts() override;
  uint32 GetImageTextureTarget() override;
  cc::SharedBitmapManager* GetSharedBitmapManager() override;
  gpu::GpuMemoryBufferManager* GetGpuMemoryBufferManager() override;
  scoped_ptr<cc::SurfaceIdAllocator> CreateSurfaceIdAllocator() override;
  void ResizeDisplay(ui::Compositor* compositor,
                     const gfx::Size& size) override;

  // ImageTransportFactory implementation.
  ui::ContextFactory* GetContextFactory() override;
  gfx::GLSurfaceHandle GetSharedSurfaceHandle() override;
  cc::SurfaceManager* GetSurfaceManager() override;
  GLHelper* GetGLHelper() override;
  void AddObserver(ImageTransportFactoryObserver* observer) override;
  void RemoveObserver(ImageTransportFactoryObserver* observer) override;
#if defined(OS_MACOSX)
  void OnSurfaceDisplayed(int surface_id) override;
  void OnCompositorRecycled(ui::Compositor* compositor) override;
  bool SurfaceShouldNotShowFramesAfterRecycle(int surface_id) const override;
#endif

 private:
  struct PerCompositorData;

  PerCompositorData* CreatePerCompositorData(ui::Compositor* compositor);
  void EstablishedGpuChannel(base::WeakPtr<ui::Compositor> compositor,
                             bool create_gpu_output_surface,
                             int num_attempts);
  scoped_ptr<WebGraphicsContext3DCommandBufferImpl> CreateContextCommon(
      scoped_refptr<GpuChannelHost> gpu_channel_host,
      int surface_id);

  void OnLostMainThreadSharedContextInsideCallback();
  void OnLostMainThreadSharedContext();

  typedef std::map<ui::Compositor*, PerCompositorData*> PerCompositorDataMap;
  PerCompositorDataMap per_compositor_data_;
  scoped_refptr<ContextProviderCommandBuffer> shared_main_thread_contexts_;
  scoped_ptr<GLHelper> gl_helper_;
  ObserverList<ImageTransportFactoryObserver> observer_list_;
  scoped_ptr<cc::SurfaceManager> surface_manager_;
  uint32_t next_surface_id_namespace_;

  // The contents of this map and its methods may only be used on the compositor
  // thread.
  IDMap<BrowserCompositorOutputSurface> output_surface_map_;

  base::WeakPtrFactory<GpuProcessTransportFactory> callback_factory_;

  DISALLOW_COPY_AND_ASSIGN(GpuProcessTransportFactory);
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_GPU_PROCESS_TRANSPORT_FACTORY_H_
