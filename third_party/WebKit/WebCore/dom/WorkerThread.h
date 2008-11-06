/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#ifndef WorkerThread_h
#define WorkerThread_h

#if ENABLE(WORKERS)

#include "PlatformString.h"
#include <wtf/PassRefPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

    class DedicatedWorker;
    class KURL;

    class WorkerThread : public ThreadSafeShared<WorkerThread> {
    public:
        static PassRefPtr<WorkerThread> create(const KURL& scriptURL, const String& sourceCode, PassRefPtr<DedicatedWorker> workerObject)
        {
            return adoptRef(new WorkerThread(scriptURL, sourceCode, workerObject));
        }

        bool start();

    private:
        WorkerThread(const KURL&, const String& sourceCode, PassRefPtr<DedicatedWorker>);

        static void* workerThreadStart(void*);
        void* workerThread();

        ThreadIdentifier m_threadID;

        String m_scriptURL;
        String m_sourceCode;
        RefPtr<DedicatedWorker> m_workerObject;
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // WorkerThread_h
