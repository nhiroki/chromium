/*
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

#ifndef WebSpeechInputResult_h
#define WebSpeechInputResult_h

#include "public/platform/WebCommon.h"
#include "public/platform/WebPrivatePtr.h"
#include "public/platform/WebString.h"
#include "public/platform/WebVector.h"

namespace WebCore {
class SpeechInputResult;
}

namespace blink {

// This class holds one speech recognition result including the text and other related
// fields, as received from the embedder.
class WebSpeechInputResult {
public:
    WebSpeechInputResult() { }
    WebSpeechInputResult(const WebSpeechInputResult& other) { assign(other); }
    ~WebSpeechInputResult() { reset(); }

    WebSpeechInputResult& operator=(const WebSpeechInputResult& other)
    {
        assign(other);
        return *this;
    }

    BLINK_EXPORT void assign(const WebString& utterance, double confidence);
    BLINK_EXPORT void assign(const WebSpeechInputResult& other);
    BLINK_EXPORT void reset();

#if BLINK_IMPLEMENTATION
    explicit WebSpeechInputResult(const PassRefPtrWillBeRawPtr<WebCore::SpeechInputResult>&);
    operator PassRefPtrWillBeRawPtr<WebCore::SpeechInputResult>() const;
#endif

private:
    WebPrivatePtr<WebCore::SpeechInputResult> m_private;
};

typedef WebVector<WebSpeechInputResult> WebSpeechInputResultArray;

} // namespace blink

#endif // WebSpeechInputResult_h
