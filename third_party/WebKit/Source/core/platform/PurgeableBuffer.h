/*
 * Copyright (C) 2008, 2010 Apple Inc. All Rights Reserved.
 * Copyright (C) 2013 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PurgeableBuffer_h
#define PurgeableBuffer_h

#include "wtf/OwnPtr.h"
#include "wtf/PassOwnPtr.h"

namespace WebKit {
class WebDiscardableMemory;
}

namespace WebCore {

class PurgeableBuffer {
    WTF_MAKE_NONCOPYABLE(PurgeableBuffer);
public:
    static PassOwnPtr<PurgeableBuffer> create(const char* data, size_t);
    ~PurgeableBuffer();

    // Call lock and check the return value before accessing the data.
    const char* data() const;
    size_t size() const { return m_size; }

    bool isPurgeable() const { return m_state != Locked; }
    bool wasPurged() const;

    bool lock();
    void unlock();

private:
    enum State {
        Locked,
        Unlocked,
        Purged
    };

    PurgeableBuffer(PassOwnPtr<WebKit::WebDiscardableMemory>, const char* data, size_t);

    OwnPtr<WebKit::WebDiscardableMemory> m_memory;
    size_t m_size;
    State m_state;
};

}

#endif
