// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/services/view_manager/context_factory_impl.h"

#include "cc/output/output_surface.h"
#include "mojo/cc/context_provider_mojo.h"
#include "ui/compositor/reflector.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_surface.h"
#include "webkit/common/gpu/context_provider_in_process.h"
#include "webkit/common/gpu/grcontext_for_webgraphicscontext3d.h"
#include "webkit/common/gpu/webgraphicscontext3d_in_process_command_buffer_impl.h"

namespace mojo {
namespace service {

ContextFactoryImpl::ContextFactoryImpl(
    ScopedMessagePipeHandle command_buffer_handle)
    : command_buffer_handle_(command_buffer_handle.Pass()),
      did_create_(false) {
  DCHECK(command_buffer_handle_.is_valid());
}

ContextFactoryImpl::~ContextFactoryImpl() {
}

scoped_ptr<cc::OutputSurface> ContextFactoryImpl::CreateOutputSurface(
    ui::Compositor* compositor, bool software_fallback) {
  DCHECK(!did_create_);
  did_create_ = true;
  DCHECK(command_buffer_handle_.is_valid());
  return make_scoped_ptr(
      new cc::OutputSurface(
          new ContextProviderMojo(command_buffer_handle_.Pass())));
}

scoped_refptr<ui::Reflector> ContextFactoryImpl::CreateReflector(
    ui::Compositor* mirroed_compositor,
    ui::Layer* mirroring_layer) {
  return NULL;
}

void ContextFactoryImpl::RemoveReflector(
    scoped_refptr<ui::Reflector> reflector) {
}

scoped_refptr<cc::ContextProvider>
ContextFactoryImpl::SharedMainThreadContextProvider() {
  if (!shared_main_thread_contexts_.get() ||
      shared_main_thread_contexts_->DestroyedOnMainThread()) {
    bool lose_context_when_out_of_memory = false;
    shared_main_thread_contexts_ =
        webkit::gpu::ContextProviderInProcess::CreateOffscreen(
            lose_context_when_out_of_memory);
    if (shared_main_thread_contexts_.get() &&
        !shared_main_thread_contexts_->BindToCurrentThread())
      shared_main_thread_contexts_ = NULL;
  }
  return shared_main_thread_contexts_;
}

void ContextFactoryImpl::RemoveCompositor(ui::Compositor* compositor) {
}

bool ContextFactoryImpl::DoesCreateTestContexts() { return false; }

cc::SharedBitmapManager* ContextFactoryImpl::GetSharedBitmapManager() {
  return NULL;
}

base::MessageLoopProxy* ContextFactoryImpl::GetCompositorMessageLoop() {
  return NULL;
}

}  // namespace service
}  // namespace mojo
