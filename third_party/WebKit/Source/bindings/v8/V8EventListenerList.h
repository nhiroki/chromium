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

#ifndef V8EventListenerList_h
#define V8EventListenerList_h

#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8EventListener.h"

namespace WebCore {

class Frame;

enum ListenerLookupType {
    ListenerFindOnly,
    ListenerFindOrCreate,
};

// This is a container for V8EventListener objects that uses hidden properties of v8::Object to speed up lookups.
class V8EventListenerList {
public:
    static PassRefPtr<V8EventListener> findWrapper(v8::Local<v8::Value> value, v8::Isolate* isolate)
    {
        ASSERT(isolate->InContext());
        if (!value->IsObject())
            return 0;

        v8::Handle<v8::String> wrapperProperty = getHiddenProperty(false, isolate);
        return doFindWrapper(v8::Local<v8::Object>::Cast(value), wrapperProperty, isolate);
    }

    template<typename WrapperType>
    static PassRefPtr<V8EventListener> findOrCreateWrapper(v8::Local<v8::Value>, bool isAttribute, v8::Isolate*);

    static void clearWrapper(v8::Handle<v8::Object> listenerObject, bool isAttribute, v8::Isolate* isolate)
    {
        v8::Handle<v8::String> wrapperProperty = getHiddenProperty(isAttribute, isolate);
        deleteHiddenValue(isolate, listenerObject, wrapperProperty);
    }

    static PassRefPtr<EventListener> getEventListener(v8::Local<v8::Value>, bool isAttribute, ListenerLookupType);

private:
    static V8EventListener* doFindWrapper(v8::Local<v8::Object> object, v8::Handle<v8::String> wrapperProperty, v8::Isolate* isolate)
    {
        ASSERT(isolate->InContext());
        v8::HandleScope scope(isolate);
        v8::Local<v8::Value> listener = getHiddenValue(isolate, object, wrapperProperty);
        if (listener.IsEmpty())
            return 0;
        return static_cast<V8EventListener*>(v8::External::Cast(*listener)->Value());
    }

    static inline v8::Handle<v8::String> getHiddenProperty(bool isAttribute, v8::Isolate* isolate)
    {
        return isAttribute ? v8AtomicString(isolate, "attributeListener") : v8AtomicString(isolate, "listener");
    }
};

template<typename WrapperType>
PassRefPtr<V8EventListener> V8EventListenerList::findOrCreateWrapper(v8::Local<v8::Value> value, bool isAttribute, v8::Isolate* isolate)
{
    ASSERT(isolate->InContext());
    if (!value->IsObject()
        // Non-callable attribute setter input is treated as null (no wrapper)
        || (isAttribute && !value->IsFunction()))
        return 0;

    v8::Local<v8::Object> object = v8::Local<v8::Object>::Cast(value);
    v8::Handle<v8::String> wrapperProperty = getHiddenProperty(isAttribute, isolate);

    V8EventListener* wrapper = doFindWrapper(object, wrapperProperty, isolate);
    if (wrapper)
        return wrapper;

    RefPtr<V8EventListener> wrapperPtr = WrapperType::create(object, isAttribute, isolate);
    if (wrapperPtr)
        setHiddenValue(isolate, object, wrapperProperty, v8::External::New(isolate, wrapperPtr.get()));

    return wrapperPtr;
}

} // namespace WebCore

#endif // V8EventListenerList_h
