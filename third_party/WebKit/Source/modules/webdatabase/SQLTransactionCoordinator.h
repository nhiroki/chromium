/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef SQLTransactionCoordinator_h
#define SQLTransactionCoordinator_h

#include "heap/Handle.h"
#include "wtf/Deque.h"
#include "wtf/HashMap.h"
#include "wtf/HashSet.h"
#include "wtf/RefPtr.h"
#include "wtf/text/StringHash.h"

namespace WebCore {

class SQLTransactionBackend;

class SQLTransactionCoordinator {
    WTF_MAKE_NONCOPYABLE(SQLTransactionCoordinator); WTF_MAKE_FAST_ALLOCATED;
public:
    SQLTransactionCoordinator();
    void acquireLock(SQLTransactionBackend*);
    void releaseLock(SQLTransactionBackend*);
    void shutdown();
private:
    typedef Deque<RefPtrWillBeMember<SQLTransactionBackend> > TransactionsQueue;
    struct CoordinationInfo {
        TransactionsQueue pendingTransactions;
        WillBeHeapHashSet<RefPtrWillBeMember<SQLTransactionBackend> > activeReadTransactions;
        RefPtrWillBeMember<SQLTransactionBackend> activeWriteTransaction;

        void trace(Visitor* visitor)
        {
            visitor->trace(pendingTransactions);
            visitor->trace(activeReadTransactions);
            visitor->trace(activeWriteTransaction);
        }
        ALLOW_ONLY_INLINE_ALLOCATION();
    };
    // Maps database names to information about pending transactions
    typedef WillBePersistentHeapHashMap<String, CoordinationInfo> CoordinationInfoHeapMap;
    CoordinationInfoHeapMap m_coordinationInfoMap;
    bool m_isShuttingDown;

    void processPendingTransactions(CoordinationInfo&);
};

} // namespace WebCore

#endif // SQLTransactionCoordinator_h
