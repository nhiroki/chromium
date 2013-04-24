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

#ifndef DOMTimer_h
#define DOMTimer_h

#include "SuspendableTimer.h"
#include "UserGestureIndicator.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

    class ScheduledAction;

    class DOMTimer : public SuspendableTimer {
    public:
        virtual ~DOMTimer();
        // Creates a new timer owned by specified ScriptExecutionContext, starts it
        // and returns its Id.
        static int install(ScriptExecutionContext*, PassOwnPtr<ScheduledAction>, int timeout, bool singleShot);
        static void removeById(ScriptExecutionContext*, int timeoutId);

        // ActiveDOMObject
        virtual void contextDestroyed();
        virtual void stop();

        virtual void reportMemoryUsage(MemoryObjectInfo*) const OVERRIDE;

        // The following are essentially constants. All intervals are in seconds.
        static double hiddenPageAlignmentInterval();
        static double visiblePageAlignmentInterval();

    private:
        DOMTimer(ScriptExecutionContext*, PassOwnPtr<ScheduledAction>, int interval, bool singleShot);
        virtual void fired();

        // Retuns timer fire time rounded to the next multiple of timer alignment interval.
        virtual double alignedFireTime(double) const;

        int m_timeoutId;
        int m_nestingLevel;
        OwnPtr<ScheduledAction> m_action;
        RefPtr<UserGestureToken> m_userGestureToken;
    };

} // namespace WebCore

#endif // DOMTimer_h

