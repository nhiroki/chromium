// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/task_management/providers/child_process_task.h"

#include "base/i18n/rtl.h"
#include "base/numerics/safe_conversions.h"
#include "chrome/browser/process_resource_usage.h"
#include "chrome/browser/task_management/task_manager_observer.h"
#include "chrome/grit/generated_resources.h"
#include "components/nacl/common/nacl_process_type.h"
#include "content/public/browser/browser_child_process_host.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/common/process_type.h"
#include "content/public/common/service_registry.h"
#include "grit/theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace task_management {

namespace {

gfx::ImageSkia* g_default_icon = nullptr;

gfx::ImageSkia* GetDefaultIcon() {
  if (!g_default_icon && ResourceBundle::HasSharedInstance()) {
    g_default_icon = ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
        IDR_PLUGINS_FAVICON);
  }

  return g_default_icon;
}

base::string16 GetLocalizedTitle(const base::string16& title,
                                 int process_type) {
  base::string16 result_title = title;
  if (result_title.empty()) {
    switch (process_type) {
      case content::PROCESS_TYPE_PLUGIN:
      case content::PROCESS_TYPE_PPAPI_PLUGIN:
      case content::PROCESS_TYPE_PPAPI_BROKER:
        result_title = l10n_util::GetStringUTF16(
            IDS_TASK_MANAGER_UNKNOWN_PLUGIN_NAME);
        break;
      default:
        // Nothing to do for non-plugin processes.
        break;
    }
  }

  // Explicitly mark name as LTR if there is no strong RTL character,
  // to avoid the wrong concatenation result similar to "!Yahoo Mail: the
  // best web-based Email: NIGULP", in which "NIGULP" stands for the Hebrew
  // or Arabic word for "plugin".
  base::i18n::AdjustStringForLocaleDirection(&result_title);

  switch (process_type) {
    case content::PROCESS_TYPE_UTILITY:
      return l10n_util::GetStringFUTF16(IDS_TASK_MANAGER_UTILITY_PREFIX,
                                        result_title);
    case content::PROCESS_TYPE_GPU:
      return l10n_util::GetStringUTF16(IDS_TASK_MANAGER_GPU_PREFIX);
    case content::PROCESS_TYPE_PLUGIN:
    case content::PROCESS_TYPE_PPAPI_PLUGIN:
      return l10n_util::GetStringFUTF16(IDS_TASK_MANAGER_PLUGIN_PREFIX,
                                        result_title);
    case content::PROCESS_TYPE_PPAPI_BROKER:
      return l10n_util::GetStringFUTF16(IDS_TASK_MANAGER_PLUGIN_BROKER_PREFIX,
                                        result_title);
    case PROCESS_TYPE_NACL_BROKER:
      return l10n_util::GetStringUTF16(IDS_TASK_MANAGER_NACL_BROKER_PREFIX);
    case PROCESS_TYPE_NACL_LOADER:
      return l10n_util::GetStringFUTF16(IDS_TASK_MANAGER_NACL_PREFIX,
                                        result_title);
    // These types don't need display names or get them from elsewhere.
    case content::PROCESS_TYPE_BROWSER:
    case content::PROCESS_TYPE_RENDERER:
    case content::PROCESS_TYPE_ZYGOTE:
    case content::PROCESS_TYPE_SANDBOX_HELPER:
    case content::PROCESS_TYPE_MAX:
      break;
    case content::PROCESS_TYPE_UNKNOWN:
      NOTREACHED() << "Need localized name for child process type.";
  }

  return result_title;
}

// Connects the |resource_reporter| to the ServiceRegistry of the
// BrowserChildProcessHost whose unique ID is |unique_child_process_id|.
void ConnectResourceReporterOnIOThread(
    int unique_child_process_id,
    mojo::InterfaceRequest<ResourceUsageReporter> resource_reporter) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  content::BrowserChildProcessHost* host =
      content::BrowserChildProcessHost::FromID(unique_child_process_id);
  if (!host)
    return;

  content::ServiceRegistry* registry = host->GetServiceRegistry();
  if (!registry)
    return;

  registry->ConnectToRemoteService(resource_reporter.Pass());
}

// Creates the Mojo service wrapper that will be used to sample the V8 memory
// usage of the browser child process whose unique ID is
// |unique_child_process_id|.
ProcessResourceUsage* CreateProcessResourcesSampler(
    int unique_child_process_id) {
  ResourceUsageReporterPtr service;
  mojo::InterfaceRequest<ResourceUsageReporter> usage_reporter =
      mojo::GetProxy(&service);

  content::BrowserThread::PostTask(
      content::BrowserThread::IO,
      FROM_HERE,
      base::Bind(&ConnectResourceReporterOnIOThread,
                 unique_child_process_id,
                 base::Passed(&usage_reporter)));

  return new ProcessResourceUsage(service.Pass());
}

}  // namespace

ChildProcessTask::ChildProcessTask(const content::ChildProcessData& data)
    : Task(GetLocalizedTitle(data.name, data.process_type),
           GetDefaultIcon(),
           data.handle),
      process_resources_sampler_(CreateProcessResourcesSampler(data.id)),
      v8_memory_allocated_(-1),
      v8_memory_used_(-1),
      unique_child_process_id_(data.id),
      process_type_(data.process_type) {
}

ChildProcessTask::~ChildProcessTask() {
}

void ChildProcessTask::Refresh(const base::TimeDelta& update_interval,
                               int64 refresh_flags) {
  Task::Refresh(update_interval, refresh_flags);

  if ((refresh_flags & REFRESH_TYPE_V8_MEMORY) == 0)
    return;

  // The child process resources refresh is performed asynchronously, we will
  // invoke it and record the current values (which might be invalid at the
  // moment. We can safely ignore that and count on future refresh cycles
  // potentially having valid values).
  process_resources_sampler_->Refresh(base::Closure());

  v8_memory_allocated_ = base::saturated_cast<int64>(
      process_resources_sampler_->GetV8MemoryAllocated());
  v8_memory_used_ = base::saturated_cast<int64>(
      process_resources_sampler_->GetV8MemoryUsed());
}

Task::Type ChildProcessTask::GetType() const {
  // Convert |content::ProcessType| to |task_management::Task::Type|.
  switch (process_type_) {
    case content::PROCESS_TYPE_PLUGIN:
    case content::PROCESS_TYPE_PPAPI_PLUGIN:
    case content::PROCESS_TYPE_PPAPI_BROKER:
      return Task::PLUGIN;
    case content::PROCESS_TYPE_UTILITY:
      return Task::UTILITY;
    case content::PROCESS_TYPE_ZYGOTE:
      return Task::ZYGOTE;
    case content::PROCESS_TYPE_SANDBOX_HELPER:
      return Task::SANDBOX_HELPER;
    case content::PROCESS_TYPE_GPU:
      return Task::GPU;
    case PROCESS_TYPE_NACL_LOADER:
    case PROCESS_TYPE_NACL_BROKER:
      return Task::NACL;
    default:
      return Task::UNKNOWN;
  }
}

int ChildProcessTask::GetChildProcessUniqueID() const {
  return unique_child_process_id_;
}

bool ChildProcessTask::ReportsV8Memory() const {
  return process_resources_sampler_->ReportsV8MemoryStats();
}

int64 ChildProcessTask::GetV8MemoryAllocated() const {
  return v8_memory_allocated_;
}

int64 ChildProcessTask::GetV8MemoryUsed() const {
  return v8_memory_used_;
}

}  // namespace task_management
