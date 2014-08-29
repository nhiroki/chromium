// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/visibility.h"

#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "tools/gn/err.h"
#include "tools/gn/filesystem_utils.h"
#include "tools/gn/item.h"
#include "tools/gn/label.h"
#include "tools/gn/scope.h"
#include "tools/gn/value.h"
#include "tools/gn/variables.h"

Visibility::Visibility() {
}

Visibility::~Visibility() {
}

bool Visibility::Set(const SourceDir& current_dir,
                     const Value& value,
                     Err* err) {
  patterns_.clear();

  // Allow a single string to be passed in to make the common case (just one
  // pattern) easier to specify.
  if (value.type() == Value::STRING) {
    patterns_.push_back(LabelPattern::GetPattern(current_dir, value, err));
    return !err->has_error();
  }

  // If it's not a string, it should be a list of strings.
  if (!value.VerifyTypeIs(Value::LIST, err))
    return false;

  const std::vector<Value>& list = value.list_value();
  for (size_t i = 0; i < list.size(); i++) {
    patterns_.push_back(LabelPattern::GetPattern(current_dir, list[i], err));
    if (err->has_error())
      return false;
  }
  return true;
}

void Visibility::SetPublic() {
  patterns_.clear();
  patterns_.push_back(
      LabelPattern(LabelPattern::RECURSIVE_DIRECTORY, SourceDir(),
      std::string(), Label()));
}

void Visibility::SetPrivate(const SourceDir& current_dir) {
  patterns_.clear();
  patterns_.push_back(
      LabelPattern(LabelPattern::DIRECTORY, current_dir, std::string(),
      Label()));
}

bool Visibility::CanSeeMe(const Label& label) const {
  for (size_t i = 0; i < patterns_.size(); i++) {
    if (patterns_[i].Matches(label))
      return true;
  }
  return false;
}

std::string Visibility::Describe(int indent, bool include_brackets) const {
  std::string outer_indent_string(indent, ' ');

  if (patterns_.empty())
    return outer_indent_string + "[] (no visibility)\n";

  std::string result;

  std::string inner_indent_string = outer_indent_string;
  if (include_brackets) {
    result += outer_indent_string + "[\n";
    // Indent the insides more if brackets are requested.
    inner_indent_string += "  ";
  }

  for (size_t i = 0; i < patterns_.size(); i++)
    result += inner_indent_string + patterns_[i].Describe() + "\n";

  if (include_brackets)
    result += outer_indent_string + "]\n";
  return result;
}

// static
bool Visibility::CheckItemVisibility(const Item* from,
                                     const Item* to,
                                     Err* err) {
  if (!to->visibility().CanSeeMe(from->label())) {
    std::string to_label = to->label().GetUserVisibleName(false);
    *err = Err(from->defined_from(), "Dependency not allowed.",
        "The item " + from->label().GetUserVisibleName(false) + "\n"
        "can not depend on " + to_label + "\n"
        "because it is not in " + to_label + "'s visibility list: " +
        to->visibility().Describe(0, true));
    return false;
  }
  return true;
}

// static
bool Visibility::FillItemVisibility(Item* item, Scope* scope, Err* err) {
  const Value* vis_value = scope->GetValue(variables::kVisibility, true);
  if (vis_value)
    item->visibility().Set(scope->GetSourceDir(), *vis_value, err);
  else  // Default to public.
    item->visibility().SetPublic();
  return !err->has_error();
}
