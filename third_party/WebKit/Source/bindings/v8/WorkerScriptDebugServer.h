/*
 * Copyright (c) 2011 Google Inc. All rights reserved.
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

#ifndef WorkerScriptDebugServer_h
#define WorkerScriptDebugServer_h

#include "bindings/v8/ScriptDebugServer.h"

namespace v8 {
class Isolate;
}

namespace WebCore {

class WorkerGlobalScope;
class WorkerThread;

class WorkerScriptDebugServer FINAL : public ScriptDebugServer {
    WTF_MAKE_NONCOPYABLE(WorkerScriptDebugServer);
public:
    WorkerScriptDebugServer(WorkerGlobalScope*, const String&);
    virtual ~WorkerScriptDebugServer() { }

    void addListener(ScriptDebugListener*);
    void removeListener(ScriptDebugListener*);

    void interruptAndRunTask(PassOwnPtr<Task>);

private:
    virtual ScriptDebugListener* getDebugListenerForContext(v8::Handle<v8::Context>) OVERRIDE;
    virtual void runMessageLoopOnPause(v8::Handle<v8::Context>) OVERRIDE;
    virtual void quitMessageLoopOnPause() OVERRIDE;

    typedef HashMap<WorkerGlobalScope*, ScriptDebugListener*> ListenersMap;
    ScriptDebugListener* m_listener;
    WorkerGlobalScope* m_workerGlobalScope;
    String m_debuggerTaskMode;
};

} // namespace WebCore

#endif // WorkerScriptDebugServer_h
