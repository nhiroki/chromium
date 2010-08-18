/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "EventListenerWrapper.h"

#include "Event.h"
#include "EventListener.h"

#include "WebDOMEvent.h"
#include "WebDOMEventListener.h"
#include "WebEvent.h"
#include "WebEventListener.h"

namespace WebKit {

EventListenerWrapper::EventListenerWrapper(WebDOMEventListener* webDOMEventListener)
    : EventListener(EventListener::JSEventListenerType)
    , m_webDOMEventListener(webDOMEventListener)
{
}

EventListenerWrapper::~EventListenerWrapper()
{
    if (m_webDOMEventListener)
        m_webDOMEventListener->notifyEventListenerDeleted(this);
}

bool EventListenerWrapper::operator==(const EventListener& listener)
{
    return this == &listener;
}

void EventListenerWrapper::handleEvent(ScriptExecutionContext* context, Event* event)
{
    if (!m_webDOMEventListener)
        return;
    WebDOMEvent webDOMEvent(event);
    m_webDOMEventListener->handleEvent(webDOMEvent);
}

void EventListenerWrapper::webDOMEventListenerDeleted()
{
    m_webDOMEventListener = 0;
}

DeprecatedEventListenerWrapper::DeprecatedEventListenerWrapper(WebEventListener* webEventListener)
    : EventListener(EventListener::JSEventListenerType)
    , m_webEventListener(webEventListener)
{
}

DeprecatedEventListenerWrapper::~DeprecatedEventListenerWrapper()
{
    if (m_webEventListener)
        m_webEventListener->notifyEventListenerDeleted(this);
}

bool DeprecatedEventListenerWrapper::operator==(const EventListener& listener)
{
    return this == &listener;
}

void DeprecatedEventListenerWrapper::handleEvent(ScriptExecutionContext* context, Event* event)
{
    if (!m_webEventListener)
        return;
    WebEvent webEvent(event);
    m_webEventListener->handleEvent(webEvent);
}

void DeprecatedEventListenerWrapper::webEventListenerDeleted()
{
    m_webEventListener = 0;
}

} // namespace WebKit
