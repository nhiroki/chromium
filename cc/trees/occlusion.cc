// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/occlusion.h"

#include "cc/base/math_util.h"
#include "ui/gfx/rect.h"

namespace cc {

Occlusion::Occlusion() {
}

Occlusion::Occlusion(const gfx::Transform& draw_transform,
                     const SimpleEnclosedRegion& occlusion_from_outside_target,
                     const SimpleEnclosedRegion& occlusion_from_inside_target)
    : draw_transform_(draw_transform),
      occlusion_from_outside_target_(occlusion_from_outside_target),
      occlusion_from_inside_target_(occlusion_from_inside_target) {
}

bool Occlusion::IsOccluded(const gfx::Rect& content_rect) const {
  if (content_rect.IsEmpty())
    return true;

  if (occlusion_from_inside_target_.IsEmpty() &&
      occlusion_from_outside_target_.IsEmpty()) {
    return false;
  }

  // Take the ToEnclosingRect at each step, as we want to contain any unoccluded
  // partial pixels in the resulting Rect.
  gfx::Rect unoccluded_rect_in_target_surface =
      MathUtil::MapEnclosingClippedRect(draw_transform_, content_rect);
  DCHECK_LE(occlusion_from_inside_target_.GetRegionComplexity(), 1u);
  DCHECK_LE(occlusion_from_outside_target_.GetRegionComplexity(), 1u);
  // These subtract operations are more lossy than if we did both operations at
  // once.
  unoccluded_rect_in_target_surface.Subtract(
      occlusion_from_inside_target_.bounds());
  unoccluded_rect_in_target_surface.Subtract(
      occlusion_from_outside_target_.bounds());

  return unoccluded_rect_in_target_surface.IsEmpty();
}

}  // namespace cc
