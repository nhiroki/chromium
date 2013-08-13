/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
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
#include "core/dom/PendingScript.h"

#include "core/dom/Element.h"
#include "core/fetch/ScriptResource.h"

namespace WebCore {

PendingScript::~PendingScript()
{
    if (m_resource)
        m_resource->removeClient(this);
}

PassRefPtr<Element> PendingScript::releaseElementAndClear()
{
    setScriptResource(0);
    m_watchingForLoad = false;
    m_startingPosition = TextPosition::belowRangePosition();
    return m_element.release();
}

void PendingScript::setScriptResource(ScriptResource* resource)
{
    if (m_resource == resource)
        return;
    if (m_resource)
        m_resource->removeClient(this);
    m_resource = resource;
    if (m_resource)
        m_resource->addClient(this);
}

ScriptResource* PendingScript::resource() const
{
    return m_resource.get();
}

void PendingScript::notifyFinished(Resource*)
{
}

}
