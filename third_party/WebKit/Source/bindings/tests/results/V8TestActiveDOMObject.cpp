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

#include "config.h"
#include "V8TestActiveDOMObject.h"

#include "RuntimeEnabledFeatures.h"
#include "V8Node.h"
#include "bindings/v8/BindingSecurity.h"
#include "bindings/v8/ExceptionMessages.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/ScriptController.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMActivityLogger.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "platform/TraceEvent.h"
#include "wtf/UnusedParam.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestActiveDOMObject* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestActiveDOMObject::wrapperTypeInfo);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(WebCore::TestActiveDOMObject* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
const WrapperTypeInfo V8TestActiveDOMObject::wrapperTypeInfo = { V8TestActiveDOMObject::GetTemplate, V8TestActiveDOMObject::derefObject, 0, 0, 0, V8TestActiveDOMObject::installPerContextEnabledMethods, 0, WrapperTypeObjectPrototype };

namespace TestActiveDOMObjectV8Internal {

template <typename T> void V8_USE(T) { }

static void excitingAttrAttributeGetter(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(info.Holder());
    v8SetReturnValueInt(info, imp->excitingAttr());
}

static void excitingAttrAttributeGetterCallback(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestActiveDOMObjectV8Internal::excitingAttrAttributeGetter(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

bool indexedSecurityCheck(v8::Local<v8::Object> host, uint32_t index, v8::AccessType type, v8::Local<v8::Value>)
{
    TestActiveDOMObject* imp =  V8TestActiveDOMObject::toNative(host);
    return BindingSecurity::shouldAllowAccessToFrame(imp->frame(), DoNotReportSecurityError);
}

bool namedSecurityCheck(v8::Local<v8::Object> host, v8::Local<v8::Value> key, v8::AccessType type, v8::Local<v8::Value>)
{
    TestActiveDOMObject* imp =  V8TestActiveDOMObject::toNative(host);
    return BindingSecurity::shouldAllowAccessToFrame(imp->frame(), DoNotReportSecurityError);
}

static void excitingFunctionMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (UNLIKELY(info.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("excitingFunction", "TestActiveDOMObject", ExceptionMessages::notEnoughArguments(1, info.Length())), info.GetIsolate());
        return;
    }
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(info.Holder());
    ExceptionState exceptionState(info.Holder(), info.GetIsolate());
    if (!BindingSecurity::shouldAllowAccessToFrame(imp->frame(), exceptionState)) {
        exceptionState.throwIfNeeded();
        return;
    }
    V8TRYCATCH_VOID(Node*, nextChild, V8Node::hasInstance(info[0], info.GetIsolate(), worldType(info.GetIsolate())) ? V8Node::toNative(v8::Handle<v8::Object>::Cast(info[0])) : 0);
    imp->excitingFunction(nextChild);
}

static void excitingFunctionMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestActiveDOMObjectV8Internal::excitingFunctionMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void postMessageMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (UNLIKELY(info.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("postMessage", "TestActiveDOMObject", ExceptionMessages::notEnoughArguments(1, info.Length())), info.GetIsolate());
        return;
    }
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, message, info[0]);
    imp->postMessage(message);
}

static void postMessageMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestActiveDOMObjectV8Internal::postMessageMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void postMessageAttributeGetter(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    // This is only for getting a unique pointer which we can pass to privateTemplate.
    static int privateTemplateUniqueKey;
    WrapperWorldType currentWorldType = worldType(info.GetIsolate());
    V8PerIsolateData* data = V8PerIsolateData::from(info.GetIsolate());
    v8::Handle<v8::FunctionTemplate> privateTemplate = data->privateTemplate(currentWorldType, &privateTemplateUniqueKey, TestActiveDOMObjectV8Internal::postMessageMethodCallback, v8Undefined(), v8::Signature::New(V8PerIsolateData::from(info.GetIsolate())->rawTemplate(&V8TestActiveDOMObject::wrapperTypeInfo, currentWorldType)), 1);

    v8::Handle<v8::Object> holder = info.This()->FindInstanceInPrototypeChain(V8TestActiveDOMObject::GetTemplate(info.GetIsolate(), currentWorldType));
    if (holder.IsEmpty()) {
        // can only reach here by 'object.__proto__.func', and it should passed
        // domain security check already
        v8SetReturnValue(info, privateTemplate->GetFunction());
        return;
    }
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(holder);
    if (!BindingSecurity::shouldAllowAccessToFrame(imp->frame(), DoNotReportSecurityError)) {
        static int sharedTemplateUniqueKey;
        v8::Handle<v8::FunctionTemplate> sharedTemplate = data->privateTemplate(currentWorldType, &sharedTemplateUniqueKey, TestActiveDOMObjectV8Internal::postMessageMethodCallback, v8Undefined(), v8::Signature::New(V8PerIsolateData::from(info.GetIsolate())->rawTemplate(&V8TestActiveDOMObject::wrapperTypeInfo, currentWorldType)), 1);
        v8SetReturnValue(info, sharedTemplate->GetFunction());
        return;
    }

    v8::Local<v8::Value> hiddenValue = info.This()->GetHiddenValue(v8::String::NewSymbol("postMessage"));
    if (!hiddenValue.IsEmpty()) {
        v8SetReturnValue(info, hiddenValue);
        return;
    }

    v8SetReturnValue(info, privateTemplate->GetFunction());
}

static void postMessageAttributeGetterCallback(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestActiveDOMObjectV8Internal::postMessageAttributeGetter(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void perWorldBindingsMethodWithDoNotCheckSecurityMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (UNLIKELY(info.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("perWorldBindingsMethodWithDoNotCheckSecurity", "TestActiveDOMObject", ExceptionMessages::notEnoughArguments(1, info.Length())), info.GetIsolate());
        return;
    }
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, url, info[0]);
    imp->perWorldBindingsMethodWithDoNotCheckSecurity(url);
}

static void perWorldBindingsMethodWithDoNotCheckSecurityMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    V8PerContextData* contextData = V8PerContextData::from(info.GetIsolate()->GetCurrentContext());
    if (contextData && contextData->activityLogger()) {
        Vector<v8::Handle<v8::Value> > loggerArgs = toVectorOfArguments(info);
        contextData->activityLogger()->log("TestActiveDOMObject.perWorldBindingsMethodWithDoNotCheckSecurity", info.Length(), loggerArgs.data(), "Method");
    }
    TestActiveDOMObjectV8Internal::perWorldBindingsMethodWithDoNotCheckSecurityMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void perWorldBindingsMethodWithDoNotCheckSecurityAttributeGetter(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    // This is only for getting a unique pointer which we can pass to privateTemplate.
    static int privateTemplateUniqueKey;
    WrapperWorldType currentWorldType = worldType(info.GetIsolate());
    V8PerIsolateData* data = V8PerIsolateData::from(info.GetIsolate());
    v8::Handle<v8::FunctionTemplate> privateTemplate = data->privateTemplate(currentWorldType, &privateTemplateUniqueKey, TestActiveDOMObjectV8Internal::perWorldBindingsMethodWithDoNotCheckSecurityMethodCallback, v8Undefined(), v8::Signature::New(V8PerIsolateData::from(info.GetIsolate())->rawTemplate(&V8TestActiveDOMObject::wrapperTypeInfo, currentWorldType)), 1);

    v8::Handle<v8::Object> holder = info.This()->FindInstanceInPrototypeChain(V8TestActiveDOMObject::GetTemplate(info.GetIsolate(), currentWorldType));
    if (holder.IsEmpty()) {
        // can only reach here by 'object.__proto__.func', and it should passed
        // domain security check already
        v8SetReturnValue(info, privateTemplate->GetFunction());
        return;
    }
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(holder);
    if (!BindingSecurity::shouldAllowAccessToFrame(imp->frame(), DoNotReportSecurityError)) {
        static int sharedTemplateUniqueKey;
        v8::Handle<v8::FunctionTemplate> sharedTemplate = data->privateTemplate(currentWorldType, &sharedTemplateUniqueKey, TestActiveDOMObjectV8Internal::perWorldBindingsMethodWithDoNotCheckSecurityMethodCallback, v8Undefined(), v8::Signature::New(V8PerIsolateData::from(info.GetIsolate())->rawTemplate(&V8TestActiveDOMObject::wrapperTypeInfo, currentWorldType)), 1);
        v8SetReturnValue(info, sharedTemplate->GetFunction());
        return;
    }

    v8::Local<v8::Value> hiddenValue = info.This()->GetHiddenValue(v8::String::NewSymbol("perWorldBindingsMethodWithDoNotCheckSecurity"));
    if (!hiddenValue.IsEmpty()) {
        v8SetReturnValue(info, hiddenValue);
        return;
    }

    v8SetReturnValue(info, privateTemplate->GetFunction());
}

static void perWorldBindingsMethodWithDoNotCheckSecurityAttributeGetterCallback(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestActiveDOMObjectV8Internal::perWorldBindingsMethodWithDoNotCheckSecurityAttributeGetter(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void perWorldBindingsMethodWithDoNotCheckSecurityMethodForMainWorld(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (UNLIKELY(info.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("perWorldBindingsMethodWithDoNotCheckSecurity", "TestActiveDOMObject", ExceptionMessages::notEnoughArguments(1, info.Length())), info.GetIsolate());
        return;
    }
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, url, info[0]);
    imp->perWorldBindingsMethodWithDoNotCheckSecurity(url);
}

static void perWorldBindingsMethodWithDoNotCheckSecurityMethodCallbackForMainWorld(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestActiveDOMObjectV8Internal::perWorldBindingsMethodWithDoNotCheckSecurityMethodForMainWorld(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void perWorldBindingsMethodWithDoNotCheckSecurityAttributeGetterForMainWorld(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    // This is only for getting a unique pointer which we can pass to privateTemplate.
    static int privateTemplateUniqueKey;
    WrapperWorldType currentWorldType = worldType(info.GetIsolate());
    V8PerIsolateData* data = V8PerIsolateData::from(info.GetIsolate());
    v8::Handle<v8::FunctionTemplate> privateTemplate = data->privateTemplate(currentWorldType, &privateTemplateUniqueKey, TestActiveDOMObjectV8Internal::perWorldBindingsMethodWithDoNotCheckSecurityMethodCallbackForMainWorld, v8Undefined(), v8::Signature::New(V8PerIsolateData::from(info.GetIsolate())->rawTemplate(&V8TestActiveDOMObject::wrapperTypeInfo, currentWorldType)), 1);

    v8::Handle<v8::Object> holder = info.This()->FindInstanceInPrototypeChain(V8TestActiveDOMObject::GetTemplate(info.GetIsolate(), currentWorldType));
    if (holder.IsEmpty()) {
        // can only reach here by 'object.__proto__.func', and it should passed
        // domain security check already
        v8SetReturnValue(info, privateTemplate->GetFunction());
        return;
    }
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(holder);
    if (!BindingSecurity::shouldAllowAccessToFrame(imp->frame(), DoNotReportSecurityError)) {
        static int sharedTemplateUniqueKey;
        v8::Handle<v8::FunctionTemplate> sharedTemplate = data->privateTemplate(currentWorldType, &sharedTemplateUniqueKey, TestActiveDOMObjectV8Internal::perWorldBindingsMethodWithDoNotCheckSecurityMethodCallbackForMainWorld, v8Undefined(), v8::Signature::New(V8PerIsolateData::from(info.GetIsolate())->rawTemplate(&V8TestActiveDOMObject::wrapperTypeInfo, currentWorldType)), 1);
        v8SetReturnValue(info, sharedTemplate->GetFunction());
        return;
    }

    v8::Local<v8::Value> hiddenValue = info.This()->GetHiddenValue(v8::String::NewSymbol("perWorldBindingsMethodWithDoNotCheckSecurity"));
    if (!hiddenValue.IsEmpty()) {
        v8SetReturnValue(info, hiddenValue);
        return;
    }

    v8SetReturnValue(info, privateTemplate->GetFunction());
}

static void perWorldBindingsMethodWithDoNotCheckSecurityAttributeGetterCallbackForMainWorld(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestActiveDOMObjectV8Internal::perWorldBindingsMethodWithDoNotCheckSecurityAttributeGetterForMainWorld(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void TestActiveDOMObjectDomainSafeFunctionSetter(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    v8::Handle<v8::Object> holder = info.This()->FindInstanceInPrototypeChain(V8TestActiveDOMObject::GetTemplate(info.GetIsolate(), worldType(info.GetIsolate())));
    if (holder.IsEmpty())
        return;
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(holder);
    ExceptionState exceptionState(info.Holder(), info.GetIsolate());
    if (!BindingSecurity::shouldAllowAccessToFrame(imp->frame(), exceptionState)) {
        exceptionState.throwIfNeeded();
        return;
    }

    info.This()->SetHiddenValue(name, jsValue);
}

} // namespace TestActiveDOMObjectV8Internal

static const V8DOMConfiguration::AttributeConfiguration V8TestActiveDOMObjectAttributes[] = {
    {"excitingAttr", TestActiveDOMObjectV8Internal::excitingAttrAttributeGetterCallback, 0, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
};

static v8::Handle<v8::FunctionTemplate> ConfigureV8TestActiveDOMObjectTemplate(v8::Handle<v8::FunctionTemplate> functionTemplate, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    functionTemplate->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(functionTemplate, "TestActiveDOMObject", v8::Local<v8::FunctionTemplate>(), V8TestActiveDOMObject::internalFieldCount,
        V8TestActiveDOMObjectAttributes, WTF_ARRAY_LENGTH(V8TestActiveDOMObjectAttributes),
        0, 0,
        isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature);
    v8::Local<v8::ObjectTemplate> instanceTemplate = functionTemplate->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> prototypeTemplate = functionTemplate->PrototypeTemplate();
    UNUSED_PARAM(instanceTemplate);
    UNUSED_PARAM(prototypeTemplate);
    instanceTemplate->SetAccessCheckCallbacks(TestActiveDOMObjectV8Internal::namedSecurityCheck, TestActiveDOMObjectV8Internal::indexedSecurityCheck, v8::External::New(const_cast<WrapperTypeInfo*>(&V8TestActiveDOMObject::wrapperTypeInfo)));

    // Custom Signature 'excitingFunction'
    const int excitingFunctionArgc = 1;
    v8::Handle<v8::FunctionTemplate> excitingFunctionArgv[excitingFunctionArgc] = { V8PerIsolateData::from(isolate)->rawTemplate(&V8Node::wrapperTypeInfo, currentWorldType) };
    v8::Handle<v8::Signature> excitingFunctionSignature = v8::Signature::New(functionTemplate, excitingFunctionArgc, excitingFunctionArgv);
    prototypeTemplate->Set(v8::String::NewSymbol("excitingFunction"), v8::FunctionTemplate::New(TestActiveDOMObjectV8Internal::excitingFunctionMethodCallback, v8Undefined(), excitingFunctionSignature, 1));

    // Function 'postMessage' (Extended Attributes: 'DoNotCheckSecurity')
    prototypeTemplate->SetAccessor(v8::String::NewSymbol("postMessage"), TestActiveDOMObjectV8Internal::postMessageAttributeGetterCallback, TestActiveDOMObjectV8Internal::TestActiveDOMObjectDomainSafeFunctionSetter, v8Undefined(), v8::ALL_CAN_READ, static_cast<v8::PropertyAttribute>(v8::DontDelete));

    // Function 'perWorldBindingsMethodWithDoNotCheckSecurity' (Extended Attributes: 'DoNotCheckSecurity PerWorldBindings ActivityLogging')
    if (currentWorldType == MainWorld) {
        prototypeTemplate->SetAccessor(v8::String::NewSymbol("perWorldBindingsMethodWithDoNotCheckSecurity"), TestActiveDOMObjectV8Internal::perWorldBindingsMethodWithDoNotCheckSecurityAttributeGetterCallbackForMainWorld, TestActiveDOMObjectV8Internal::TestActiveDOMObjectDomainSafeFunctionSetter, v8Undefined(), v8::ALL_CAN_READ, static_cast<v8::PropertyAttribute>(v8::DontDelete));
    } else {
        prototypeTemplate->SetAccessor(v8::String::NewSymbol("perWorldBindingsMethodWithDoNotCheckSecurity"), TestActiveDOMObjectV8Internal::perWorldBindingsMethodWithDoNotCheckSecurityAttributeGetterCallback, TestActiveDOMObjectV8Internal::TestActiveDOMObjectDomainSafeFunctionSetter, v8Undefined(), v8::ALL_CAN_READ, static_cast<v8::PropertyAttribute>(v8::DontDelete));
    }

    // Custom toString template
    functionTemplate->Set(v8::String::NewSymbol("toString"), V8PerIsolateData::current()->toStringTemplate());
    return functionTemplate;
}

v8::Handle<v8::FunctionTemplate> V8TestActiveDOMObject::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&wrapperTypeInfo);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::HandleScope handleScope(isolate);
    v8::Handle<v8::FunctionTemplate> templ =
        ConfigureV8TestActiveDOMObjectTemplate(data->rawTemplate(&wrapperTypeInfo, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&wrapperTypeInfo, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Close(templ);
}

bool V8TestActiveDOMObject::hasInstance(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, currentWorldType);
}

bool V8TestActiveDOMObject::hasInstanceInAnyWorld(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, WorkerWorld);
}

v8::Handle<v8::Object> V8TestActiveDOMObject::createWrapper(PassRefPtr<TestActiveDOMObject> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8TestActiveDOMObject>(impl.get(), isolate));
    if (ScriptWrappable::wrapperCanBeStoredInObject(impl.get())) {
        const WrapperTypeInfo* actualInfo = ScriptWrappable::getTypeInfoFromObject(impl.get());
        // Might be a XXXConstructor::wrapperTypeInfo instead of an XXX::wrapperTypeInfo. These will both have
        // the same object de-ref functions, though, so use that as the basis of the check.
        RELEASE_ASSERT_WITH_SECURITY_IMPLICATION(actualInfo->derefObjectFunction == wrapperTypeInfo.derefObjectFunction);
    }

    v8::Handle<v8::Object> wrapper = V8DOMWrapper::createWrapper(creationContext, &wrapperTypeInfo, toInternalPointer(impl.get()), isolate);
    if (UNLIKELY(wrapper.IsEmpty()))
        return wrapper;

    installPerContextEnabledProperties(wrapper, impl.get(), isolate);
    V8DOMWrapper::associateObjectWithWrapper<V8TestActiveDOMObject>(impl, &wrapperTypeInfo, wrapper, isolate, WrapperConfiguration::Independent);
    return wrapper;
}

void V8TestActiveDOMObject::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

} // namespace WebCore
