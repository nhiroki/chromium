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
#include "InspectorFrontendClientImpl.h"

#include "V8InspectorFrontendHost.h"
#include "WebDevToolsFrontendClient.h"
#include "WebDevToolsFrontendImpl.h"
#include "bindings/v8/ScriptController.h"
#include "core/dom/Document.h"
#include "core/inspector/InspectorFrontendHost.h"
#include "core/page/Frame.h"
#include "core/page/Page.h"
#include "core/platform/NotImplemented.h"
#include "public/platform/WebFloatPoint.h"
#include "public/platform/WebString.h"
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

InspectorFrontendClientImpl::InspectorFrontendClientImpl(Page* frontendPage, WebDevToolsFrontendClient* client, WebDevToolsFrontendImpl* frontend)
    : m_frontendPage(frontendPage)
    , m_client(client)
{
}

InspectorFrontendClientImpl::~InspectorFrontendClientImpl()
{
    if (m_frontendHost)
        m_frontendHost->disconnectClient();
    m_client = 0;
}

void InspectorFrontendClientImpl::windowObjectCleared()
{
    v8::HandleScope handleScope;
    v8::Handle<v8::Context> frameContext = m_frontendPage->mainFrame() ? m_frontendPage->mainFrame()->script()->currentWorldContext() : v8::Local<v8::Context>();
    v8::Context::Scope contextScope(frameContext);

    if (m_frontendHost)
        m_frontendHost->disconnectClient();
    m_frontendHost = InspectorFrontendHost::create(this, m_frontendPage);
    v8::Handle<v8::Value> frontendHostObj = toV8(m_frontendHost.get(), v8::Handle<v8::Object>(), frameContext->GetIsolate());
    v8::Handle<v8::Object> global = frameContext->Global();

    global->Set(v8::String::New("InspectorFrontendHost"), frontendHostObj);
}

void InspectorFrontendClientImpl::moveWindowBy(float x, float y)
{
    m_client->moveWindowBy(WebFloatPoint(x, y));
}

void InspectorFrontendClientImpl::bringToFront()
{
    m_client->activateWindow();
}

void InspectorFrontendClientImpl::closeWindow()
{
    m_client->closeWindow();
}

void InspectorFrontendClientImpl::requestSetDockSide(DockSide side)
{
    String sideString = "undocked";
    switch (side) {
    case DockedToRight: sideString = "right"; break;
    case DockedToBottom: sideString = "bottom"; break;
    case Undocked: sideString = "undocked"; break;
    }
    m_client->requestSetDockSide(sideString);
}

void InspectorFrontendClientImpl::changeAttachedWindowHeight(unsigned height)
{
    m_client->changeAttachedWindowHeight(height);
}

void InspectorFrontendClientImpl::openInNewTab(const String& url)
{
    m_client->openInNewTab(url);
}

void InspectorFrontendClientImpl::save(const String& url, const String& content, bool forceSaveAs)
{
    m_client->save(url, content, forceSaveAs);
}

void InspectorFrontendClientImpl::append(const String& url, const String& content)
{
    m_client->append(url, content);
}

void InspectorFrontendClientImpl::inspectedURLChanged(const String& url)
{
    m_frontendPage->mainFrame()->document()->setTitle("Developer Tools - " + url);
}

void InspectorFrontendClientImpl::sendMessageToBackend(const String& message)
{
    m_client->sendMessageToBackend(message);
}

void InspectorFrontendClientImpl::requestFileSystems()
{
    m_client->requestFileSystems();
}

void InspectorFrontendClientImpl::addFileSystem()
{
    m_client->addFileSystem();
}

void InspectorFrontendClientImpl::removeFileSystem(const String& fileSystemPath)
{
    m_client->removeFileSystem(fileSystemPath);
}

bool InspectorFrontendClientImpl::isUnderTest()
{
    return m_client->isUnderTest();
}

} // namespace WebKit
