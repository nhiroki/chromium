/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#include "config.h"
#include "core/dom/ErrorEvent.h"

#include "core/dom/EventNames.h"

namespace WebCore {

ErrorEventInit::ErrorEventInit()
    : message()
    , filename()
    , lineno(0)
    , colno(0)
{
}

ErrorEvent::ErrorEvent()
{
    ScriptWrappable::init(this);
}

ErrorEvent::ErrorEvent(const AtomicString& type, const ErrorEventInit& initializer)
    : Event(type, initializer)
    , m_sanitizedMessage(initializer.message)
    , m_fileName(initializer.filename)
    , m_lineNumber(initializer.lineno)
    , m_columnNumber(initializer.colno)
{
    ScriptWrappable::init(this);
}

ErrorEvent::ErrorEvent(const String& message, const String& fileName, unsigned lineNumber, unsigned columnNumber)
    : Event(eventNames().errorEvent, false, true)
    , m_sanitizedMessage(message)
    , m_fileName(fileName)
    , m_lineNumber(lineNumber)
    , m_columnNumber(columnNumber)
{
    ScriptWrappable::init(this);
}

void ErrorEvent::setUnsanitizedMessage(const String& message)
{
    ASSERT(m_unsanitizedMessage.isEmpty());
    m_unsanitizedMessage = message;
}

ErrorEvent::~ErrorEvent()
{
}

const AtomicString& ErrorEvent::interfaceName() const
{
    return eventNames().interfaceForErrorEvent;
}

} // namespace WebCore
