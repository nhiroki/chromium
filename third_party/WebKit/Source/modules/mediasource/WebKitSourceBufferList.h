/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef WebKitSourceBufferList_h
#define WebKitSourceBufferList_h

#include "bindings/v8/ScriptWrappable.h"
#include "core/dom/EventTarget.h"
#include "wtf/RefCounted.h"
#include "wtf/Vector.h"

namespace WebCore {

class WebKitSourceBuffer;
class GenericEventQueue;

class WebKitSourceBufferList : public RefCounted<WebKitSourceBufferList>, public ScriptWrappable, public EventTarget {
public:
    static PassRefPtr<WebKitSourceBufferList> create(ScriptExecutionContext* context, GenericEventQueue* asyncEventQueue)
    {
        return adoptRef(new WebKitSourceBufferList(context, asyncEventQueue));
    }
    virtual ~WebKitSourceBufferList() { }

    unsigned long length() const;
    WebKitSourceBuffer* item(unsigned index) const;

    void add(PassRefPtr<WebKitSourceBuffer>);
    bool remove(WebKitSourceBuffer*);
    void clear();

    // EventTarget interface
    virtual const AtomicString& interfaceName() const OVERRIDE;
    virtual ScriptExecutionContext* scriptExecutionContext() const OVERRIDE;

    using RefCounted<WebKitSourceBufferList>::ref;
    using RefCounted<WebKitSourceBufferList>::deref;

protected:
    virtual EventTargetData* eventTargetData() OVERRIDE;
    virtual EventTargetData* ensureEventTargetData() OVERRIDE;

private:
    WebKitSourceBufferList(ScriptExecutionContext*, GenericEventQueue*);

    void createAndFireEvent(const AtomicString&);

    virtual void refEventTarget() OVERRIDE { ref(); }
    virtual void derefEventTarget() OVERRIDE { deref(); }

    EventTargetData m_eventTargetData;
    ScriptExecutionContext* m_scriptExecutionContext;
    GenericEventQueue* m_asyncEventQueue;

    Vector<RefPtr<WebKitSourceBuffer> > m_list;
};

} // namespace WebCore

#endif
