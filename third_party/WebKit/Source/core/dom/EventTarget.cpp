/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 *           (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
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
 *
 */

#include "config.h"
#include "core/dom/EventTarget.h"

#include "bindings/v8/DOMWrapperWorld.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/ScriptController.h"
#include "core/dom/Event.h"
#include "core/dom/ExceptionCode.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "core/page/DOMWindow.h"
#include "wtf/StdLibExtras.h"
#include "wtf/Vector.h"

using namespace WTF;

namespace WebCore {

EventTargetData::EventTargetData()
{
}

EventTargetData::~EventTargetData()
{
}

EventTarget::~EventTarget()
{
}

Node* EventTarget::toNode()
{
    return 0;
}

DOMWindow* EventTarget::toDOMWindow()
{
    return 0;
}

MessagePort* EventTarget::toMessagePort()
{
    return 0;
}

inline DOMWindow* EventTarget::executingWindow()
{
    if (ScriptExecutionContext* context = scriptExecutionContext())
        return context->executingWindow();
    return 0;
}

bool EventTarget::addEventListener(const AtomicString& eventType, PassRefPtr<EventListener> listener, bool useCapture)
{
    EventTargetData* d = ensureEventTargetData();
    return d->eventListenerMap.add(eventType, listener, useCapture);
}

bool EventTarget::removeEventListener(const AtomicString& eventType, EventListener* listener, bool useCapture)
{
    EventTargetData* d = eventTargetData();
    if (!d)
        return false;

    size_t indexOfRemovedListener;

    if (!d->eventListenerMap.remove(eventType, listener, useCapture, indexOfRemovedListener))
        return false;

    // Notify firing events planning to invoke the listener at 'index' that
    // they have one less listener to invoke.
    if (!d->firingEventIterators)
        return true;
    for (size_t i = 0; i < d->firingEventIterators->size(); ++i) {
        FiringEventIterator& firingIterator = d->firingEventIterators->at(i);
        if (eventType != firingIterator.eventType)
            continue;

        if (indexOfRemovedListener >= firingIterator.end)
            continue;

        --firingIterator.end;
        if (indexOfRemovedListener <= firingIterator.iterator)
            --firingIterator.iterator;
    }

    return true;
}

bool EventTarget::setAttributeEventListener(const AtomicString& eventType, PassRefPtr<EventListener> listener, DOMWrapperWorld* isolatedWorld)
{
    clearAttributeEventListener(eventType, isolatedWorld);
    if (!listener)
        return false;
    return addEventListener(eventType, listener, false);
}

EventListener* EventTarget::getAttributeEventListener(const AtomicString& eventType, DOMWrapperWorld* isolatedWorld)
{
    const EventListenerVector& entry = getEventListeners(eventType);
    for (size_t i = 0; i < entry.size(); ++i) {
        EventListener* listener = entry[i].listener.get();
        if (listener->isAttribute()) {
            DOMWrapperWorld* listenerWorld = listener->world();
            // Worker listener
            if (!listenerWorld) {
                ASSERT(!isolatedWorld);
                return listener;
            }
            if (listenerWorld->isMainWorld() && !isolatedWorld)
                return listener;
            if (listenerWorld == isolatedWorld)
                return listener;
        }
    }
    return 0;
}

bool EventTarget::clearAttributeEventListener(const AtomicString& eventType, DOMWrapperWorld* isolatedWorld)
{
    EventListener* listener = getAttributeEventListener(eventType, isolatedWorld);
    if (!listener)
        return false;
    return removeEventListener(eventType, listener, false);
}

bool EventTarget::dispatchEvent(PassRefPtr<Event> event, ExceptionState& es)
{
    if (!event || event->type().isEmpty() || event->isBeingDispatched()) {
        es.throwDOMException(InvalidStateError);
        return false;
    }

    if (!scriptExecutionContext())
        return false;

    return dispatchEvent(event);
}

bool EventTarget::dispatchEvent(PassRefPtr<Event> event)
{
    event->setTarget(this);
    event->setCurrentTarget(this);
    event->setEventPhase(Event::AT_TARGET);
    bool defaultPrevented = fireEventListeners(event.get());
    event->setEventPhase(0);
    return defaultPrevented;
}

void EventTarget::uncaughtExceptionInEventHandler()
{
}

static AtomicString legacyType(const Event* event)
{
    if (event->type() == eventNames().transitionendEvent)
        return eventNames().webkitTransitionEndEvent;

    if (event->type() == eventNames().wheelEvent)
        return eventNames().mousewheelEvent;

    return emptyString();
}

bool EventTarget::fireEventListeners(Event* event)
{
    ASSERT(!NoEventDispatchAssertion::isEventDispatchForbidden());
    ASSERT(event && !event->type().isEmpty());

    EventTargetData* d = eventTargetData();
    if (!d)
        return true;

    EventListenerVector* legacyListenersVector = 0;
    AtomicString legacyTypeName = legacyType(event);
    if (!legacyTypeName.isEmpty())
        legacyListenersVector = d->eventListenerMap.find(legacyTypeName);

    EventListenerVector* listenersVector = d->eventListenerMap.find(event->type());

    if (listenersVector) {
        fireEventListeners(event, d, *listenersVector);
    } else if (legacyListenersVector) {
        AtomicString unprefixedTypeName = event->type();
        event->setType(legacyTypeName);
        fireEventListeners(event, d, *legacyListenersVector);
        event->setType(unprefixedTypeName);
    }

    if (legacyTypeName == eventNames().webkitTransitionEndEvent) {
        if (DOMWindow* executingWindow = this->executingWindow()) {
            if (legacyListenersVector) {
                if (listenersVector)
                    UseCounter::count(executingWindow, UseCounter::PrefixedAndUnprefixedTransitionEndEvent);
                else
                    UseCounter::count(executingWindow, UseCounter::PrefixedTransitionEndEvent);
            } else if (listenersVector) {
                UseCounter::count(executingWindow, UseCounter::UnprefixedTransitionEndEvent);
            }
        }
    }

    return !event->defaultPrevented();
}

void EventTarget::fireEventListeners(Event* event, EventTargetData* d, EventListenerVector& entry)
{
    RefPtr<EventTarget> protect = this;

    // Fire all listeners registered for this event. Don't fire listeners removed
    // during event dispatch. Also, don't fire event listeners added during event
    // dispatch. Conveniently, all new event listeners will be added after or at
    // index |size|, so iterating up to (but not including) |size| naturally excludes
    // new event listeners.

    if (event->type() == eventNames().beforeunloadEvent) {
        if (DOMWindow* executingWindow = this->executingWindow()) {
            if (executingWindow->top())
                UseCounter::count(executingWindow, UseCounter::SubFrameBeforeUnloadFired);
        }
    }

    bool userEventWasHandled = false;
    size_t i = 0;
    size_t size = entry.size();
    if (!d->firingEventIterators)
        d->firingEventIterators = adoptPtr(new FiringEventIteratorVector);
    d->firingEventIterators->append(FiringEventIterator(event->type(), i, size));
    for ( ; i < size; ++i) {
        RegisteredEventListener& registeredListener = entry[i];
        if (event->eventPhase() == Event::CAPTURING_PHASE && !registeredListener.useCapture)
            continue;
        if (event->eventPhase() == Event::BUBBLING_PHASE && registeredListener.useCapture)
            continue;

        // If stopImmediatePropagation has been called, we just break out immediately, without
        // handling any more events on this target.
        if (event->immediatePropagationStopped())
            break;

        ScriptExecutionContext* context = scriptExecutionContext();
        InspectorInstrumentationCookie cookie = InspectorInstrumentation::willHandleEvent(context, event);
        // To match Mozilla, the AT_TARGET phase fires both capturing and bubbling
        // event listeners, even though that violates some versions of the DOM spec.
        registeredListener.listener->handleEvent(context, event);
        if (!userEventWasHandled && ScriptController::processingUserGesture())
            userEventWasHandled = true;
        InspectorInstrumentation::didHandleEvent(cookie);
    }
    d->firingEventIterators->removeLast();
    if (userEventWasHandled) {
        if (ScriptExecutionContext* context = scriptExecutionContext())
            context->userEventWasHandled();
    }
}

const EventListenerVector& EventTarget::getEventListeners(const AtomicString& eventType)
{
    DEFINE_STATIC_LOCAL(EventListenerVector, emptyVector, ());

    EventTargetData* d = eventTargetData();
    if (!d)
        return emptyVector;

    EventListenerVector* listenerVector = d->eventListenerMap.find(eventType);
    if (!listenerVector)
        return emptyVector;

    return *listenerVector;
}

void EventTarget::removeAllEventListeners()
{
    EventTargetData* d = eventTargetData();
    if (!d)
        return;
    d->eventListenerMap.clear();

    // Notify firing events planning to invoke the listener at 'index' that
    // they have one less listener to invoke.
    if (d->firingEventIterators) {
        for (size_t i = 0; i < d->firingEventIterators->size(); ++i) {
            d->firingEventIterators->at(i).iterator = 0;
            d->firingEventIterators->at(i).end = 0;
        }
    }
}

} // namespace WebCore
