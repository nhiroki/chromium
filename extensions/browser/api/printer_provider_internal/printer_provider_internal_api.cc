// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/printer_provider_internal/printer_provider_internal_api.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/guid.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"
#include "content/public/browser/blob_handle.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/api/printer_provider/printer_provider_api.h"
#include "extensions/browser/api/printer_provider/printer_provider_api_factory.h"
#include "extensions/browser/api/printer_provider/printer_provider_print_job.h"
#include "extensions/browser/blob_holder.h"
#include "extensions/common/api/printer_provider.h"
#include "extensions/common/api/printer_provider_internal.h"

namespace internal_api = extensions::core_api::printer_provider_internal;

namespace extensions {

namespace {

static base::LazyInstance<
    BrowserContextKeyedAPIFactory<PrinterProviderInternalAPI>> g_api_factory =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
BrowserContextKeyedAPIFactory<PrinterProviderInternalAPI>*
PrinterProviderInternalAPI::GetFactoryInstance() {
  return g_api_factory.Pointer();
}

PrinterProviderInternalAPI::PrinterProviderInternalAPI(
    content::BrowserContext* browser_context) {
}

PrinterProviderInternalAPI::~PrinterProviderInternalAPI() {
}

void PrinterProviderInternalAPI::AddObserver(
    PrinterProviderInternalAPIObserver* observer) {
  observers_.AddObserver(observer);
}

void PrinterProviderInternalAPI::RemoveObserver(
    PrinterProviderInternalAPIObserver* observer) {
  observers_.RemoveObserver(observer);
}

void PrinterProviderInternalAPI::NotifyGetPrintersResult(
    const Extension* extension,
    int request_id,
    const PrinterProviderInternalAPIObserver::PrinterInfoVector& printers) {
  FOR_EACH_OBSERVER(PrinterProviderInternalAPIObserver, observers_,
                    OnGetPrintersResult(extension, request_id, printers));
}

void PrinterProviderInternalAPI::NotifyGetCapabilityResult(
    const Extension* extension,
    int request_id,
    const base::DictionaryValue& capability) {
  FOR_EACH_OBSERVER(PrinterProviderInternalAPIObserver, observers_,
                    OnGetCapabilityResult(extension, request_id, capability));
}

void PrinterProviderInternalAPI::NotifyPrintResult(
    const Extension* extension,
    int request_id,
    core_api::printer_provider_internal::PrintError error) {
  FOR_EACH_OBSERVER(PrinterProviderInternalAPIObserver, observers_,
                    OnPrintResult(extension, request_id, error));
}

void PrinterProviderInternalAPI::NotifyGetUsbPrinterInfoResult(
    const Extension* extension,
    int request_id,
    const core_api::printer_provider::PrinterInfo* printer_info) {
  FOR_EACH_OBSERVER(
      PrinterProviderInternalAPIObserver, observers_,
      OnGetUsbPrinterInfoResult(extension, request_id, printer_info));
}

PrinterProviderInternalReportPrintResultFunction::
    PrinterProviderInternalReportPrintResultFunction() {
}

PrinterProviderInternalReportPrintResultFunction::
    ~PrinterProviderInternalReportPrintResultFunction() {
}

ExtensionFunction::ResponseAction
PrinterProviderInternalReportPrintResultFunction::Run() {
  scoped_ptr<internal_api::ReportPrintResult::Params> params(
      internal_api::ReportPrintResult::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  PrinterProviderInternalAPI::GetFactoryInstance()
      ->Get(browser_context())
      ->NotifyPrintResult(extension(), params->request_id, params->error);
  return RespondNow(NoArguments());
}

PrinterProviderInternalReportPrinterCapabilityFunction::
    PrinterProviderInternalReportPrinterCapabilityFunction() {
}

PrinterProviderInternalReportPrinterCapabilityFunction::
    ~PrinterProviderInternalReportPrinterCapabilityFunction() {
}

ExtensionFunction::ResponseAction
PrinterProviderInternalReportPrinterCapabilityFunction::Run() {
  scoped_ptr<internal_api::ReportPrinterCapability::Params> params(
      internal_api::ReportPrinterCapability::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  if (params->capability) {
    PrinterProviderInternalAPI::GetFactoryInstance()
        ->Get(browser_context())
        ->NotifyGetCapabilityResult(extension(), params->request_id,
                                    params->capability->additional_properties);
  } else {
    PrinterProviderInternalAPI::GetFactoryInstance()
        ->Get(browser_context())
        ->NotifyGetCapabilityResult(extension(), params->request_id,
                                    base::DictionaryValue());
  }
  return RespondNow(NoArguments());
}

PrinterProviderInternalReportPrintersFunction::
    PrinterProviderInternalReportPrintersFunction() {
}

PrinterProviderInternalReportPrintersFunction::
    ~PrinterProviderInternalReportPrintersFunction() {
}

ExtensionFunction::ResponseAction
PrinterProviderInternalReportPrintersFunction::Run() {
  scoped_ptr<internal_api::ReportPrinters::Params> params(
      internal_api::ReportPrinters::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  base::ListValue printers;
  if (params->printers) {
    PrinterProviderInternalAPI::GetFactoryInstance()
        ->Get(browser_context())
        ->NotifyGetPrintersResult(extension(), params->request_id,
                                  *params->printers);
  } else {
    PrinterProviderInternalAPI::GetFactoryInstance()
        ->Get(browser_context())
        ->NotifyGetPrintersResult(
            extension(), params->request_id,
            PrinterProviderInternalAPIObserver::PrinterInfoVector());
  }
  return RespondNow(NoArguments());
}

PrinterProviderInternalGetPrintDataFunction::
    PrinterProviderInternalGetPrintDataFunction() {
}

PrinterProviderInternalGetPrintDataFunction::
    ~PrinterProviderInternalGetPrintDataFunction() {
}

ExtensionFunction::ResponseAction
PrinterProviderInternalGetPrintDataFunction::Run() {
  scoped_ptr<internal_api::GetPrintData::Params> params(
      internal_api::GetPrintData::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const PrinterProviderPrintJob* job =
      PrinterProviderAPIFactory::GetInstance()
          ->GetForBrowserContext(browser_context())
          ->GetPrintJob(extension(), params->request_id);
  if (!job)
    return RespondNow(Error("Print request not found."));

  if (job->document_bytes.get()) {
    // |job->document_bytes| are passed to the callback to make sure the ref
    // counted memory does not go away before the memory backed blob is created.
    content::BrowserContext::CreateMemoryBackedBlob(
        browser_context(), job->document_bytes->front_as<char>(),
        job->document_bytes->size(),
        base::Bind(&PrinterProviderInternalGetPrintDataFunction::OnBlob, this,
                   job->content_type, job->document_bytes->size(),
                   job->document_bytes));
  } else if (!job->document_path.empty()) {
    content::BrowserContext::CreateFileBackedBlob(
        browser_context(), job->document_path, 0 /* offset */,
        job->file_info.size, job->file_info.last_modified,
        base::Bind(&PrinterProviderInternalGetPrintDataFunction::OnBlob, this,
                   job->content_type, job->file_info.size,
                   scoped_refptr<base::RefCountedMemory>()));
  } else {
    return RespondNow(Error("Job data not set"));
  }
  return RespondLater();
}

void PrinterProviderInternalGetPrintDataFunction::OnBlob(
    const std::string& type,
    int size,
    const scoped_refptr<base::RefCountedMemory>& data,
    scoped_ptr<content::BlobHandle> blob) {
  if (!blob) {
    SetError("Unable to create the blob.");
    SendResponse(false);
    return;
  }

  internal_api::BlobInfo info;
  info.blob_uuid = blob->GetUUID();
  info.type = type;
  info.size = size;

  std::vector<std::string> uuids;
  uuids.push_back(blob->GetUUID());

  content::WebContents* contents =
      content::WebContents::FromRenderViewHost(render_view_host());
  extensions::BlobHolder* holder =
      extensions::BlobHolder::FromRenderProcessHost(
          contents->GetRenderProcessHost());
  holder->HoldBlobReference(blob.Pass());

  results_ = internal_api::GetPrintData::Results::Create(info);
  SetTransferredBlobUUIDs(uuids);
  SendResponse(true);
}

PrinterProviderInternalReportUsbPrinterInfoFunction::
    PrinterProviderInternalReportUsbPrinterInfoFunction() {
}

PrinterProviderInternalReportUsbPrinterInfoFunction::
    ~PrinterProviderInternalReportUsbPrinterInfoFunction() {
}

ExtensionFunction::ResponseAction
PrinterProviderInternalReportUsbPrinterInfoFunction::Run() {
  scoped_ptr<internal_api::ReportUsbPrinterInfo::Params> params(
      internal_api::ReportUsbPrinterInfo::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  PrinterProviderInternalAPI::GetFactoryInstance()
      ->Get(browser_context())
      ->NotifyGetUsbPrinterInfoResult(extension(), params->request_id,
                                      params->printer_info.get());
  return RespondNow(NoArguments());
}

}  // namespace extensions
