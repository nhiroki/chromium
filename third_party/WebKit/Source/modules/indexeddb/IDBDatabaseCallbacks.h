/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef IDBDatabaseCallbacks_h
#define IDBDatabaseCallbacks_h

#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"

namespace WebCore {

class DOMError;
class IDBDatabase;

class IDBDatabaseCallbacks : public RefCounted<IDBDatabaseCallbacks> {
public:
    static PassRefPtr<IDBDatabaseCallbacks> create();
    virtual ~IDBDatabaseCallbacks();

    // IDBDatabaseCallbacks
    virtual void onForcedClose();
    virtual void onVersionChange(int64_t oldVersion, int64_t newVersion);

    virtual void onAbort(int64_t transactionId, PassRefPtr<DOMError>);
    virtual void onComplete(int64_t transactionId);

    void connect(IDBDatabase*);

protected:
    // Exposed to subclasses for unit tests.
    IDBDatabaseCallbacks();

private:
    // The initial IDBOpenDBRequest or final IDBDatabase maintains a RefPtr to this
    IDBDatabase* m_database;
};

} // namespace WebCore

#endif // IDBDatabaseCallbacks_h
