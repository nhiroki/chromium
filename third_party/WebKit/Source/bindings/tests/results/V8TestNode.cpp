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
#include "V8TestNode.h"

#include "RuntimeEnabledFeatures.h"
#include "V8Node.h"
#include "bindings/v8/ScriptController.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "bindings/v8/V8ObjectConstructor.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "core/page/Frame.h"
#include "core/platform/chromium/TraceEvent.h"
#include "wtf/UnusedParam.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestNode* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestNode::info);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(WebCore::TestNode* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
WrapperTypeInfo V8TestNode::info = { V8TestNode::GetTemplate, V8TestNode::derefObject, 0, V8TestNode::toEventTarget, 0, V8TestNode::installPerContextPrototypeProperties, &V8Node::info, WrapperTypeObjectPrototype };

namespace TestNodeV8Internal {

template <typename T> void V8_USE(T) { }

static void constructor(const v8::FunctionCallbackInfo<v8::Value>& args)
{

    RefPtr<TestNode> impl = TestNode::create();
    v8::Handle<v8::Object> wrapper = args.Holder();

    V8DOMWrapper::associateObjectWithWrapper(impl.release(), &V8TestNode::info, wrapper, args.GetIsolate(), WrapperConfiguration::Dependent);
    args.GetReturnValue().Set(wrapper);
}

} // namespace TestNodeV8Internal

void V8TestNode::constructorCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "DOMConstructor");
    if (!args.IsConstructCall()) {
        throwTypeError("DOM object constructor cannot be called as a function.", args.GetIsolate());
        return;
    }

    if (ConstructorMode::current() == ConstructorMode::WrapExistingObject) {
        args.GetReturnValue().Set(args.Holder());
        return;
    }

    TestNodeV8Internal::constructor(args);
}

static v8::Handle<v8::FunctionTemplate> ConfigureV8TestNodeTemplate(v8::Handle<v8::FunctionTemplate> desc, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    desc->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::configureTemplate(desc, "TestNode", V8Node::GetTemplate(isolate, currentWorldType), V8TestNode::internalFieldCount,
        0, 0,
        0, 0, isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature); // In some cases, it will not be used.
    desc->SetCallHandler(V8TestNode::constructorCallback);
    desc->SetLength(0);

    // Custom toString template
    desc->Set(v8::String::NewSymbol("toString"), V8PerIsolateData::current()->toStringTemplate());
    return desc;
}

v8::Handle<v8::FunctionTemplate> V8TestNode::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&info);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::HandleScope handleScope(isolate);
    v8::Handle<v8::FunctionTemplate> templ =
        ConfigureV8TestNodeTemplate(data->rawTemplate(&info, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&info, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Close(templ);
}

bool V8TestNode::HasInstance(v8::Handle<v8::Value> value, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, currentWorldType);
}

bool V8TestNode::HasInstanceInAnyWorld(v8::Handle<v8::Value> value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, WorkerWorld);
}

EventTarget* V8TestNode::toEventTarget(v8::Handle<v8::Object> object)
{
    return toNative(object);
}


v8::Handle<v8::Object> V8TestNode::createWrapper(PassRefPtr<TestNode> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl.get());
    ASSERT(DOMDataStore::getWrapper(impl.get(), isolate).IsEmpty());
    ASSERT(static_cast<void*>(static_cast<Node*>(impl.get())) == static_cast<void*>(impl.get()));

    v8::Handle<v8::Object> wrapper = V8DOMWrapper::createWrapper(creationContext, &info, impl.get(), isolate);
    if (UNLIKELY(wrapper.IsEmpty()))
        return wrapper;
    installPerContextProperties(wrapper, impl.get(), isolate);
    V8DOMWrapper::associateObjectWithWrapper(impl, &info, wrapper, isolate, WrapperConfiguration::Dependent);
    return wrapper;
}
void V8TestNode::derefObject(void* object)
{
    static_cast<TestNode*>(object)->deref();
}

} // namespace WebCore
