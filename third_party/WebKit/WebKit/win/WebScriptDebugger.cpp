/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

#include "config.h"
#include "WebKitDLL.h"
#include "WebScriptDebugger.h"

#include "WebKit.h"
#include "WebFrame.h"
#include "WebScriptCallFrame.h"
#include "WebScriptDebugServer.h"
#include "WebView.h"

#pragma warning(push, 0)
#include <WebCore/BString.h>
#include <WebCore/DOMWindow.h>
#include <WebCore/kjs_binding.h>
#include <WebCore/kjs_proxy.h>
#include <WebCore/PlatformString.h>
#pragma warning(pop)

using namespace WebCore;
using namespace KJS;

WebScriptDebugger::WebScriptDebugger()
    : m_callingServer(false)
{
}

WebScriptDebugger::~WebScriptDebugger()
{
}

static Frame* frame(ExecState* exec)
{
    JSDOMWindow* window = static_cast<JSDOMWindow*>(exec->dynamicGlobalObject());
    return window->impl()->frame();
}

static WebFrame* webFrame(ExecState* exec)
{
    return kit(frame(exec));
}

static WebView* webView(ExecState* exec)
{
    return kit(frame(exec)->page());
}

bool WebScriptDebugger::sourceParsed(ExecState* exec, int sourceId, const UString& sourceURL,
                  const UString& source, int startingLineNumber, int errorLine, const UString& /*errorMsg*/)
{
    if (m_callingServer)
        return true;

    m_callingServer = true;

    if (WebScriptDebugServer::listenerCount() <= 0)
        return true;

    BString bSource = String(source);
    BString bSourceURL = String(sourceURL);
    
    if (errorLine == -1) {
        WebScriptDebugServer::sharedWebScriptDebugServer()->didParseSource(webView(exec),
            bSource,
            startingLineNumber,
            bSourceURL,
            sourceId,
            webFrame(exec));
    } else {
        // FIXME: the error var should be made with the information in the errorMsg.  It is not a simple
        // UString to BSTR conversion there is some logic involved that I don't fully understand yet.
        BString error(L"An Error Occurred.");
        WebScriptDebugServer::sharedWebScriptDebugServer()->failedToParseSource(webView(exec),
            bSource,
            startingLineNumber,
            bSourceURL,
            error,
            webFrame(exec));
    }

    m_callingServer = false;
    return true;
}

bool WebScriptDebugger::callEvent(ExecState* exec, int sourceId, int lineno, JSObject* /*function*/, const List &/*args*/)
{
    if (m_callingServer)
        return true;

    m_callingServer = true;

    COMPtr<WebScriptCallFrame> callFrame(AdoptCOM, WebScriptCallFrame::createInstance(exec));
    WebScriptDebugServer::sharedWebScriptDebugServer()->didEnterCallFrame(webView(exec), callFrame.get(), sourceId, lineno, webFrame(exec));

    m_callingServer = false;

    return true;
}

bool WebScriptDebugger::atStatement(ExecState* exec, int sourceId, int firstLine, int /*lastLine*/)
{
    if (m_callingServer)
        return true;

    m_callingServer = true;

    COMPtr<WebScriptCallFrame> callFrame(AdoptCOM, WebScriptCallFrame::createInstance(exec));
    WebScriptDebugServer::sharedWebScriptDebugServer()->willExecuteStatement(webView(exec), callFrame.get(), sourceId, firstLine, webFrame(exec));

    m_callingServer = false;

    return true;
}

bool WebScriptDebugger::returnEvent(ExecState* exec, int sourceId, int lineno, JSObject* /*function*/)
{
    if (m_callingServer)
        return true;

    m_callingServer = true;

    COMPtr<WebScriptCallFrame> callFrame(AdoptCOM, WebScriptCallFrame::createInstance(exec->callingExecState()));
    WebScriptDebugServer::sharedWebScriptDebugServer()->willLeaveCallFrame(webView(exec), callFrame.get(), sourceId, lineno, webFrame(exec));

    m_callingServer = false;

    return true;
}

bool WebScriptDebugger::exception(ExecState* exec, int sourceId, int lineno, JSValue* /*exception */)
{
    if (m_callingServer)
        return true;

    m_callingServer = true;

    COMPtr<WebScriptCallFrame> callFrame(AdoptCOM, WebScriptCallFrame::createInstance(exec));
    WebScriptDebugServer::sharedWebScriptDebugServer()->exceptionWasRaised(webView(exec), callFrame.get(), sourceId, lineno, webFrame(exec));

    m_callingServer = false;

    return true;
}
