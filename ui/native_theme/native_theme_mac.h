// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_NATIVE_THEME_NATIVE_THEME_MAC_H_
#define UI_NATIVE_THEME_NATIVE_THEME_MAC_H_

#include "ui/native_theme/fallback_theme.h"
#include "ui/native_theme/native_theme_export.h"

namespace ui {

// Mac implementation of native theme support.
// TODO(tapted): This should not use FallbackTheme. http://crbug.com/379086.
class NativeThemeMac : public FallbackTheme {
 public:
  static NativeThemeMac* instance();

  // Overridden from NativeTheme:
  virtual SkColor GetSystemColor(ColorId color_id) const OVERRIDE;

  // Overridden from NativeThemeBase:
  virtual void PaintScrollbarTrack(
      SkCanvas* canvas,
      Part part,
      State state,
      const ScrollbarTrackExtraParams& extra_params,
      const gfx::Rect& rect) const OVERRIDE;
  virtual void PaintScrollbarThumb(SkCanvas* sk_canvas,
                                   Part part,
                                   State state,
                                   const gfx::Rect& rect) const OVERRIDE;
  virtual void PaintScrollbarCorner(SkCanvas* canvas,
                                    State state,
                                    const gfx::Rect& rect) const OVERRIDE;
  virtual void PaintMenuPopupBackground(
      SkCanvas* canvas,
      const gfx::Size& size,
      const MenuBackgroundExtraParams& menu_background) const OVERRIDE;
  virtual void PaintMenuItemBackground(
      SkCanvas* canvas,
      State state,
      const gfx::Rect& rect,
      const MenuListExtraParams& menu_list) const OVERRIDE;

 private:
  NativeThemeMac();
  virtual ~NativeThemeMac();

  DISALLOW_COPY_AND_ASSIGN(NativeThemeMac);
};

}  // namespace ui

#endif  // UI_NATIVE_THEME_NATIVE_THEME_MAC_H_
