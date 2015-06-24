// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/button/blue_button.h"

#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/sys_color_change_listener.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/resources/grit/views_resources.h"

namespace views {

// static
const char BlueButton::kViewClassName[] = "views/BlueButton";

BlueButton::BlueButton(ButtonListener* listener, const base::string16& text)
    : LabelButton(listener, text) {
  // Inherit STYLE_BUTTON insets, minimum size, alignment, etc.
  SetStyle(STYLE_BUTTON);
  UpdateThemedBorder();
}

BlueButton::~BlueButton() {}

void BlueButton::ResetColorsFromNativeTheme() {
  LabelButton::ResetColorsFromNativeTheme();
  if (!gfx::IsInvertedColorScheme()) {
    SetTextColor(STATE_NORMAL, GetNativeTheme()->
        GetSystemColor(ui::NativeTheme::kColorId_BlueButtonEnabledColor));
    SetTextColor(STATE_HOVERED, GetNativeTheme()->
        GetSystemColor(ui::NativeTheme::kColorId_BlueButtonHoverColor));
    SetTextColor(STATE_PRESSED, GetNativeTheme()->
        GetSystemColor(ui::NativeTheme::kColorId_BlueButtonPressedColor));
    SetTextColor(STATE_DISABLED, GetNativeTheme()->
        GetSystemColor(ui::NativeTheme::kColorId_BlueButtonDisabledColor));

    label()->SetShadows(gfx::ShadowValues(
        1, gfx::ShadowValue(
               gfx::Vector2d(0, 1), 0,
               GetNativeTheme()->GetSystemColor(
                   ui::NativeTheme::kColorId_BlueButtonShadowColor))));
  }
}

const char* BlueButton::GetClassName() const {
  return BlueButton::kViewClassName;
}

scoped_ptr<LabelButtonBorder> BlueButton::CreateDefaultBorder() const {
  // Insets for splitting the images.
  const gfx::Insets insets(5, 5, 5, 5);
  scoped_ptr<LabelButtonBorder> button_border(new LabelButtonBorder(style()));
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  button_border->SetPainter(false, STATE_NORMAL, Painter::CreateImagePainter(
      *rb.GetImageSkiaNamed(IDR_BLUE_BUTTON_NORMAL), insets));
  button_border->SetPainter(false, STATE_HOVERED, Painter::CreateImagePainter(
      *rb.GetImageSkiaNamed(IDR_BLUE_BUTTON_HOVER), insets));
  button_border->SetPainter(false, STATE_PRESSED, Painter::CreateImagePainter(
      *rb.GetImageSkiaNamed(IDR_BLUE_BUTTON_PRESSED), insets));
  button_border->SetPainter(false, STATE_DISABLED, Painter::CreateImagePainter(
      *rb.GetImageSkiaNamed(IDR_BLUE_BUTTON_DISABLED), insets));
  button_border->SetPainter(true, STATE_NORMAL, Painter::CreateImagePainter(
      *rb.GetImageSkiaNamed(IDR_BLUE_BUTTON_FOCUSED_NORMAL), insets));
  button_border->SetPainter(true, STATE_HOVERED, Painter::CreateImagePainter(
      *rb.GetImageSkiaNamed(IDR_BLUE_BUTTON_FOCUSED_HOVER), insets));
  button_border->SetPainter(true, STATE_PRESSED, Painter::CreateImagePainter(
      *rb.GetImageSkiaNamed(IDR_BLUE_BUTTON_FOCUSED_PRESSED), insets));
  button_border->SetPainter(true, STATE_DISABLED, Painter::CreateImagePainter(
      *rb.GetImageSkiaNamed(IDR_BLUE_BUTTON_DISABLED), insets));
  return button_border.Pass();
}

}  // namespace views
