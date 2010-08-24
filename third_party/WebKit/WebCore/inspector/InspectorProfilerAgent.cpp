/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
#include "InspectorProfilerAgent.h"

#if ENABLE(JAVASCRIPT_DEBUGGER)

#include "Console.h"
#include "InspectorController.h"
#include "InspectorFrontend.h"
#include "InspectorValues.h"
#include "KURL.h"
#include "Page.h"
#include "ScriptDebugServer.h"
#include "ScriptProfile.h"
#include "ScriptProfiler.h"
#include <wtf/OwnPtr.h>

#if USE(JSC)
#include "JSDOMWindow.h"
#endif

namespace WebCore {

static const char* const UserInitiatedProfileName = "org.webkit.profiles.user-initiated";
static const char* const CPUProfileType = "CPU";

PassOwnPtr<InspectorProfilerAgent> InspectorProfilerAgent::create(InspectorController* inspectorController)
{
    OwnPtr<InspectorProfilerAgent> agent = adoptPtr(new InspectorProfilerAgent(inspectorController));
    return agent.release();
}

InspectorProfilerAgent::InspectorProfilerAgent(InspectorController* inspectorController)
    : m_inspectorController(inspectorController)
    , m_frontend(0)
    , m_enabled(ScriptProfiler::isProfilerAlwaysEnabled())
    , m_recordingUserInitiatedProfile(false)
    , m_currentUserInitiatedProfileNumber(-1)
    , m_nextUserInitiatedProfileNumber(1)
{
}

InspectorProfilerAgent::~InspectorProfilerAgent()
{
}

void InspectorProfilerAgent::addProfile(PassRefPtr<ScriptProfile> prpProfile, unsigned lineNumber, const String& sourceURL)
{
    RefPtr<ScriptProfile> profile = prpProfile;
    m_profiles.add(profile->uid(), profile);
    if (m_frontend)
        m_frontend->addProfileHeader(createProfileHeader(*profile));
    addProfileFinishedMessageToConsole(profile, lineNumber, sourceURL);
}

void InspectorProfilerAgent::addProfileFinishedMessageToConsole(PassRefPtr<ScriptProfile> prpProfile, unsigned lineNumber, const String& sourceURL)
{
    RefPtr<ScriptProfile> profile = prpProfile;
    String title = profile->title();
    String message = String::format("Profile \"webkit-profile://%s/%s#%d\" finished.", CPUProfileType, encodeWithURLEscapeSequences(title).utf8().data(), profile->uid());
    m_inspectorController->addMessageToConsole(JSMessageSource, LogMessageType, LogMessageLevel, message, lineNumber, sourceURL);
}

void InspectorProfilerAgent::addStartProfilingMessageToConsole(const String& title, unsigned lineNumber, const String& sourceURL)
{
    String message = String::format("Profile \"webkit-profile://%s/%s#0\" started.", CPUProfileType, encodeWithURLEscapeSequences(title).utf8().data());
    m_inspectorController->addMessageToConsole(JSMessageSource, LogMessageType, LogMessageLevel, message, lineNumber, sourceURL);
}

PassRefPtr<InspectorObject> InspectorProfilerAgent::createProfileHeader(const ScriptProfile& profile)
{
    RefPtr<InspectorObject> header = InspectorObject::create();
    header->setString("title", profile.title());
    header->setNumber("uid", profile.uid());
    header->setString("typeId", String(CPUProfileType));
    return header;
}

void InspectorProfilerAgent::disable()
{
    if (!m_enabled)
        return;
    m_enabled = false;
    ScriptDebugServer::shared().recompileAllJSFunctionsSoon();
    if (m_frontend)
        m_frontend->profilerWasDisabled();
}

void InspectorProfilerAgent::enable(bool skipRecompile)
{
    if (m_enabled)
        return;
    m_enabled = true;
    if (!skipRecompile)
        ScriptDebugServer::shared().recompileAllJSFunctionsSoon();
    if (m_frontend)
        m_frontend->profilerWasEnabled();
}

String InspectorProfilerAgent::getCurrentUserInitiatedProfileName(bool incrementProfileNumber)
{
    if (incrementProfileNumber)
        m_currentUserInitiatedProfileNumber = m_nextUserInitiatedProfileNumber++;

    return String::format("%s.%d", UserInitiatedProfileName, m_currentUserInitiatedProfileNumber);
}

void InspectorProfilerAgent::getProfileHeaders(RefPtr<InspectorArray>* headers)
{
    ProfilesMap::iterator profilesEnd = m_profiles.end();
    for (ProfilesMap::iterator it = m_profiles.begin(); it != profilesEnd; ++it)
        (*headers)->pushObject(createProfileHeader(*it->second));
}

void InspectorProfilerAgent::getProfile(unsigned uid, RefPtr<InspectorObject>* profileObject)
{
    ProfilesMap::iterator it = m_profiles.find(uid);
    if (it != m_profiles.end()) {
        *profileObject = createProfileHeader(*it->second);
        (*profileObject)->setObject("head", it->second->buildInspectorObjectForHead());
    }
}

void InspectorProfilerAgent::removeProfile(unsigned uid)
{
    if (m_profiles.contains(uid))
        m_profiles.remove(uid);
}

void InspectorProfilerAgent::resetState()
{
    m_profiles.clear();
    m_currentUserInitiatedProfileNumber = 1;
    m_nextUserInitiatedProfileNumber = 1;
    if (m_frontend)
        m_frontend->resetProfilesPanel();
}

void InspectorProfilerAgent::startUserInitiatedProfiling()
{
    if (!enabled()) {
        enable(false);
        ScriptDebugServer::shared().recompileAllJSFunctions();
    }
    m_recordingUserInitiatedProfile = true;
    String title = getCurrentUserInitiatedProfileName(true);
#if USE(JSC)
    JSC::ExecState* scriptState = toJSDOMWindow(m_inspectorController->inspectedPage()->mainFrame(), debuggerWorld())->globalExec();
#else
    ScriptState* scriptState = 0;
#endif
    ScriptProfiler::start(scriptState, title);
    addStartProfilingMessageToConsole(title, 0, String());
    toggleRecordButton(true);
}

void InspectorProfilerAgent::stopUserInitiatedProfiling()
{
    m_recordingUserInitiatedProfile = false;
    String title = getCurrentUserInitiatedProfileName();
#if USE(JSC)
    JSC::ExecState* scriptState = toJSDOMWindow(m_inspectorController->inspectedPage()->mainFrame(), debuggerWorld())->globalExec();
#else
    // Use null script state to avoid filtering by context security token.
    // All functions from all iframes should be visible from Inspector UI.
    ScriptState* scriptState = 0;
#endif
    RefPtr<ScriptProfile> profile = ScriptProfiler::stop(scriptState, title);
    if (profile)
        addProfile(profile, 0, String());
    toggleRecordButton(false);
}

void InspectorProfilerAgent::toggleRecordButton(bool isProfiling)
{
    if (m_frontend)
        m_frontend->setRecordingProfile(isProfiling);
}

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER)
