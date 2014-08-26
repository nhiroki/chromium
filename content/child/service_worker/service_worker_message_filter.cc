// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/service_worker/service_worker_message_filter.h"

#include "base/message_loop/message_loop_proxy.h"
#include "content/child/service_worker/service_worker_dispatcher.h"
#include "content/child/thread_safe_sender.h"
#include "content/child/worker_thread_task_runner.h"
#include "content/common/service_worker/service_worker_messages.h"
#include "content/common/service_worker/service_worker_types.h"
#include "ipc/ipc_message_macros.h"

namespace content {

namespace {

// Sends a ServiceWorkerObjectDestroyed message to the browser so it can delete
// the ServiceWorker handle.
void SendServiceWorkerObjectDestroyed(
    ThreadSafeSender* sender,
    int handle_id) {
  if (handle_id == kInvalidServiceWorkerHandleId)
    return;
  sender->Send(
      new ServiceWorkerHostMsg_DecrementServiceWorkerRefCount(handle_id));
}

void SendRegistrationObjectDestroyed(
    ThreadSafeSender* sender,
    int handle_id) {
  if (handle_id == kInvalidServiceWorkerRegistrationHandleId)
    return;
  sender->Send(
      new ServiceWorkerHostMsg_DecrementRegistrationRefCount(handle_id));
}

}  // namespace

ServiceWorkerMessageFilter::ServiceWorkerMessageFilter(ThreadSafeSender* sender)
    : main_thread_loop_proxy_(base::MessageLoopProxy::current()),
      thread_safe_sender_(sender) {}

ServiceWorkerMessageFilter::~ServiceWorkerMessageFilter() {}

base::TaskRunner* ServiceWorkerMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& msg) {
  if (IPC_MESSAGE_CLASS(msg) != ServiceWorkerMsgStart)
    return NULL;
  int ipc_thread_id = 0;
  const bool success = PickleIterator(msg).ReadInt(&ipc_thread_id);
  DCHECK(success);
  if (!ipc_thread_id)
    return main_thread_loop_proxy_.get();
  return new WorkerThreadTaskRunner(ipc_thread_id);
}

bool ServiceWorkerMessageFilter::OnMessageReceived(const IPC::Message& msg) {
  if (IPC_MESSAGE_CLASS(msg) != ServiceWorkerMsgStart)
    return false;
  ServiceWorkerDispatcher::GetOrCreateThreadSpecificInstance(
      thread_safe_sender_.get())->OnMessageReceived(msg);
  return true;
}

void ServiceWorkerMessageFilter::OnStaleMessageReceived(
    const IPC::Message& msg) {
  // Specifically handle some messages in case we failed to post task
  // to the thread (meaning that the context on the thread is now gone).
  IPC_BEGIN_MESSAGE_MAP(ServiceWorkerMessageFilter, msg)
    IPC_MESSAGE_HANDLER(ServiceWorkerMsg_ServiceWorkerRegistered,
                        OnStaleRegistered)
    IPC_MESSAGE_HANDLER(ServiceWorkerMsg_SetVersionAttributes,
                        OnStaleSetVersionAttributes)
    IPC_MESSAGE_HANDLER(ServiceWorkerMsg_SetControllerServiceWorker,
                        OnStaleSetControllerServiceWorker)
  IPC_END_MESSAGE_MAP()
}

void ServiceWorkerMessageFilter::OnStaleRegistered(
    int thread_id,
    int request_id,
    const ServiceWorkerRegistrationObjectInfo& info) {
  SendRegistrationObjectDestroyed(thread_safe_sender_.get(), info.handle_id);
}

void ServiceWorkerMessageFilter::OnStaleSetVersionAttributes(
    int thread_id,
    int provider_id,
    int registration_handle_id,
    int changed_mask,
    const ServiceWorkerVersionAttributes& attributes) {
  SendServiceWorkerObjectDestroyed(thread_safe_sender_.get(),
                                   attributes.installing.handle_id);
  SendServiceWorkerObjectDestroyed(thread_safe_sender_.get(),
                                   attributes.waiting.handle_id);
  SendServiceWorkerObjectDestroyed(thread_safe_sender_.get(),
                                   attributes.active.handle_id);
  SendRegistrationObjectDestroyed(thread_safe_sender_.get(),
                                  registration_handle_id);
}

void ServiceWorkerMessageFilter::OnStaleSetControllerServiceWorker(
    int thread_id,
    int provider_id,
    const ServiceWorkerObjectInfo& info) {
  SendServiceWorkerObjectDestroyed(thread_safe_sender_.get(), info.handle_id);
}

}  // namespace content
