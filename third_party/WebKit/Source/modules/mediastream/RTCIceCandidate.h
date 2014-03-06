/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#ifndef RTCIceCandidate_h
#define RTCIceCandidate_h

#include "bindings/v8/ScriptWrappable.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"
#include "wtf/RefPtr.h"
#include "wtf/text/WTFString.h"
#include "public/platform/WebRTCICECandidate.h"

namespace WebCore {

class Dictionary;
class ExceptionState;

class RTCIceCandidate FINAL : public RefCounted<RTCIceCandidate>, public ScriptWrappable {
public:
    static PassRefPtr<RTCIceCandidate> create(const Dictionary&, ExceptionState&);
    static PassRefPtr<RTCIceCandidate> create(blink::WebRTCICECandidate);

    String candidate() const;
    void setCandidate(String);
    String sdpMid() const;
    void setSdpMid(String);
    unsigned short sdpMLineIndex() const;
    void setSdpMLineIndex(unsigned short);

    blink::WebRTCICECandidate webCandidate() const;

private:
    explicit RTCIceCandidate(blink::WebRTCICECandidate);

    blink::WebRTCICECandidate m_webCandidate;
};

} // namespace WebCore

#endif // RTCIceCandidate_h
