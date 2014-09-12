// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_UI_ACCESSIBILITY_FOCUS_RING_H_
#define CHROME_BROWSER_CHROMEOS_UI_ACCESSIBILITY_FOCUS_RING_H_

#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"

namespace chromeos {

// An AccessibilityFocusRing is a special type of shape designed to
// outline the focused object on the screen for users with visual
// impairments. It's specifically designed to outline text ranges that
// span multiple lines (we'll call this a "paragraph" shape from here on,
// but it works for any text range), so it can outline a shape defined by a
// few words from the first line, the complete contents of more lines,
// followed by a few words from the last line. See the figure below.
// When highlighting any other object, it outlines a rectangular shape.
//
// The outline is outset from the object it's highlighting by a few pixels;
// this margin distance also determines its border radius for rounded
// corners.
//
// An AccessibilityFocusRing can be initialized with either a rectangle
// defining the bounds of an object, or a paragraph-shape with three
// rectangles defining a top line, a body, and a bottom line, which are
// assumed to be adjacent to one another.
//
// Initializing an AccessibilityFocusRing computes the following 36 points
// that completely define the shape's outline. This shape can be traced
// using Skia or any other drawing utility just by drawing alternating
// straight lines and quadratic curves (e.g. a line from 0 to 1, a curve
// from 1 to 3 with 2 as a control point, then a line from 3 to 4, and so on.
//
// The same path should be used even if the focus ring was initialized with
// a rectangle and not a paragraph shape - this makes it possible to
// smoothly animate between one object and the next simply by interpolating
// points.
//
// Noncontiguous shapes should be handled by drawing multiple focus rings.
//
// The 36 points are defined as follows:
//
//          2 3------------------------------4 5
//           /                                \
//          1                                  6
//          |      First line of paragraph     |
//          0                                  7
//         /                                    \
// 32 33-34 35                                 8 9---------------10 11
//   /                                                             \
// 31      Middle line of paragraph..........................       12
// |                                                                |
// |                                                                |
// |       Middle line of paragraph..........................       |
// |                                                                |
// |                                                                |
// 30      Middle line of paragraph..........................       13
//   \                                                             /
// 29 28---------27 26                             17 16---------15 14
//                 \                                 /
//                  25                             18
//                  |    Last line of paragraph    |
//                  24                             19
//                    \                           /
//                  23 22-----------------------21 20

struct AccessibilityFocusRing {
  // Construct an AccessibilityFocusRing that outlines a rectangular object.
  static AccessibilityFocusRing CreateWithRect(
      const gfx::Rect& bounds, int margin);

  // Construct an AccessibilityFocusRing that outlines a paragraph-shaped
  // object.
  static AccessibilityFocusRing CreateWithParagraphShape(
      const gfx::Rect& top_line,
      const gfx::Rect& body,
      const gfx::Rect& bottom_line,
      int margin);

  gfx::Point points[36];
  gfx::Rect GetBounds() const;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_UI_ACCESSIBILITY_FOCUS_RING_H_
