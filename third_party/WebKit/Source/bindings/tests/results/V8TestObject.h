// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated by code_generator_v8.py. DO NOT MODIFY!

#ifndef V8TestObject_h
#define V8TestObject_h

#include "bindings/tests/idls/TestObject.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "bindings/v8/WrapperTypeInfo.h"
#include "heap/Handle.h"

namespace WebCore {

class V8TestObject {
public:
    static bool hasInstance(v8::Handle<v8::Value>, v8::Isolate*);
    static v8::Handle<v8::Object> findInstanceInPrototypeChain(v8::Handle<v8::Value>, v8::Isolate*);
    static v8::Handle<v8::FunctionTemplate> domTemplate(v8::Isolate*);
    static TestObject* toNative(v8::Handle<v8::Object> object)
    {
        return fromInternalPointer(object->GetAlignedPointerFromInternalField(v8DOMWrapperObjectIndex));
    }
    static TestObject* toNativeWithTypeCheck(v8::Isolate*, v8::Handle<v8::Value>);
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
    static inline void* toInternalPointer(TestObject* impl)
    {
        return impl;
    }

    static inline TestObject* fromInternalPointer(void* object)
    {
        return static_cast<TestObject*>(object);
    }
    static void installPerContextEnabledProperties(v8::Handle<v8::Object>, TestObject*, v8::Isolate*);
    static void installPerContextEnabledMethods(v8::Handle<v8::Object>, v8::Isolate*);

private:
    friend v8::Handle<v8::Object> wrap(TestObject*, v8::Handle<v8::Object> creationContext, v8::Isolate*);
    static v8::Handle<v8::Object> createWrapper(PassRefPtr<TestObject>, v8::Handle<v8::Object> creationContext, v8::Isolate*);
};

inline v8::Handle<v8::Object> wrap(TestObject* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8TestObject>(impl, isolate));
    return V8TestObject::createWrapper(impl, creationContext, isolate);
}

inline v8::Handle<v8::Value> toV8(TestObject* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    if (UNLIKELY(!impl))
        return v8::Null(isolate);
    v8::Handle<v8::Value> wrapper = DOMDataStore::getWrapper<V8TestObject>(impl, isolate);
    if (!wrapper.IsEmpty())
        return wrapper;
    return wrap(impl, creationContext, isolate);
}

template<typename CallbackInfo>
inline void v8SetReturnValue(const CallbackInfo& callbackInfo, TestObject* impl)
{
    if (UNLIKELY(!impl)) {
        v8SetReturnValueNull(callbackInfo);
        return;
    }
    if (DOMDataStore::setReturnValueFromWrapper<V8TestObject>(callbackInfo.GetReturnValue(), impl))
        return;
    v8::Handle<v8::Object> wrapper = wrap(impl, callbackInfo.Holder(), callbackInfo.GetIsolate());
    v8SetReturnValue(callbackInfo, wrapper);
}

template<typename CallbackInfo>
inline void v8SetReturnValueForMainWorld(const CallbackInfo& callbackInfo, TestObject* impl)
{
    ASSERT(DOMWrapperWorld::current(callbackInfo.GetIsolate()).isMainWorld());
    if (UNLIKELY(!impl)) {
        v8SetReturnValueNull(callbackInfo);
        return;
    }
    if (DOMDataStore::setReturnValueFromWrapperForMainWorld<V8TestObject>(callbackInfo.GetReturnValue(), impl))
        return;
    v8::Handle<v8::Value> wrapper = wrap(impl, callbackInfo.Holder(), callbackInfo.GetIsolate());
    v8SetReturnValue(callbackInfo, wrapper);
}

template<class CallbackInfo, class Wrappable>
inline void v8SetReturnValueFast(const CallbackInfo& callbackInfo, TestObject* impl, Wrappable* wrappable)
{
    if (UNLIKELY(!impl)) {
        v8SetReturnValueNull(callbackInfo);
        return;
    }
    if (DOMDataStore::setReturnValueFromWrapperFast<V8TestObject>(callbackInfo.GetReturnValue(), impl, callbackInfo.Holder(), wrappable))
        return;
    v8::Handle<v8::Object> wrapper = wrap(impl, callbackInfo.Holder(), callbackInfo.GetIsolate());
    v8SetReturnValue(callbackInfo, wrapper);
}

inline v8::Handle<v8::Value> toV8(PassRefPtr<TestObject> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl.get(), creationContext, isolate);
}

template<class CallbackInfo>
inline void v8SetReturnValue(const CallbackInfo& callbackInfo, PassRefPtr<TestObject> impl)
{
    v8SetReturnValue(callbackInfo, impl.get());
}

template<class CallbackInfo>
inline void v8SetReturnValueForMainWorld(const CallbackInfo& callbackInfo, PassRefPtr<TestObject> impl)
{
    v8SetReturnValueForMainWorld(callbackInfo, impl.get());
}

template<class CallbackInfo, class Wrappable>
inline void v8SetReturnValueFast(const CallbackInfo& callbackInfo, PassRefPtr<TestObject> impl, Wrappable* wrappable)
{
    v8SetReturnValueFast(callbackInfo, impl.get(), wrappable);
}

}
#endif // V8TestObject_h
