/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef CryptoResult_h
#define CryptoResult_h

#include "bindings/v8/ScriptObject.h"
#include "public/platform/WebCrypto.h"
#include "wtf/Forward.h"
#include "wtf/ThreadSafeRefCounted.h"

namespace WebCore {

class ScriptPromiseResolver;

// Wrapper around a Promise to notify completion of the crypto operation.
class CryptoResult : public ThreadSafeRefCounted<CryptoResult> {
public:
    ~CryptoResult();

    static PassRefPtr<CryptoResult> create();

    void completeWithError();
    void completeWithBuffer(const WebKit::WebArrayBuffer&);
    void completeWithBoolean(bool);
    void completeWithKey(const WebKit::WebCryptoKey&);

    WebKit::WebCryptoResult result()
    {
        return WebKit::WebCryptoResult(this);
    }

    ScriptObject promise();

private:
    CryptoResult();
    void finish();

    RefPtr<ScriptPromiseResolver> m_promiseResolver;
    bool m_finished;
};

} // namespace WebCore

#endif
