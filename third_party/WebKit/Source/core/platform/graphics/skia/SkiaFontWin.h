/*
 * Copyright (c) 2008, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SkiaFontWin_h
#define SkiaFontWin_h

#include <windows.h>
#include <usp10.h>

struct SkPoint;
struct SkRect;

namespace WebCore {

class FontPlatformData;
class GraphicsContext;

// Note that the offsets parameter is optional. If not null it represents a
// per glyph offset (such as returned by ScriptPlace Windows API function).
void paintSkiaText(GraphicsContext*,
                   const FontPlatformData&,
                   int numGlyphs,
                   const WORD* glyphs,
                   const int* advances,
                   const GOFFSET* offsets,
                   const SkPoint& origin,
                   const SkRect& textRect);

// Note that the offsets parameter is optional. If not null it represents a
// per glyph offset (such as returned by ScriptPlace Windows API function).
// Note: this is less efficient than calling the version with FontPlatformData,
// as that caches the SkTypeface object.
void paintSkiaText(GraphicsContext*,
                   HFONT,
                   int numGlyphs,
                   const WORD* glyphs,
                   const int* advances,
                   const GOFFSET* offsets,
                   const SkPoint& origin,
                   const SkRect& textRect);

}  // namespace WebCore

#endif  // SkiaFontWin_h
