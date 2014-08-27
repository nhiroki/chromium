// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/toolchain.h"

#include <string.h>

#include "base/logging.h"
#include "tools/gn/target.h"
#include "tools/gn/value.h"

const char* Toolchain::kToolCc = "cc";
const char* Toolchain::kToolCxx = "cxx";
const char* Toolchain::kToolObjC = "objc";
const char* Toolchain::kToolObjCxx = "objcxx";
const char* Toolchain::kToolRc = "rc";
const char* Toolchain::kToolAsm = "asm";
const char* Toolchain::kToolAlink = "alink";
const char* Toolchain::kToolSolink = "solink";
const char* Toolchain::kToolLink = "link";
const char* Toolchain::kToolStamp = "stamp";
const char* Toolchain::kToolCopy = "copy";

Toolchain::Toolchain(const Settings* settings, const Label& label)
    : Item(settings, label),
      concurrent_links_(0),
      setup_complete_(false) {
}

Toolchain::~Toolchain() {
}

Toolchain* Toolchain::AsToolchain() {
  return this;
}

const Toolchain* Toolchain::AsToolchain() const {
  return this;
}

// static
Toolchain::ToolType Toolchain::ToolNameToType(const base::StringPiece& str) {
  if (str == kToolCc) return TYPE_CC;
  if (str == kToolCxx) return TYPE_CXX;
  if (str == kToolObjC) return TYPE_OBJC;
  if (str == kToolObjCxx) return TYPE_OBJCXX;
  if (str == kToolRc) return TYPE_RC;
  if (str == kToolAsm) return TYPE_ASM;
  if (str == kToolAlink) return TYPE_ALINK;
  if (str == kToolSolink) return TYPE_SOLINK;
  if (str == kToolLink) return TYPE_LINK;
  if (str == kToolStamp) return TYPE_STAMP;
  if (str == kToolCopy) return TYPE_COPY;
  return TYPE_NONE;
}

// static
std::string Toolchain::ToolTypeToName(ToolType type) {
  switch (type) {
    case TYPE_CC: return kToolCc;
    case TYPE_CXX: return kToolCxx;
    case TYPE_OBJC: return kToolObjC;
    case TYPE_OBJCXX: return kToolObjCxx;
    case TYPE_RC: return kToolRc;
    case TYPE_ASM: return kToolAsm;
    case TYPE_ALINK: return kToolAlink;
    case TYPE_SOLINK: return kToolSolink;
    case TYPE_LINK: return kToolLink;
    case TYPE_STAMP: return kToolStamp;
    case TYPE_COPY: return kToolCopy;
    default:
      NOTREACHED();
      return std::string();
  }
}

const Tool* Toolchain::GetTool(ToolType type) const {
  DCHECK(type != TYPE_NONE);
  return tools_[static_cast<size_t>(type)].get();
}

void Toolchain::SetTool(ToolType type, scoped_ptr<Tool> t) {
  DCHECK(type != TYPE_NONE);
  DCHECK(!tools_[type].get());
  t->SetComplete();
  tools_[type] = t.Pass();
}

void Toolchain::ToolchainSetupComplete() {
  // Collect required bits from all tools.
  for (size_t i = 0; i < TYPE_NUMTYPES; i++) {
    if (tools_[i])
      substitution_bits_.MergeFrom(tools_[i]->substitution_bits());
  }

  setup_complete_ = true;
}

// static
Toolchain::ToolType Toolchain::GetToolTypeForSourceType(SourceFileType type) {
  switch (type) {
    case SOURCE_C:
      return TYPE_CC;
    case SOURCE_CC:
      return TYPE_CXX;
    case SOURCE_M:
      return TYPE_OBJC;
    case SOURCE_MM:
      return TYPE_OBJCXX;
    case SOURCE_ASM:
    case SOURCE_S:
      return TYPE_ASM;
    case SOURCE_RC:
      return TYPE_RC;
    case SOURCE_UNKNOWN:
    case SOURCE_H:
    case SOURCE_O:
      return TYPE_NONE;
    default:
      NOTREACHED();
      return TYPE_NONE;
  }
}

const Tool* Toolchain::GetToolForSourceType(SourceFileType type) {
  return tools_[GetToolTypeForSourceType(type)].get();
}

// static
Toolchain::ToolType Toolchain::GetToolTypeForTargetFinalOutput(
    const Target* target) {
  // The contents of this list might be suprising (i.e. stamp tool for copy
  // rules). See the header for why.
  switch (target->output_type()) {
    case Target::GROUP:
      return TYPE_STAMP;
    case Target::EXECUTABLE:
      return Toolchain::TYPE_LINK;
    case Target::SHARED_LIBRARY:
      return Toolchain::TYPE_SOLINK;
    case Target::STATIC_LIBRARY:
      return Toolchain::TYPE_ALINK;
    case Target::SOURCE_SET:
      return TYPE_STAMP;
    case Target::COPY_FILES:
    case Target::ACTION:
    case Target::ACTION_FOREACH:
      return TYPE_STAMP;
    default:
      NOTREACHED();
      return Toolchain::TYPE_NONE;
  }
}

const Tool* Toolchain::GetToolForTargetFinalOutput(const Target* target) const {
  return tools_[GetToolTypeForTargetFinalOutput(target)].get();
}
