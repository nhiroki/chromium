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
#include "V8TestEventTarget.h"

#include "RuntimeEnabledFeatures.h"
#include "V8Event.h"
#include "V8Node.h"
#include "bindings/v8/ScriptController.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "bindings/v8/V8EventListenerList.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "core/page/Frame.h"
#include "core/platform/chromium/TraceEvent.h"
#include "wtf/UnusedParam.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestEventTarget* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestEventTarget::info);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(WebCore::TestEventTarget* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
WrapperTypeInfo V8TestEventTarget::info = { V8TestEventTarget::GetTemplate, V8TestEventTarget::derefObject, 0, V8TestEventTarget::toEventTarget, 0, V8TestEventTarget::installPerContextPrototypeProperties, 0, WrapperTypeObjectPrototype };

namespace TestEventTargetV8Internal {

template <typename T> void V8_USE(T) { }

static void itemMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() < 1) {
        throwNotEnoughArgumentsError(args.GetIsolate());
        return;
    }
    TestEventTarget* imp = V8TestEventTarget::toNative(args.Holder());
    ExceptionCode ec = 0;
    V8TRYCATCH_VOID(int, index, toUInt32(args[0]));
    if (UNLIKELY(index < 0)) {
        setDOMException(INDEX_SIZE_ERR, args.GetIsolate());
        return;
    }
    v8SetReturnValue(args, toV8(imp->item(index), args.Holder(), args.GetIsolate()));
    return;
}

static void itemMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink\0DOMMethod");
    TestEventTargetV8Internal::itemMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8\0Execution");
}

static void namedItemMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() < 1) {
        throwNotEnoughArgumentsError(args.GetIsolate());
        return;
    }
    TestEventTarget* imp = V8TestEventTarget::toNative(args.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, name, args[0]);
    v8SetReturnValue(args, toV8(imp->namedItem(name), args.Holder(), args.GetIsolate()));
    return;
}

static void namedItemMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink\0DOMMethod");
    TestEventTargetV8Internal::namedItemMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8\0Execution");
}

static void addEventListenerMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    RefPtr<EventListener> listener = V8EventListenerList::getEventListener(args[1], false, ListenerFindOrCreate);
    if (listener) {
        V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<WithNullCheck>, stringResource, args[0]);
        V8TestEventTarget::toNative(args.Holder())->addEventListener(stringResource, listener, args[2]->BooleanValue());
        createHiddenDependency(args.Holder(), args[1], V8TestEventTarget::eventListenerCacheIndex, args.GetIsolate());
    }
}

static void addEventListenerMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink\0DOMMethod");
    TestEventTargetV8Internal::addEventListenerMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8\0Execution");
}

static void removeEventListenerMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    RefPtr<EventListener> listener = V8EventListenerList::getEventListener(args[1], false, ListenerFindOnly);
    if (listener) {
        V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<WithNullCheck>, stringResource, args[0]);
        V8TestEventTarget::toNative(args.Holder())->removeEventListener(stringResource, listener.get(), args[2]->BooleanValue());
        removeHiddenDependency(args.Holder(), args[1], V8TestEventTarget::eventListenerCacheIndex, args.GetIsolate());
    }
}

static void removeEventListenerMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink\0DOMMethod");
    TestEventTargetV8Internal::removeEventListenerMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8\0Execution");
}

static void dispatchEventMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() < 1) {
        throwNotEnoughArgumentsError(args.GetIsolate());
        return;
    }
    TestEventTarget* imp = V8TestEventTarget::toNative(args.Holder());
    ExceptionCode ec = 0;
    V8TRYCATCH_VOID(Event*, evt, V8Event::HasInstance(args[0], args.GetIsolate(), worldType(args.GetIsolate())) ? V8Event::toNative(v8::Handle<v8::Object>::Cast(args[0])) : 0);
    bool result = imp->dispatchEvent(evt, ec);
    if (UNLIKELY(ec)) {
        setDOMException(ec, args.GetIsolate());
        return;
    }
    v8SetReturnValueBool(args, result);
    return;
}

static void dispatchEventMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink\0DOMMethod");
    TestEventTargetV8Internal::dispatchEventMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8\0Execution");
}

static void indexedPropertyGetter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    ASSERT(V8DOMWrapper::maybeDOMWrapper(info.Holder()));
    TestEventTarget* collection = V8TestEventTarget::toNative(info.Holder());
    RefPtr<Node> element = collection->item(index);
    if (!element)
        return;
    v8SetReturnValue(info, toV8Fast(element.release(), info, collection));
}

static void indexedPropertyGetterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink\0DOMIndexedProperty");
    TestEventTargetV8Internal::indexedPropertyGetter(index, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8\0Execution");
}

static void indexedPropertySetter(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestEventTarget* collection = V8TestEventTarget::toNative(info.Holder());
    V8TRYCATCH_VOID(Node*, propertyValue, V8Node::HasInstance(value, info.GetIsolate(), worldType(info.GetIsolate())) ? V8Node::toNative(v8::Handle<v8::Object>::Cast(value)) : 0);
    bool result = collection->anonymousIndexedSetter(index, propertyValue);
    if (!result)
        return;
    v8SetReturnValue(info, value);
}

static void indexedPropertySetterCallback(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink\0DOMIndexedProperty");
    TestEventTargetV8Internal::indexedPropertySetter(index, value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8\0Execution");
}

static void indexedPropertyDeleter(unsigned index, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
    TestEventTarget* collection = V8TestEventTarget::toNative(info.Holder());
    ExceptionCode ec = 0;
    bool result = collection->anonymousIndexedDeleter(index, ec);
    if (ec) {
        setDOMException(ec, info.GetIsolate());
        return;
    }
    return v8SetReturnValueBool(info, result);
}

static void indexedPropertyDeleterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink\0DOMIndexedProperty");
    TestEventTargetV8Internal::indexedPropertyDeleter(index, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8\0Execution");
}

static void namedPropertyGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    if (!info.Holder()->GetRealNamedPropertyInPrototypeChain(name).IsEmpty())
        return;
    if (info.Holder()->HasRealNamedCallbackProperty(name))
        return;
    if (info.Holder()->HasRealNamedProperty(name))
        return;

    ASSERT(V8DOMWrapper::maybeDOMWrapper(info.Holder()));
    TestEventTarget* collection = V8TestEventTarget::toNative(info.Holder());
    AtomicString propertyName = toWebCoreAtomicString(name);
    RefPtr<Node> element = collection->namedItem(propertyName);
    if (!element)
        return;
    v8SetReturnValue(info, toV8Fast(element.release(), info, collection));
}

static void namedPropertyGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink\0DOMNamedProperty");
    TestEventTargetV8Internal::namedPropertyGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8\0Execution");
}

static void namedPropertySetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    if (!info.Holder()->GetRealNamedPropertyInPrototypeChain(name).IsEmpty())
        return;
    if (info.Holder()->HasRealNamedCallbackProperty(name))
        return;
    if (info.Holder()->HasRealNamedProperty(name))
        return;
    TestEventTarget* collection = V8TestEventTarget::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, propertyName, name);
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, propertyValue, value);
    bool result;
    if (value->IsUndefined())
        result = collection->anonymousNamedSetterUndefined(propertyName);
    else
        result = collection->anonymousNamedSetter(propertyName, propertyValue);
    if (!result)
        return;
    v8SetReturnValue(info, value);
}

static void namedPropertySetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink\0DOMNamedProperty");
    TestEventTargetV8Internal::namedPropertySetter(name, value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8\0Execution");
}

static void namedPropertyDeleter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
    TestEventTarget* collection = V8TestEventTarget::toNative(info.Holder());
    AtomicString propertyName = toWebCoreAtomicString(name);
    bool result = collection->anonymousNamedDeleter(propertyName);
    return v8SetReturnValueBool(info, result);
}

static void namedPropertyDeleterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink\0DOMNamedProperty");
    TestEventTargetV8Internal::namedPropertyDeleter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8\0Execution");
}

static void namedPropertyEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
    ExceptionCode ec = 0;
    TestEventTarget* collection = V8TestEventTarget::toNative(info.Holder());
    Vector<String> names;
    collection->namedPropertyEnumerator(names, ec);
    if (ec) {
        setDOMException(ec, info.GetIsolate());
        return;
    }
    v8::Handle<v8::Array> v8names = v8::Array::New(names.size());
    for (size_t i = 0; i < names.size(); ++i)
        v8names->Set(v8::Integer::New(i, info.GetIsolate()), v8String(names[i], info.GetIsolate()));
    v8SetReturnValue(info, v8names);
}

static void namedPropertyQuery(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Integer>& info)
{
    TestEventTarget* collection = V8TestEventTarget::toNative(info.Holder());
    AtomicString propertyName = toWebCoreAtomicString(name);
    ExceptionCode ec = 0;
    bool result = collection->namedPropertyQuery(propertyName, ec);
    if (ec) {
        setDOMException(ec, info.GetIsolate());
        return;
    }
    if (!result)
        return;
    v8SetReturnValueInt(info, v8::None);
}

static void namedPropertyEnumeratorCallback(const v8::PropertyCallbackInfo<v8::Array>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink\0DOMNamedProperty");
    TestEventTargetV8Internal::namedPropertyEnumerator(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8\0Execution");
}

static void namedPropertyQueryCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Integer>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink\0DOMNamedProperty");
    TestEventTargetV8Internal::namedPropertyQuery(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8\0Execution");
}

} // namespace TestEventTargetV8Internal

static const V8DOMConfiguration::BatchedMethod V8TestEventTargetMethods[] = {
    {"item", TestEventTargetV8Internal::itemMethodCallback, 0, 1},
    {"namedItem", TestEventTargetV8Internal::namedItemMethodCallback, 0, 1},
    {"addEventListener", TestEventTargetV8Internal::addEventListenerMethodCallback, 0, 2},
    {"removeEventListener", TestEventTargetV8Internal::removeEventListenerMethodCallback, 0, 2},
};

static v8::Handle<v8::FunctionTemplate> ConfigureV8TestEventTargetTemplate(v8::Handle<v8::FunctionTemplate> desc, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    desc->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::configureTemplate(desc, "TestEventTarget", v8::Local<v8::FunctionTemplate>(), V8TestEventTarget::internalFieldCount,
        0, 0,
        V8TestEventTargetMethods, WTF_ARRAY_LENGTH(V8TestEventTargetMethods), isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature); // In some cases, it will not be used.
    v8::Local<v8::ObjectTemplate> instance = desc->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> proto = desc->PrototypeTemplate();
    UNUSED_PARAM(instance); // In some cases, it will not be used.
    UNUSED_PARAM(proto); // In some cases, it will not be used.
    desc->InstanceTemplate()->SetIndexedPropertyHandler(TestEventTargetV8Internal::indexedPropertyGetterCallback, TestEventTargetV8Internal::indexedPropertySetterCallback, 0, TestEventTargetV8Internal::indexedPropertyDeleterCallback, indexedPropertyEnumerator<TestEventTarget>);
    desc->InstanceTemplate()->SetNamedPropertyHandler(TestEventTargetV8Internal::namedPropertyGetterCallback, TestEventTargetV8Internal::namedPropertySetterCallback, TestEventTargetV8Internal::namedPropertyQueryCallback, TestEventTargetV8Internal::namedPropertyDeleterCallback, TestEventTargetV8Internal::namedPropertyEnumeratorCallback);
    desc->InstanceTemplate()->MarkAsUndetectable();

    // Custom Signature 'dispatchEvent'
    const int dispatchEventArgc = 1;
    v8::Handle<v8::FunctionTemplate> dispatchEventArgv[dispatchEventArgc] = { V8PerIsolateData::from(isolate)->rawTemplate(&V8Event::info, currentWorldType) };
    v8::Handle<v8::Signature> dispatchEventSignature = v8::Signature::New(desc, dispatchEventArgc, dispatchEventArgv);
    proto->Set(v8::String::NewSymbol("dispatchEvent"), v8::FunctionTemplate::New(TestEventTargetV8Internal::dispatchEventMethodCallback, v8Undefined(), dispatchEventSignature, 1));

    // Custom toString template
    desc->Set(v8::String::NewSymbol("toString"), V8PerIsolateData::current()->toStringTemplate());
    return desc;
}

v8::Handle<v8::FunctionTemplate> V8TestEventTarget::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&info);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::HandleScope handleScope(isolate);
    v8::Handle<v8::FunctionTemplate> templ =
        ConfigureV8TestEventTargetTemplate(data->rawTemplate(&info, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&info, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Close(templ);
}

bool V8TestEventTarget::HasInstance(v8::Handle<v8::Value> value, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, currentWorldType);
}

bool V8TestEventTarget::HasInstanceInAnyWorld(v8::Handle<v8::Value> value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, WorkerWorld);
}

EventTarget* V8TestEventTarget::toEventTarget(v8::Handle<v8::Object> object)
{
    return toNative(object);
}


v8::Handle<v8::Object> V8TestEventTarget::createWrapper(PassRefPtr<TestEventTarget> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
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
void V8TestEventTarget::derefObject(void* object)
{
    static_cast<TestEventTarget*>(object)->deref();
}

} // namespace WebCore
