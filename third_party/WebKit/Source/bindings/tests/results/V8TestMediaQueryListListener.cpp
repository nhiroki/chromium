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
#include "V8TestMediaQueryListListener.h"

#include "RuntimeEnabledFeatures.h"
#include "bindings/v8/ScriptController.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "core/css/MediaQueryListListener.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "core/page/Frame.h"
#include "wtf/UnusedParam.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestMediaQueryListListener* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestMediaQueryListListener::info);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(WebCore::TestMediaQueryListListener* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
WrapperTypeInfo V8TestMediaQueryListListener::info = { V8TestMediaQueryListListener::GetTemplate, V8TestMediaQueryListListener::derefObject, 0, 0, 0, V8TestMediaQueryListListener::installPerContextPrototypeProperties, 0, WrapperTypeObjectPrototype };

namespace TestMediaQueryListListenerV8Internal {

template <typename T> void V8_USE(T) { }

static void methodMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() < 1) {
        throwNotEnoughArgumentsError(args.GetIsolate());
        return;
    }
    TestMediaQueryListListener* imp = V8TestMediaQueryListListener::toNative(args.Holder());
    V8TRYCATCH_VOID(RefPtr<MediaQueryListListener>, listener, MediaQueryListListener::create(args[0]));
    imp->method(listener);

    v8SetReturnValue(args, v8Undefined());
    return;
}

static void methodMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TestMediaQueryListListenerV8Internal::methodMethod(args);
}

} // namespace TestMediaQueryListListenerV8Internal

static const V8DOMConfiguration::BatchedMethod V8TestMediaQueryListListenerMethods[] = {
    {"method", TestMediaQueryListListenerV8Internal::methodMethodCallback, 0, 1},
};

static v8::Handle<v8::FunctionTemplate> ConfigureV8TestMediaQueryListListenerTemplate(v8::Handle<v8::FunctionTemplate> desc, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    desc->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::configureTemplate(desc, "TestMediaQueryListListener", v8::Local<v8::FunctionTemplate>(), V8TestMediaQueryListListener::internalFieldCount,
        0, 0,
        V8TestMediaQueryListListenerMethods, WTF_ARRAY_LENGTH(V8TestMediaQueryListListenerMethods), isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature); // In some cases, it will not be used.
    v8::Local<v8::ObjectTemplate> instance = desc->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> proto = desc->PrototypeTemplate();
    UNUSED_PARAM(instance); // In some cases, it will not be used.
    UNUSED_PARAM(proto); // In some cases, it will not be used.

    // Custom toString template
    desc->Set(v8::String::NewSymbol("toString"), V8PerIsolateData::current()->toStringTemplate());
    return desc;
}

v8::Handle<v8::FunctionTemplate> V8TestMediaQueryListListener::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&info);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    v8::HandleScope handleScope(isolate);
    v8::Handle<v8::FunctionTemplate> templ =
        ConfigureV8TestMediaQueryListListenerTemplate(data->rawTemplate(&info, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&info, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Close(templ);
}

bool V8TestMediaQueryListListener::HasInstance(v8::Handle<v8::Value> value, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, currentWorldType);
}

bool V8TestMediaQueryListListener::HasInstanceInAnyWorld(v8::Handle<v8::Value> value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, WorkerWorld);
}


v8::Handle<v8::Object> V8TestMediaQueryListListener::createWrapper(PassRefPtr<TestMediaQueryListListener> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
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
void V8TestMediaQueryListListener::derefObject(void* object)
{
    static_cast<TestMediaQueryListListener*>(object)->deref();
}

} // namespace WebCore
