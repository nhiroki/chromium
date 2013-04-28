/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Google Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ScrollbarThemeChromiumMac_h
#define ScrollbarThemeChromiumMac_h

#include "core/platform/mac/ScrollbarThemeMac.h"

namespace WebCore {

class ScrollbarThemeChromiumMac : public ScrollbarThemeMac {
public:
    ScrollbarThemeChromiumMac();
    virtual ~ScrollbarThemeChromiumMac();

    virtual bool paint(ScrollbarThemeClient*, GraphicsContext*, const IntRect& damageRect);

    virtual void paintOverhangAreas(ScrollView*, GraphicsContext*, const IntRect& horizontalOverhangArea, const IntRect& verticalOverhangArea, const IntRect& dirtyRect);
    virtual void paintTickmarks(GraphicsContext*, ScrollbarThemeClient*, const IntRect&) OVERRIDE;
    
private:
    void paintGivenTickmarks(GraphicsContext*, ScrollbarThemeClient*, const IntRect&, const Vector<IntRect>&);

    RefPtr<Pattern> m_overhangPattern;
};

}

#endif // ScrollbarThemeChromiumMac_h
