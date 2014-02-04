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

// This file has been auto-generated by code_generator_v8.pm. DO NOT MODIFY!

#ifndef V8TestObjectPython_h
#define V8TestObjectPython_h

#include "bindings/tests/idls/TestObjectPython.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "bindings/v8/WrapperTypeInfo.h"
#include "heap/Handle.h"

namespace WebCore {

class V8TestObjectPython {
public:
    static bool hasInstance(v8::Handle<v8::Value>, v8::Isolate*);
    static v8::Handle<v8::FunctionTemplate> domTemplate(v8::Isolate*, WrapperWorldType);
    static TestObjectPython* toNative(v8::Handle<v8::Object> object)
    {
        return fromInternalPointer(object->GetAlignedPointerFromInternalField(v8DOMWrapperObjectIndex));
    }
    static const WrapperTypeInfo wrapperTypeInfo;
    static void derefObject(void*);
    static void customVoidMethodMethodCustom(const v8::FunctionCallbackInfo<v8::Value>&);
#if ENABLE(CONDITION)
    static void conditionalConditionCustomVoidMethodMethodCustom(const v8::FunctionCallbackInfo<v8::Value>&);
#endif // ENABLE(CONDITION)
    static void customObjectAttributeAttributeGetterCustom(const v8::PropertyCallbackInfo<v8::Value>&);
    static void customObjectAttributeAttributeSetterCustom(v8::Local<v8::Value>, const v8::PropertyCallbackInfo<void>&);
    static void customGetterLongAttributeAttributeGetterCustom(const v8::PropertyCallbackInfo<v8::Value>&);
    static void customGetterReadonlyObjectAttributeAttributeGetterCustom(const v8::PropertyCallbackInfo<v8::Value>&);
    static void customSetterLongAttributeAttributeSetterCustom(v8::Local<v8::Value>, const v8::PropertyCallbackInfo<void>&);
#if ENABLE(CONDITION)
    static void customLongAttributeAttributeGetterCustom(const v8::PropertyCallbackInfo<v8::Value>&);
#endif // ENABLE(CONDITION)
#if ENABLE(CONDITION)
    static void customLongAttributeAttributeSetterCustom(v8::Local<v8::Value>, const v8::PropertyCallbackInfo<void>&);
#endif // ENABLE(CONDITION)
    static void customImplementedAsLongAttributeAttributeGetterCustom(const v8::PropertyCallbackInfo<v8::Value>&);
    static void customImplementedAsLongAttributeAttributeSetterCustom(v8::Local<v8::Value>, const v8::PropertyCallbackInfo<void>&);
    static void customGetterImplementedAsLongAttributeAttributeGetterCustom(const v8::PropertyCallbackInfo<v8::Value>&);
    static void customSetterImplementedAsLongAttributeAttributeSetterCustom(v8::Local<v8::Value>, const v8::PropertyCallbackInfo<void>&);
    static const int internalFieldCount = v8DefaultWrapperInternalFieldCount + 0;
    static inline void* toInternalPointer(TestObjectPython* impl)
    {
        return impl;
    }

    static inline TestObjectPython* fromInternalPointer(void* object)
    {
        return static_cast<TestObjectPython*>(object);
    }
    static void installPerContextEnabledProperties(v8::Handle<v8::Object>, TestObjectPython*, v8::Isolate*);
    static void installPerContextEnabledMethods(v8::Handle<v8::Object>, v8::Isolate*);

private:
    friend v8::Handle<v8::Object> wrap(TestObjectPython*, v8::Handle<v8::Object> creationContext, v8::Isolate*);
    static v8::Handle<v8::Object> createWrapper(PassRefPtr<TestObjectPython>, v8::Handle<v8::Object> creationContext, v8::Isolate*);
};

template<>
class WrapperTypeTraits<TestObjectPython > {
public:
    static const WrapperTypeInfo* wrapperTypeInfo() { return &V8TestObjectPython::wrapperTypeInfo; }
};

inline v8::Handle<v8::Object> wrap(TestObjectPython* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8TestObjectPython>(impl, isolate));
    return V8TestObjectPython::createWrapper(impl, creationContext, isolate);
}

inline v8::Handle<v8::Value> toV8(TestObjectPython* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    if (UNLIKELY(!impl))
        return v8::Null(isolate);
    v8::Handle<v8::Value> wrapper = DOMDataStore::getWrapper<V8TestObjectPython>(impl, isolate);
    if (!wrapper.IsEmpty())
        return wrapper;
    return wrap(impl, creationContext, isolate);
}

template<typename CallbackInfo>
inline void v8SetReturnValue(const CallbackInfo& callbackInfo, TestObjectPython* impl)
{
    if (UNLIKELY(!impl)) {
        v8SetReturnValueNull(callbackInfo);
        return;
    }
    if (DOMDataStore::setReturnValueFromWrapper<V8TestObjectPython>(callbackInfo.GetReturnValue(), impl))
        return;
    v8::Handle<v8::Object> wrapper = wrap(impl, callbackInfo.Holder(), callbackInfo.GetIsolate());
    v8SetReturnValue(callbackInfo, wrapper);
}

template<typename CallbackInfo>
inline void v8SetReturnValueForMainWorld(const CallbackInfo& callbackInfo, TestObjectPython* impl)
{
    ASSERT(worldType(callbackInfo.GetIsolate()) == MainWorld);
    if (UNLIKELY(!impl)) {
        v8SetReturnValueNull(callbackInfo);
        return;
    }
    if (DOMDataStore::setReturnValueFromWrapperForMainWorld<V8TestObjectPython>(callbackInfo.GetReturnValue(), impl))
        return;
    v8::Handle<v8::Value> wrapper = wrap(impl, callbackInfo.Holder(), callbackInfo.GetIsolate());
    v8SetReturnValue(callbackInfo, wrapper);
}

template<class CallbackInfo, class Wrappable>
inline void v8SetReturnValueFast(const CallbackInfo& callbackInfo, TestObjectPython* impl, Wrappable* wrappable)
{
    if (UNLIKELY(!impl)) {
        v8SetReturnValueNull(callbackInfo);
        return;
    }
    if (DOMDataStore::setReturnValueFromWrapperFast<V8TestObjectPython>(callbackInfo.GetReturnValue(), impl, callbackInfo.Holder(), wrappable))
        return;
    v8::Handle<v8::Object> wrapper = wrap(impl, callbackInfo.Holder(), callbackInfo.GetIsolate());
    v8SetReturnValue(callbackInfo, wrapper);
}

inline v8::Handle<v8::Value> toV8(PassRefPtr<TestObjectPython> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl.get(), creationContext, isolate);
}

template<class CallbackInfo>
inline void v8SetReturnValue(const CallbackInfo& callbackInfo, PassRefPtr<TestObjectPython> impl)
{
    v8SetReturnValue(callbackInfo, impl.get());
}

template<class CallbackInfo>
inline void v8SetReturnValueForMainWorld(const CallbackInfo& callbackInfo, PassRefPtr<TestObjectPython> impl)
{
    v8SetReturnValueForMainWorld(callbackInfo, impl.get());
}

template<class CallbackInfo, class Wrappable>
inline void v8SetReturnValueFast(const CallbackInfo& callbackInfo, PassRefPtr<TestObjectPython> impl, Wrappable* wrappable)
{
    v8SetReturnValueFast(callbackInfo, impl.get(), wrappable);
}

}
#endif // V8TestObjectPython_h
