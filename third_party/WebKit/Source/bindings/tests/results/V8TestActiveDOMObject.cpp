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
#include "bindings/v8/ScriptController.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "core/page/Frame.h"
#include "wtf/UnusedParam.h"

namespace WebCore {

#if defined(OS_WIN)
// In ScriptWrappable, the use of extern function prototypes inside templated static methods has an issue on windows.
// These prototypes do not pick up the surrounding namespace, so drop out of WebCore as a workaround.
} // namespace WebCore
using WebCore::ScriptWrappable;
using WebCore::V8TestActiveDOMObject;
using WebCore::TestActiveDOMObject;
#endif
void initializeScriptWrappableForInterface(TestActiveDOMObject* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestActiveDOMObject::info);
    else
        ASSERT_NOT_REACHED();
}
#if defined(OS_WIN)
namespace WebCore {
#endif
WrapperTypeInfo V8TestActiveDOMObject::info = { V8TestActiveDOMObject::GetTemplate, V8TestActiveDOMObject::derefObject, 0, 0, 0, V8TestActiveDOMObject::installPerContextPrototypeProperties, 0, WrapperTypeObjectPrototype };

namespace TestActiveDOMObjectV8Internal {

template <typename T> void V8_USE(T) { }

static v8::Handle<v8::Value> excitingAttrAttrGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(info.Holder());
    return v8Integer(imp->excitingAttr(), info.GetIsolate());
}

static v8::Handle<v8::Value> excitingAttrAttrGetterCallback(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return TestActiveDOMObjectV8Internal::excitingAttrAttrGetter(name, info);
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
    if (args.Length() < 1) {
        throwNotEnoughArgumentsError(args.GetIsolate());
        return;
    }
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(args.Holder());
    if (!BindingSecurity::shouldAllowAccessToFrame(imp->frame()))
        return;
    V8TRYCATCH_VOID(Node*, nextChild, V8Node::HasInstance(args[0], args.GetIsolate(), worldType(args.GetIsolate())) ? V8Node::toNative(v8::Handle<v8::Object>::Cast(args[0])) : 0);
    imp->excitingFunction(nextChild);

    v8SetReturnValue(args, v8Undefined());
    return;
}

static void excitingFunctionMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TestActiveDOMObjectV8Internal::excitingFunctionMethod(args);
}

static void postMessageMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() < 1) {
        throwNotEnoughArgumentsError(args.GetIsolate());
        return;
    }
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(args.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, message, args[0]);
    imp->postMessage(message);

    v8SetReturnValue(args, v8Undefined());
    return;
}

static void postMessageMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TestActiveDOMObjectV8Internal::postMessageMethod(args);
}

static v8::Handle<v8::Value> postMessageAttrGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
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
        return privateTemplate->GetFunction();
    }
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(holder);
    if (!BindingSecurity::shouldAllowAccessToFrame(imp->frame(), DoNotReportSecurityError)) {
        static const char* sharedTemplateUniqueKey = "postMessageSharedTemplate";
        v8::Handle<v8::FunctionTemplate> sharedTemplate = data->privateTemplate(currentWorldType, &sharedTemplateUniqueKey, TestActiveDOMObjectV8Internal::postMessageMethodCallback, v8Undefined(), v8::Signature::New(V8PerIsolateData::from(info.GetIsolate())->rawTemplate(&V8TestActiveDOMObject::info, currentWorldType)), 1);
        return sharedTemplate->GetFunction();
    }

    v8::Local<v8::Value> hiddenValue = info.This()->GetHiddenValue(name);
    if (!hiddenValue.IsEmpty())
        return hiddenValue;

    return privateTemplate->GetFunction();
}

static v8::Handle<v8::Value> postMessageAttrGetterCallback(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return TestActiveDOMObjectV8Internal::postMessageAttrGetter(name, info);
}

static void TestActiveDOMObjectDomainSafeFunctionSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    v8::Handle<v8::Object> holder = info.This()->FindInstanceInPrototypeChain(V8TestActiveDOMObject::GetTemplate(info.GetIsolate(), worldType(info.GetIsolate())));
    if (holder.IsEmpty())
        return;
    TestActiveDOMObject* imp = V8TestActiveDOMObject::toNative(holder);
    if (!BindingSecurity::shouldAllowAccessToFrame(imp->frame()))
        return;

    info.This()->SetHiddenValue(name, value);
}

} // namespace TestActiveDOMObjectV8Internal

static const V8DOMConfiguration::BatchedAttribute V8TestActiveDOMObjectAttrs[] = {
    // Attribute 'excitingAttr' (Type: 'attribute' ExtAttr: '')
    {"excitingAttr", TestActiveDOMObjectV8Internal::excitingAttrAttrGetterCallback, 0, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
};

static v8::Handle<v8::FunctionTemplate> ConfigureV8TestActiveDOMObjectTemplate(v8::Handle<v8::FunctionTemplate> desc, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    desc->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::configureTemplate(desc, "TestActiveDOMObject", v8::Local<v8::FunctionTemplate>(), V8TestActiveDOMObject::internalFieldCount,
        V8TestActiveDOMObjectAttrs, WTF_ARRAY_LENGTH(V8TestActiveDOMObjectAttrs),
        0, 0, isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature); // In some cases, it will not be used.
    v8::Local<v8::ObjectTemplate> instance = desc->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> proto = desc->PrototypeTemplate();
    UNUSED_PARAM(instance); // In some cases, it will not be used.
    UNUSED_PARAM(proto); // In some cases, it will not be used.
    instance->SetAccessCheckCallbacks(TestActiveDOMObjectV8Internal::namedSecurityCheck, TestActiveDOMObjectV8Internal::indexedSecurityCheck, v8::External::New(&V8TestActiveDOMObject::info));

    // Custom Signature 'excitingFunction'
    const int excitingFunctionArgc = 1;
    v8::Handle<v8::FunctionTemplate> excitingFunctionArgv[excitingFunctionArgc] = { V8PerIsolateData::from(isolate)->rawTemplate(&V8Node::info, currentWorldType) };
    v8::Handle<v8::Signature> excitingFunctionSignature = v8::Signature::New(desc, excitingFunctionArgc, excitingFunctionArgv);
    proto->Set(v8::String::NewSymbol("excitingFunction"), v8::FunctionTemplate::New(TestActiveDOMObjectV8Internal::excitingFunctionMethodCallback, v8Undefined(), excitingFunctionSignature, 1));

    // Function 'postMessage' (ExtAttr: 'DoNotCheckSecurity')
    proto->SetAccessor(v8::String::NewSymbol("postMessage"), TestActiveDOMObjectV8Internal::postMessageAttrGetterCallback, TestActiveDOMObjectV8Internal::TestActiveDOMObjectDomainSafeFunctionSetter, v8Undefined(), v8::ALL_CAN_READ, static_cast<v8::PropertyAttribute>(v8::DontDelete));

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

    v8::HandleScope handleScope(isolate);
    v8::Handle<v8::FunctionTemplate> templ =
        ConfigureV8TestActiveDOMObjectTemplate(data->rawTemplate(&info, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&info, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Close(templ);
}

bool V8TestActiveDOMObject::HasInstance(v8::Handle<v8::Value> value, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, currentWorldType);
}

bool V8TestActiveDOMObject::HasInstanceInAnyWorld(v8::Handle<v8::Value> value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, WorkerWorld);
}


v8::Handle<v8::Object> V8TestActiveDOMObject::createWrapper(PassRefPtr<TestActiveDOMObject> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl.get());
    ASSERT(DOMDataStore::getWrapper(impl.get(), isolate).IsEmpty());

    v8::Handle<v8::Object> wrapper = V8DOMWrapper::createWrapper(creationContext, &info, impl.get(), isolate);
    if (UNLIKELY(wrapper.IsEmpty()))
        return wrapper;

    installPerContextProperties(wrapper, impl.get(), isolate);
    V8DOMWrapper::associateObjectWithWrapper(impl, &info, wrapper, isolate, WrapperConfiguration::Independent);
    return wrapper;
}
void V8TestActiveDOMObject::derefObject(void* object)
{
    static_cast<TestActiveDOMObject*>(object)->deref();
}

} // namespace WebCore
