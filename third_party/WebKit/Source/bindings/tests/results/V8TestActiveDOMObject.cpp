/*
    This file is part of the Blink open source project.
    This file has been auto-generated by CodeGeneratorV8.pm. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "config.h"
#include "V8TestActiveDOMObject.h"

#include "RuntimeEnabledFeatures.h"
#include "V8Node.h"
#include "bindings/v8/BindingSecurity.h"
#include "bindings/v8/ExceptionMessages.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/ScriptController.h"
#include "bindings/v8/V8Binding.h"
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
        ScriptWrappable::setTypeInfoInObject(object, &V8TestActiveDOMObject::info);
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
WrapperTypeInfo V8TestActiveDOMObject::info = { V8TestActiveDOMObject::GetTemplate, V8TestActiveDOMObject::derefObject, 0, 0, 0, V8TestActiveDOMObject::installPerContextEnabledPrototypeProperties, 0, WrapperTypeObjectPrototype };

namespace TestActiveDOMObjectV8Internal {

template <typename T> void V8_USE(T) { }

static void excitingAttrAttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(info.Holder());
    v8SetReturnValueInt(info, imp->excitingAttr());
}

static void excitingAttrAttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestActiveDOMObjectV8Internal::excitingAttrAttributeGetter(name, info);
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

static void excitingFunctionMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (UNLIKELY(args.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("excitingFunction", "TestActiveDOMObject", ExceptionMessages::notEnoughArguments(1, args.Length())), args.GetIsolate());
        return;
    }
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(args.Holder());
    ExceptionState es(args.GetIsolate());
    if (!BindingSecurity::shouldAllowAccessToFrame(imp->frame(), es)) {
        es.throwIfNeeded();
        return;
    }
    V8TRYCATCH_VOID(Node*, nextChild, V8Node::HasInstance(args[0], args.GetIsolate(), worldType(args.GetIsolate())) ? V8Node::toNative(v8::Handle<v8::Object>::Cast(args[0])) : 0);
    imp->excitingFunction(nextChild);

    return;
}

static void excitingFunctionMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestActiveDOMObjectV8Internal::excitingFunctionMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void postMessageMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (UNLIKELY(args.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("postMessage", "TestActiveDOMObject", ExceptionMessages::notEnoughArguments(1, args.Length())), args.GetIsolate());
        return;
    }
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(args.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, message, args[0]);
    imp->postMessage(message);

    return;
}

static void postMessageMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestActiveDOMObjectV8Internal::postMessageMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void postMessageAttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    // This is only for getting a unique pointer which we can pass to privateTemplate.
    static const char* privateTemplateUniqueKey = "postMessagePrivateTemplate";
    WrapperWorldType currentWorldType = worldType(info.GetIsolate());
    V8PerIsolateData* data = V8PerIsolateData::from(info.GetIsolate());
    v8::Handle<v8::FunctionTemplate> privateTemplate = data->privateTemplate(currentWorldType, &privateTemplateUniqueKey, TestActiveDOMObjectV8Internal::postMessageMethodCallback, v8Undefined(), v8::Signature::New(V8PerIsolateData::from(info.GetIsolate())->rawTemplate(&V8TestActiveDOMObject::info, currentWorldType)), 1);

    v8::Handle<v8::Object> holder = info.This()->FindInstanceInPrototypeChain(V8TestActiveDOMObject::GetTemplate(info.GetIsolate(), currentWorldType));
    if (holder.IsEmpty()) {
        // can only reach here by 'object.__proto__.func', and it should passed
        // domain security check already
        v8SetReturnValue(info, privateTemplate->GetFunction());
        return;
    }
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(holder);
    if (!BindingSecurity::shouldAllowAccessToFrame(imp->frame(), DoNotReportSecurityError)) {
        static const char* sharedTemplateUniqueKey = "postMessageSharedTemplate";
        v8::Handle<v8::FunctionTemplate> sharedTemplate = data->privateTemplate(currentWorldType, &sharedTemplateUniqueKey, TestActiveDOMObjectV8Internal::postMessageMethodCallback, v8Undefined(), v8::Signature::New(V8PerIsolateData::from(info.GetIsolate())->rawTemplate(&V8TestActiveDOMObject::info, currentWorldType)), 1);
        v8SetReturnValue(info, sharedTemplate->GetFunction());
        return;
    }

    v8::Local<v8::Value> hiddenValue = info.This()->GetHiddenValue(name);
    if (!hiddenValue.IsEmpty()) {
        v8SetReturnValue(info, hiddenValue);
        return;
    }

    v8SetReturnValue(info, privateTemplate->GetFunction());
}

static void postMessageAttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestActiveDOMObjectV8Internal::postMessageAttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void TestActiveDOMObjectDomainSafeFunctionSetter(v8::Local<v8::String> name, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    v8::Handle<v8::Object> holder = info.This()->FindInstanceInPrototypeChain(V8TestActiveDOMObject::GetTemplate(info.GetIsolate(), worldType(info.GetIsolate())));
    if (holder.IsEmpty())
        return;
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(holder);
    ExceptionState es(info.GetIsolate());
    if (!BindingSecurity::shouldAllowAccessToFrame(imp->frame(), es)) {
        es.throwIfNeeded();
        return;
    }

    info.This()->SetHiddenValue(name, jsValue);
}

} // namespace TestActiveDOMObjectV8Internal

static const V8DOMConfiguration::AttributeConfiguration V8TestActiveDOMObjectAttributes[] = {
    {"excitingAttr", TestActiveDOMObjectV8Internal::excitingAttrAttributeGetterCallback, 0, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
};

static v8::Handle<v8::FunctionTemplate> ConfigureV8TestActiveDOMObjectTemplate(v8::Handle<v8::FunctionTemplate> desc, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    desc->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(desc, "TestActiveDOMObject", v8::Local<v8::FunctionTemplate>(), V8TestActiveDOMObject::internalFieldCount,
        V8TestActiveDOMObjectAttributes, WTF_ARRAY_LENGTH(V8TestActiveDOMObjectAttributes),
        0, 0, isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature);
    v8::Local<v8::ObjectTemplate> instance = desc->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> proto = desc->PrototypeTemplate();
    UNUSED_PARAM(instance);
    UNUSED_PARAM(proto);
    instance->SetAccessCheckCallbacks(TestActiveDOMObjectV8Internal::namedSecurityCheck, TestActiveDOMObjectV8Internal::indexedSecurityCheck, v8::External::New(&V8TestActiveDOMObject::info));

    // Custom Signature 'excitingFunction'
    const int excitingFunctionArgc = 1;
    v8::Handle<v8::FunctionTemplate> excitingFunctionArgv[excitingFunctionArgc] = { V8PerIsolateData::from(isolate)->rawTemplate(&V8Node::info, currentWorldType) };
    v8::Handle<v8::Signature> excitingFunctionSignature = v8::Signature::New(desc, excitingFunctionArgc, excitingFunctionArgv);
    proto->Set(v8::String::NewSymbol("excitingFunction"), v8::FunctionTemplate::New(TestActiveDOMObjectV8Internal::excitingFunctionMethodCallback, v8Undefined(), excitingFunctionSignature, 1));

    // Function 'postMessage' (Extended Attributes: 'DoNotCheckSecurity')
    proto->SetAccessor(v8::String::NewSymbol("postMessage"), TestActiveDOMObjectV8Internal::postMessageAttributeGetterCallback, TestActiveDOMObjectV8Internal::TestActiveDOMObjectDomainSafeFunctionSetter, v8Undefined(), v8::ALL_CAN_READ, static_cast<v8::PropertyAttribute>(v8::DontDelete));

    // Custom toString template
    desc->Set(v8::String::NewSymbol("toString"), V8PerIsolateData::current()->toStringTemplate());
    return desc;
}

v8::Handle<v8::FunctionTemplate> V8TestActiveDOMObject::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&info);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::HandleScope handleScope(isolate);
    v8::Handle<v8::FunctionTemplate> templ =
        ConfigureV8TestActiveDOMObjectTemplate(data->rawTemplate(&info, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&info, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Close(templ);
}

bool V8TestActiveDOMObject::HasInstance(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, jsValue, currentWorldType);
}

bool V8TestActiveDOMObject::HasInstanceInAnyWorld(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, jsValue, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, jsValue, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, jsValue, WorkerWorld);
}

v8::Handle<v8::Object> V8TestActiveDOMObject::createWrapper(PassRefPtr<TestActiveDOMObject> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8TestActiveDOMObject>(impl.get(), isolate));
    if (ScriptWrappable::wrapperCanBeStoredInObject(impl.get())) {
        const WrapperTypeInfo* actualInfo = ScriptWrappable::getTypeInfoFromObject(impl.get());
        // Might be a XXXConstructor::info instead of an XXX::info. These will both have
        // the same object de-ref functions, though, so use that as the basis of the check.
        RELEASE_ASSERT_WITH_SECURITY_IMPLICATION(actualInfo->derefObjectFunction == info.derefObjectFunction);
    }

    v8::Handle<v8::Object> wrapper = V8DOMWrapper::createWrapper(creationContext, &info, toInternalPointer(impl.get()), isolate);
    if (UNLIKELY(wrapper.IsEmpty()))
        return wrapper;

    installPerContextEnabledProperties(wrapper, impl.get(), isolate);
    V8DOMWrapper::associateObjectWithWrapper<V8TestActiveDOMObject>(impl, &info, wrapper, isolate, WrapperConfiguration::Independent);
    return wrapper;
}

void V8TestActiveDOMObject::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

} // namespace WebCore
