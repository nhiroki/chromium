// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/private/pdf.h"

#include "ppapi/c/trusted/ppb_browser_font_trusted.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/var.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_PDF>() {
  return PPB_PDF_INTERFACE;
}

}  // namespace

// static
bool PDF::IsAvailable() {
  return has_interface<PPB_PDF>();
}

// static
Var PDF::GetLocalizedString(const InstanceHandle& instance,
                            PP_ResourceString string_id) {
  if (has_interface<PPB_PDF>()) {
    return Var(PASS_REF,
               get_interface<PPB_PDF>()->GetLocalizedString(
                   instance.pp_instance(), string_id));
  }
  return Var();
}

// static
PP_Resource PDF::GetFontFileWithFallback(
    const InstanceHandle& instance,
    const PP_BrowserFont_Trusted_Description* description,
    PP_PrivateFontCharset charset) {
  if (has_interface<PPB_PDF>()) {
    return get_interface<PPB_PDF>()->GetFontFileWithFallback(
        instance.pp_instance(), description, charset);
  }
  return 0;
}

// static
bool PDF::GetFontTableForPrivateFontFile(PP_Resource font_file,
                                         uint32_t table,
                                         void* output,
                                         uint32_t* output_length) {
  if (has_interface<PPB_PDF>()) {
    return get_interface<PPB_PDF>()->GetFontTableForPrivateFontFile(font_file,
        table, output, output_length);
  }
  return false;
}

// static
void PDF::SearchString(const InstanceHandle& instance,
                       const unsigned short* string,
                       const unsigned short* term,
                       bool case_sensitive,
                       PP_PrivateFindResult** results,
                       int* count) {
  if (has_interface<PPB_PDF>()) {
    get_interface<PPB_PDF>()->SearchString(instance.pp_instance(), string,
        term, case_sensitive, results, count);
  }
}

// static
void PDF::DidStartLoading(const InstanceHandle& instance) {
  if (has_interface<PPB_PDF>())
    get_interface<PPB_PDF>()->DidStartLoading(instance.pp_instance());
}

// static
void PDF::DidStopLoading(const InstanceHandle& instance) {
  if (has_interface<PPB_PDF>())
    get_interface<PPB_PDF>()->DidStopLoading(instance.pp_instance());
}

// static
void PDF::SetContentRestriction(const InstanceHandle& instance,
                                int restrictions) {
  if (has_interface<PPB_PDF>()) {
    get_interface<PPB_PDF>()->SetContentRestriction(instance.pp_instance(),
                                                    restrictions);
  }
}

// static
void PDF::UserMetricsRecordAction(const InstanceHandle& instance,
                                  const Var& action) {
  if (has_interface<PPB_PDF>()) {
    get_interface<PPB_PDF>()->UserMetricsRecordAction(instance.pp_instance(),
                                                      action.pp_var());
  }
}

// static
void PDF::HasUnsupportedFeature(const InstanceHandle& instance) {
  if (has_interface<PPB_PDF>())
    get_interface<PPB_PDF>()->HasUnsupportedFeature(instance.pp_instance());
}

// static
void PDF::SaveAs(const InstanceHandle& instance) {
  if (has_interface<PPB_PDF>())
    get_interface<PPB_PDF>()->SaveAs(instance.pp_instance());
}

// static
void PDF::Print(const InstanceHandle& instance) {
  if (has_interface<PPB_PDF>())
    get_interface<PPB_PDF>()->Print(instance.pp_instance());
}

// static
bool PDF::IsFeatureEnabled(const InstanceHandle& instance,
                           PP_PDFFeature feature) {
  if (has_interface<PPB_PDF>())
    return PP_ToBool(get_interface<PPB_PDF>()->IsFeatureEnabled(
        instance.pp_instance(), feature));
  return false;
}

// static
void PDF::SetSelectedText(const InstanceHandle& instance,
                          const char* selected_text) {
  if (has_interface<PPB_PDF>()) {
    get_interface<PPB_PDF>()->SetSelectedText(instance.pp_instance(),
                                              selected_text);
  }
}

// static
void PDF::SetLinkUnderCursor(const InstanceHandle& instance, const char* url) {
  if (has_interface<PPB_PDF>())
    get_interface<PPB_PDF>()->SetLinkUnderCursor(instance.pp_instance(), url);
}

// static
void PDF::GetV8ExternalSnapshotData(const InstanceHandle& instance,
                                    const char** natives_data_out,
                                    int* natives_size_out,
                                    const char** snapshot_data_out,
                                    int* snapshot_size_out) {
  if (has_interface<PPB_PDF>()) {
    get_interface<PPB_PDF>()->GetV8ExternalSnapshotData(instance.pp_instance(),
        natives_data_out, natives_size_out, snapshot_data_out,
        snapshot_size_out);
    return;
  }
  *natives_data_out = NULL;
  *snapshot_data_out = NULL;
  *natives_size_out = 0;
  *snapshot_size_out = 0;
}

}  // namespace pp
