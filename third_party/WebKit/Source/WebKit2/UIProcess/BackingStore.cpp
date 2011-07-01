/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "BackingStore.h"

#include "ShareableBitmap.h"
#include "UpdateInfo.h"

using namespace WebCore;

namespace WebKit {

PassOwnPtr<BackingStore> BackingStore::create(const IntSize& size, float scaleFactor, WebPageProxy* webPageProxy)
{
    return adoptPtr(new BackingStore(size, scaleFactor, webPageProxy));
}

BackingStore::BackingStore(const IntSize& size, float scaleFactor, WebPageProxy* webPageProxy)
    : m_size(size)
    , m_scaleFactor(scaleFactor)
    , m_webPageProxy(webPageProxy)
{
    ASSERT(!m_size.isEmpty());
}

BackingStore::~BackingStore()
{
}

void BackingStore::incorporateUpdate(const UpdateInfo& updateInfo)
{
//    ASSERT(m_size == updateInfo.viewSize);
    
    RefPtr<ShareableBitmap> bitmap = ShareableBitmap::create(updateInfo.bitmapHandle);
    if (!bitmap)
        return;

#if !ASSERT_DISABLED
    IntSize updateSize = updateInfo.updateRectBounds.size();
    updateSize.scale(m_scaleFactor);
    ASSERT(bitmap->size() == updateSize);
#endif
    
    incorporateUpdate(bitmap.get(), updateInfo);
}

} // namespace WebKit
