// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/services/gles2/command_buffer_impl.h"

#include "base/bind.h"
#include "base/memory/shared_memory.h"

#include "gpu/command_buffer/common/constants.h"
#include "gpu/command_buffer/service/command_buffer_service.h"
#include "gpu/command_buffer/service/context_group.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "gpu/command_buffer/service/gpu_scheduler.h"
#include "gpu/command_buffer/service/image_manager.h"
#include "gpu/command_buffer/service/mailbox_manager.h"
#include "gpu/command_buffer/service/memory_tracking.h"
#include "mojo/services/gles2/command_buffer_type_conversions.h"
#include "mojo/services/gles2/mojo_buffer_backing.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_surface.h"

namespace mojo {

namespace {

class MemoryTrackerStub : public gpu::gles2::MemoryTracker {
 public:
  MemoryTrackerStub() {}

  virtual void TrackMemoryAllocatedChange(size_t old_size,
                                          size_t new_size,
                                          gpu::gles2::MemoryTracker::Pool pool)
      OVERRIDE {}

  virtual bool EnsureGPUMemoryAvailable(size_t size_needed) OVERRIDE {
    return true;
  };

 private:
  virtual ~MemoryTrackerStub() {}

  DISALLOW_COPY_AND_ASSIGN(MemoryTrackerStub);
};

}  // anonymous namespace

CommandBufferImpl::CommandBufferImpl(
    gfx::GLShareGroup* share_group,
    gpu::gles2::MailboxManager* mailbox_manager)
    : widget_(gfx::kNullAcceleratedWidget),
      size_(1, 1),
      share_group_(share_group),
      mailbox_manager_(mailbox_manager) {
}

CommandBufferImpl::CommandBufferImpl(
    gfx::AcceleratedWidget widget,
    const gfx::Size& size,
    gfx::GLShareGroup* share_group,
    gpu::gles2::MailboxManager* mailbox_manager)
    : widget_(widget),
      size_(size),
      share_group_(share_group),
      mailbox_manager_(mailbox_manager) {
}

CommandBufferImpl::~CommandBufferImpl() {
  client()->DidDestroy();
  if (decoder_) {
    bool have_context = decoder_->MakeCurrent();
    decoder_->Destroy(have_context);
  }
}

void CommandBufferImpl::Initialize(
    CommandBufferSyncClientPtr sync_client,
    mojo::ScopedSharedBufferHandle shared_state) {
  sync_client_ = sync_client.Pass();
  sync_client_->DidInitialize(DoInitialize(shared_state.Pass()));
}

bool CommandBufferImpl::DoInitialize(
    mojo::ScopedSharedBufferHandle shared_state) {
  if (widget_ == gfx::kNullAcceleratedWidget)
    surface_ = gfx::GLSurface::CreateOffscreenGLSurface(size_);
  else
    surface_ = gfx::GLSurface::CreateViewGLSurface(widget_);
  if (!surface_.get())
    return false;

  // TODO(piman): virtual contexts, gpu preference.
  context_ = gfx::GLContext::CreateGLContext(
      share_group_.get(), surface_.get(), gfx::PreferIntegratedGpu);
  if (!context_.get())
    return false;

  if (!context_->MakeCurrent(surface_.get()))
    return false;

  // TODO(piman): ShaderTranslatorCache is currently per-ContextGroup but
  // only needs to be per-thread.
  scoped_refptr<gpu::gles2::ContextGroup> context_group =
      new gpu::gles2::ContextGroup(mailbox_manager_.get(),
                                   new MemoryTrackerStub,
                                   new gpu::gles2::ShaderTranslatorCache,
                                   NULL,
                                   true);

  command_buffer_.reset(
      new gpu::CommandBufferService(context_group->transfer_buffer_manager()));
  bool result = command_buffer_->Initialize();
  DCHECK(result);

  decoder_.reset(::gpu::gles2::GLES2Decoder::Create(context_group.get()));
  scheduler_.reset(new gpu::GpuScheduler(
      command_buffer_.get(), decoder_.get(), decoder_.get()));
  decoder_->set_engine(scheduler_.get());
  decoder_->SetResizeCallback(
      base::Bind(&CommandBufferImpl::OnResize, base::Unretained(this)));

  gpu::gles2::DisallowedFeatures disallowed_features;

  // TODO(piman): attributes.
  std::vector<int32> attrib_vector;
  if (!decoder_->Initialize(surface_,
                            context_,
                            false /* offscreen */,
                            size_,
                            disallowed_features,
                            attrib_vector))
    return false;

  command_buffer_->SetPutOffsetChangeCallback(base::Bind(
      &gpu::GpuScheduler::PutChanged, base::Unretained(scheduler_.get())));
  command_buffer_->SetGetBufferChangeCallback(base::Bind(
      &gpu::GpuScheduler::SetGetBuffer, base::Unretained(scheduler_.get())));
  command_buffer_->SetParseErrorCallback(
      base::Bind(&CommandBufferImpl::OnParseError, base::Unretained(this)));

  // TODO(piman): other callbacks

  const size_t kSize = sizeof(gpu::CommandBufferSharedState);
  scoped_ptr<gpu::BufferBacking> backing(
      gles2::MojoBufferBacking::Create(shared_state.Pass(), kSize));
  if (!backing)
    return false;

  command_buffer_->SetSharedStateBuffer(backing.Pass());
  return true;
}

void CommandBufferImpl::SetGetBuffer(int32_t buffer) {
  command_buffer_->SetGetBuffer(buffer);
}

void CommandBufferImpl::Flush(int32_t put_offset) {
  if (!context_->MakeCurrent(surface_.get())) {
    DLOG(WARNING) << "Context lost";
    client()->LostContext(gpu::error::kUnknown);
    return;
  }
  command_buffer_->Flush(put_offset);
}

void CommandBufferImpl::MakeProgress(int32_t last_get_offset) {
  // TODO(piman): handle out-of-order.
  sync_client_->DidMakeProgress(
      CommandBufferState::From(command_buffer_->GetLastState()));
}

void CommandBufferImpl::RegisterTransferBuffer(
    int32_t id,
    mojo::ScopedSharedBufferHandle transfer_buffer,
    uint32_t size) {
  // Take ownership of the memory and map it into this process.
  // This validates the size.
  scoped_ptr<gpu::BufferBacking> backing(
      gles2::MojoBufferBacking::Create(transfer_buffer.Pass(), size));
  if (!backing) {
    DVLOG(0) << "Failed to map shared memory.";
    return;
  }
  command_buffer_->RegisterTransferBuffer(id, backing.Pass());
}

void CommandBufferImpl::DestroyTransferBuffer(int32_t id) {
  command_buffer_->DestroyTransferBuffer(id);
}

void CommandBufferImpl::Echo(const Callback<void()>& callback) {
  callback.Run();
}

void CommandBufferImpl::OnParseError() {
  gpu::CommandBuffer::State state = command_buffer_->GetLastState();
  client()->LostContext(state.context_lost_reason);
}

void CommandBufferImpl::OnResize(gfx::Size size, float scale_factor) {
  surface_->Resize(size);
}

}  // namespace mojo
