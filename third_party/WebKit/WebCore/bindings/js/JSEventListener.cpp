/*
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All Rights Reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "JSEventListener.h"

#include "Event.h"
#include "Frame.h"
#include "JSEvent.h"
#include "JSEventTarget.h"
#include <runtime/JSLock.h>
#include <wtf/RefCountedLeakCounter.h>

using namespace JSC;

namespace WebCore {

void JSAbstractEventListener::handleEvent(Event* event, bool isWindowEvent)
{
    JSLock lock(false);

    JSObject* jsFunction = this->jsFunction();
    if (!jsFunction)
        return;

    JSDOMGlobalObject* globalObject = this->globalObject();
    // Null check as clearGlobalObject() can clear this and we still get called back by
    // xmlhttprequest objects. See http://bugs.webkit.org/show_bug.cgi?id=13275
    // FIXME: Is this check still necessary? Requests are supposed to be stopped before clearGlobalObject() is called.
    if (!globalObject)
        return;

    ScriptExecutionContext* scriptExecutionContext = globalObject->scriptExecutionContext();
    if (!scriptExecutionContext)
        return;

    if (scriptExecutionContext->isDocument()) {
        JSDOMWindow* window = static_cast<JSDOMWindow*>(globalObject);
        Frame* frame = window->impl()->frame();
        if (!frame)
            return;
        // The window must still be active in its frame. See <https://bugs.webkit.org/show_bug.cgi?id=21921>.
        // FIXME: A better fix for this may be to change DOMWindow::frame() to not return a frame the detached window used to be in.
        if (frame->domWindow() != window->impl())
            return;
        // FIXME: Is this check needed for other contexts?
        ScriptController* script = frame->script();
        if (!script->isEnabled() || script->isPaused())
            return;
    }

    ExecState* exec = globalObject->globalExec();

    JSValuePtr handleEventFunction = jsFunction->get(exec, Identifier(exec, "handleEvent"));
    CallData callData;
    CallType callType = handleEventFunction.getCallData(callData);
    if (callType == CallTypeNone) {
        handleEventFunction = noValue();
        callType = jsFunction->getCallData(callData);
    }

    if (callType != CallTypeNone) {
        ref();

        ArgList args;
        args.append(toJS(exec, event));

        Event* savedEvent = globalObject->currentEvent();
        globalObject->setCurrentEvent(event);

        // If this event handler is the first JavaScript to execute, then the
        // dynamic global object should be set to the global object of the
        // window in which the event occurred.
        JSGlobalData* globalData = globalObject->globalData();
        DynamicGlobalObjectScope globalObjectScope(exec, globalData->dynamicGlobalObject ? globalData->dynamicGlobalObject : globalObject);

        JSValuePtr retval;
        if (handleEventFunction) {
            globalObject->globalData()->timeoutChecker.start();
            retval = call(exec, handleEventFunction, callType, callData, jsFunction, args);
        } else {
            JSValuePtr thisValue;
            if (isWindowEvent)
                thisValue = globalObject->toThisObject(exec);
            else
                thisValue = toJS(exec, event->currentTarget());
            globalObject->globalData()->timeoutChecker.start();
            retval = call(exec, jsFunction, callType, callData, thisValue, args);
        }
        globalObject->globalData()->timeoutChecker.stop();

        globalObject->setCurrentEvent(savedEvent);

        if (exec->hadException())
            reportCurrentException(exec);
        else {
            if (!retval.isUndefinedOrNull() && event->storesResultAsString())
                event->storeResult(retval.toString(exec));
            if (m_isInline) {
                bool retvalbool;
                if (retval.getBoolean(retvalbool) && !retvalbool)
                    event->preventDefault();
            }
        }

        if (scriptExecutionContext->isDocument())
            Document::updateStyleForAllDocuments();
        deref();
    }
}

bool JSAbstractEventListener::virtualIsInline() const
{
    return m_isInline;
}

// -------------------------------------------------------------------------

JSEventListener::JSEventListener(JSObject* function, JSDOMGlobalObject* globalObject, bool isInline)
    : JSAbstractEventListener(isInline)
    , m_jsFunction(function)
    , m_globalObject(globalObject)
{
    if (m_jsFunction) {
        JSDOMWindow::JSListenersMap& listeners = isInline
            ? globalObject->jsInlineEventListeners() : globalObject->jsEventListeners();
        listeners.set(m_jsFunction, this);
    }
}

inline void JSEventListener::clearJSFunctionInline()
{
    if (m_jsFunction && m_globalObject) {
        JSDOMWindow::JSListenersMap& listeners = isInline()
            ? m_globalObject->jsInlineEventListeners() : m_globalObject->jsEventListeners();
        listeners.remove(m_jsFunction);
    }
    
    m_jsFunction = 0;
    m_globalObject = 0;
}

JSEventListener::~JSEventListener()
{
    clearJSFunctionInline();
}

JSObject* JSEventListener::jsFunction() const
{
    return m_jsFunction;
}

void JSEventListener::clearJSFunction()
{
    clearJSFunctionInline();
}

JSDOMGlobalObject* JSEventListener::globalObject() const
{
    return m_globalObject;
}

void JSEventListener::clearGlobalObject()
{
    m_globalObject = 0;
}

void JSEventListener::markJSFunction()
{
    if (m_jsFunction && !m_jsFunction->marked())
        m_jsFunction->mark();
    if (m_globalObject && !m_globalObject->marked())
        m_globalObject->mark();
}

#ifndef NDEBUG
static WTF::RefCountedLeakCounter eventListenerCounter("EventListener");
#endif

// -------------------------------------------------------------------------

JSProtectedEventListener::JSProtectedEventListener(JSObject* listener, JSDOMGlobalObject* globalObject, bool isInline)
    : JSAbstractEventListener(isInline)
    , m_jsFunction(listener)
    , m_globalObject(globalObject)
{
    if (m_jsFunction) {
        JSDOMWindow::ProtectedListenersMap& listeners = isInline
            ? m_globalObject->jsProtectedInlineEventListeners() : m_globalObject->jsProtectedEventListeners();
        listeners.set(m_jsFunction, this);
    }
#ifndef NDEBUG
    eventListenerCounter.increment();
#endif
}

JSProtectedEventListener::~JSProtectedEventListener()
{
    if (m_jsFunction && m_globalObject) {
        JSDOMWindow::ProtectedListenersMap& listeners = isInline()
            ? m_globalObject->jsProtectedInlineEventListeners() : m_globalObject->jsProtectedEventListeners();
        listeners.remove(m_jsFunction);
    }
#ifndef NDEBUG
    eventListenerCounter.decrement();
#endif
}

JSObject* JSProtectedEventListener::jsFunction() const
{
    return m_jsFunction;
}

JSDOMGlobalObject* JSProtectedEventListener::globalObject() const
{
    return m_globalObject;
}

void JSProtectedEventListener::clearGlobalObject()
{
    m_globalObject = 0;
}

} // namespace WebCore
