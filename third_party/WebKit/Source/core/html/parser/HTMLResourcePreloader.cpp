/*
 * Copyright (C) 2013 Google Inc. All Rights Reserved.
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

#include "config.h"
#include "core/html/parser/HTMLResourcePreloader.h"

#include "core/dom/Document.h"
#include "core/loader/cache/CachedResourceInitiatorInfo.h"
#include "core/loader/cache/ResourceFetcher.h"

#include "core/css/MediaList.h"
#include "core/css/MediaQueryEvaluator.h"
#include "core/rendering/RenderObject.h"

namespace WebCore {

bool PreloadRequest::isSafeToSendToAnotherThread() const
{
    return m_initiatorName.isSafeToSendToAnotherThread()
        && m_charset.isSafeToSendToAnotherThread()
        && m_resourceURL.isSafeToSendToAnotherThread()
        && m_mediaAttribute.isSafeToSendToAnotherThread()
        && m_baseURL.isSafeToSendToAnotherThread();
}

KURL PreloadRequest::completeURL(Document* document)
{
    return document->completeURL(m_resourceURL, m_baseURL.isEmpty() ? document->url() : m_baseURL);
}

FetchRequest PreloadRequest::resourceRequest(Document* document)
{
    ASSERT(isMainThread());
    CachedResourceInitiatorInfo initiatorInfo;
    initiatorInfo.name = m_initiatorName;
    initiatorInfo.position = m_initiatorPosition;
    FetchRequest request(ResourceRequest(completeURL(document)), initiatorInfo);

    // FIXME: It's possible CORS should work for other request types?
    if (m_resourceType == CachedResource::Script)
        request.mutableResourceRequest().setAllowCookies(m_crossOriginModeAllowsCookies);
    return request;
}

void HTMLResourcePreloader::takeAndPreload(PreloadRequestStream& r)
{
    PreloadRequestStream requests;
    requests.swap(r);

    for (PreloadRequestStream::iterator it = requests.begin(); it != requests.end(); ++it)
        preload(it->release());
}

static bool mediaAttributeMatches(Frame* frame, RenderStyle* renderStyle, const String& attributeValue)
{
    RefPtr<MediaQuerySet> mediaQueries = MediaQuerySet::create(attributeValue);
    MediaQueryEvaluator mediaQueryEvaluator("screen", frame, renderStyle);
    return mediaQueryEvaluator.eval(mediaQueries.get());
}

void HTMLResourcePreloader::preload(PassOwnPtr<PreloadRequest> preload)
{
    ASSERT(m_document->frame());
    ASSERT(m_document->renderer());
    ASSERT(m_document->renderer()->style());
    if (!preload->media().isEmpty() && !mediaAttributeMatches(m_document->frame(), m_document->renderer()->style(), preload->media()))
        return;

    FetchRequest request = preload->resourceRequest(m_document);
    m_document->fetcher()->preload(preload->resourceType(), request, preload->charset());
}


}
