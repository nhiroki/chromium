/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
 */

#ifndef InspectorClient_h
#define InspectorClient_h

#include "InspectorController.h"
#include <wtf/Forward.h>

namespace WebCore {

class InspectorController;
class Node;
class Page;

class InspectorClient {
public:
    virtual ~InspectorClient() {  }

    virtual void inspectorDestroyed() = 0;

    virtual void openInspectorFrontend(InspectorController*) = 0;

    virtual void highlight(Node*) = 0;
    virtual void hideHighlight() = 0;

    virtual void populateSetting(const String& key, String* value) = 0;
    virtual void storeSetting(const String& key, const String& value) = 0;

    virtual bool sendMessageToFrontend(const String& message) = 0;

    // Navigation can cause some WebKit implementations to change the view / page / inspector controller instance.
    // However, there are some inspector controller states that should survive navigation (such as tracking resources
    // or recording timeline). Following callbacks allow embedders to track these states.
    virtual void resourceTrackingWasEnabled() { };
    virtual void resourceTrackingWasDisabled() { };
    virtual void timelineProfilerWasStarted() { };
    virtual void timelineProfilerWasStopped() { };
};

} // namespace WebCore

#endif // !defined(InspectorClient_h)
