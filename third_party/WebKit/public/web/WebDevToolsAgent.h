/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef WebDevToolsAgent_h
#define WebDevToolsAgent_h

#include "../platform/WebCommon.h"
#include "../platform/WebVector.h"

namespace blink {
class WebDevToolsAgentClient;
class WebDevToolsMessageTransport;
class WebFrame;
class WebString;
class WebURLRequest;
class WebURLResponse;
class WebView;
struct WebDevToolsMessageData;
struct WebPoint;
struct WebMemoryUsageInfo;
struct WebURLError;

class WebDevToolsAgent {
public:
    virtual ~WebDevToolsAgent() {}

    // Returns WebKit WebInspector protocol version.
    BLINK_EXPORT static WebString inspectorProtocolVersion();

    // Returns true if and only if the given protocol version is supported by the WebKit Web Inspector.
    BLINK_EXPORT static bool supportsInspectorProtocolVersion(const WebString& version);

    virtual void attach() = 0;
    virtual void reattach(const WebString& savedState) = 0;
    virtual void detach() = 0;

    virtual void didNavigate() = 0;

    virtual void dispatchOnInspectorBackend(const WebString& message) = 0;

    virtual void inspectElementAt(const WebPoint&) = 0;
    virtual void setProcessId(long) = 0;
    virtual void setLayerTreeId(int) = 0;

    virtual void didBeginFrame(int frameId = 0) = 0;
    virtual void didCancelFrame() = 0;
    virtual void willComposite() = 0;
    virtual void didComposite() = 0;

    // FIXME: remove it once the client side stops firing these.
    virtual void processGPUEvent(double timestamp, int phase, bool foreign) = 0;

    class GPUEvent {
    public:
        GPUEvent(double timestamp, int phase, bool foreign, size_t usedGPUMemoryBytes) :
            timestamp(timestamp),
            phase(phase),
            foreign(foreign),
            usedGPUMemoryBytes(usedGPUMemoryBytes) { }
        double timestamp;
        int phase;
        bool foreign;
        size_t usedGPUMemoryBytes;
    };
    virtual void processGPUEvent(const GPUEvent&) = 0;

    // Exposed for TestRunner.
    virtual void evaluateInWebInspector(long callId, const WebString& script) = 0;

    class MessageDescriptor {
    public:
        virtual ~MessageDescriptor() { }
        virtual WebDevToolsAgent* agent() = 0;
        virtual WebString message() = 0;
    };
    // Asynchronously request debugger to pause immediately and run the command.
    BLINK_EXPORT static void interruptAndDispatch(MessageDescriptor*);
    BLINK_EXPORT static bool shouldInterruptForMessage(const WebString&);
    BLINK_EXPORT static void processPendingMessages();

};

} // namespace blink

#endif
