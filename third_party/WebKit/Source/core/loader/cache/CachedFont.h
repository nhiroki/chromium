/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef CachedFont_h
#define CachedFont_h

#include "core/loader/cache/CachedResource.h"
#include "core/loader/cache/CachedResourceClient.h"
#include "core/platform/graphics/FontOrientation.h"
#include "core/platform/graphics/FontRenderingMode.h"
#include "core/platform/graphics/FontWidthVariant.h"

namespace WebCore {

class CachedResourceLoader;
class FontPlatformData;
class SVGDocument;
class SVGFontElement;
struct FontCustomPlatformData;

class CachedFont : public CachedResource {
public:
    CachedFont(const ResourceRequest&);
    virtual ~CachedFont();
    
    virtual void load(CachedResourceLoader*, const ResourceLoaderOptions&);

    virtual void didAddClient(CachedResourceClient*);

    virtual void allClientsRemoved();
    void beginLoadIfNeeded(CachedResourceLoader* dl);
    bool stillNeedsLoad() const { return !m_loadInitiated; }

    bool ensureCustomFontData();
    FontPlatformData platformDataFromCustomData(float size, bool bold, bool italic, FontOrientation = Horizontal, FontWidthVariant = RegularWidth, FontRenderingMode = NormalRenderingMode);

#if ENABLE(SVG_FONTS)
    bool ensureSVGFontData();
    SVGFontElement* getSVGFontById(const String&) const;
#endif

    virtual void reportMemoryUsage(MemoryObjectInfo*) const OVERRIDE;

private:
    virtual void checkNotify();
    FontCustomPlatformData* m_fontData;
    bool m_loadInitiated;

#if ENABLE(SVG_FONTS)
    RefPtr<SVGDocument> m_externalSVGDocument;
#endif

    friend class MemoryCache;
};

class CachedFontClient : public CachedResourceClient {
public:
    virtual ~CachedFontClient() { }
    static CachedResourceClientType expectedType() { return FontType; }
    virtual CachedResourceClientType resourceClientType() const { return expectedType(); }
    virtual void fontLoaded(CachedFont*) { }
};

}

#endif
