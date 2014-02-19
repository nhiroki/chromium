/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "public/platform/WebIDBKeyRange.h"

#include "public/platform/WebIDBKey.h"
#include "modules/indexeddb/IDBKey.h"
#include "modules/indexeddb/IDBKeyRange.h"

using namespace WebCore;

namespace blink {

void WebIDBKeyRange::assign(const WebIDBKeyRange& other)
{
    m_private = other.m_private;
}

void WebIDBKeyRange::assign(const WebIDBKey& lower, const WebIDBKey& upper, bool lowerOpen, bool upperOpen)
{
    // FIXME: replace reset() with nullptr assignment (cf. https://codereview.chromium.org/170603003/ )
    if (!lower.isValid() && !upper.isValid())
        m_private.reset();
    else
        m_private = IDBKeyRange::create(lower, upper, lowerOpen ? IDBKeyRange::LowerBoundOpen : IDBKeyRange::LowerBoundClosed, upperOpen ? IDBKeyRange::UpperBoundOpen : IDBKeyRange::UpperBoundClosed);
}

void WebIDBKeyRange::reset()
{
    m_private.reset();
}

WebIDBKey WebIDBKeyRange::lower() const
{
    if (!m_private.get())
        return WebIDBKey::createInvalid();
    return m_private->lower();
}

WebIDBKey WebIDBKeyRange::upper() const
{
    if (!m_private.get())
        return WebIDBKey::createInvalid();
    return m_private->upper();
}

bool WebIDBKeyRange::lowerOpen() const
{
    return m_private.get() && m_private->lowerOpen();
}

bool WebIDBKeyRange::upperOpen() const
{
    return m_private.get() && m_private->upperOpen();
}

WebIDBKeyRange::WebIDBKeyRange(const PassRefPtr<IDBKeyRange>& value)
    : m_private(value)
{
}

WebIDBKeyRange& WebIDBKeyRange::operator=(const PassRefPtr<IDBKeyRange>& value)
{
    m_private = value;
    return *this;
}

WebIDBKeyRange::operator PassRefPtr<IDBKeyRange>() const
{
    return m_private.get();
}

} // namespace blink
