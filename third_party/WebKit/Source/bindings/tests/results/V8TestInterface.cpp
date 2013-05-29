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
#if ENABLE(Condition1) || ENABLE(Condition2)
#include "V8TestInterface.h"

#include "RuntimeEnabledFeatures.h"
#include "V8Node.h"
#include "V8NodeList.h"
#include "V8TestObject.h"
#include "bindings/bindings/tests/idls/TestPartialInterface.h"
#include "bindings/v8/ScriptController.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8Collection.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "bindings/v8/V8ObjectConstructor.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "core/page/Frame.h"
#include "wtf/GetPtr.h"
#include "wtf/RefCounted.h"
#include "wtf/RefPtr.h"
#include "wtf/UnusedParam.h"

namespace WebCore {

#if defined(OS_WIN)
// In ScriptWrappable, the use of extern function prototypes inside templated static methods has an issue on windows.
// These prototypes do not pick up the surrounding namespace, so drop out of WebCore as a workaround.
} // namespace WebCore
using WebCore::ScriptWrappable;
using WebCore::V8TestInterface;
using WebCore::TestInterface;
#endif
void initializeScriptWrappableForInterface(TestInterface* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestInterface::info);
    else
        ASSERT_NOT_REACHED();
}
#if defined(OS_WIN)
namespace WebCore {
#endif
WrapperTypeInfo V8TestInterface::info = { V8TestInterface::GetTemplate, V8TestInterface::derefObject, V8TestInterface::toActiveDOMObject, 0, 0, V8TestInterface::installPerContextPrototypeProperties, 0, WrapperTypeObjectPrototype };

namespace TestInterfaceV8Internal {

template <typename T> void V8_USE(T) { }

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalStaticReadOnlyAttrAttrGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return v8Integer(TestPartialInterface::supplementalStaticReadOnlyAttr(), info.GetIsolate());
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalStaticReadOnlyAttrAttrGetterCallback(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return TestInterfaceV8Internal::supplementalStaticReadOnlyAttrAttrGetter(name, info);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalStaticAttrAttrGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return v8String(TestPartialInterface::supplementalStaticAttr(), info.GetIsolate(), ReturnUnsafeHandle);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalStaticAttrAttrGetterCallback(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return TestInterfaceV8Internal::supplementalStaticAttrAttrGetter(name, info);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void supplementalStaticAttrAttrSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, v, value);
    TestPartialInterface::setSupplementalStaticAttr(v);
    return;
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void supplementalStaticAttrAttrSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    TestInterfaceV8Internal::supplementalStaticAttrAttrSetter(name, value, info);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalStr1AttrGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    TestInterface* imp = V8TestInterface::toNative(info.Holder());
    return v8String(TestPartialInterface::supplementalStr1(imp), info.GetIsolate(), ReturnUnsafeHandle);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalStr1AttrGetterCallback(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return TestInterfaceV8Internal::supplementalStr1AttrGetter(name, info);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalStr2AttrGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    TestInterface* imp = V8TestInterface::toNative(info.Holder());
    return v8String(TestPartialInterface::supplementalStr2(imp), info.GetIsolate(), ReturnUnsafeHandle);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalStr2AttrGetterCallback(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return TestInterfaceV8Internal::supplementalStr2AttrGetter(name, info);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void supplementalStr2AttrSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    TestInterface* imp = V8TestInterface::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, v, value);
    TestPartialInterface::setSupplementalStr2(imp, v);
    return;
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void supplementalStr2AttrSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    TestInterfaceV8Internal::supplementalStr2AttrSetter(name, value, info);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalStr3AttrGetterCallback(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return V8TestInterface::supplementalStr3AttrGetterCustom(name, info);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void supplementalStr3AttrSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    V8TestInterface::supplementalStr3AttrSetterCustom(name, value, info);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalNodeAttrGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    TestInterface* imp = V8TestInterface::toNative(info.Holder());
    return toV8Fast(TestPartialInterface::supplementalNode(imp), info, imp);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalNodeAttrGetterCallback(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return TestInterfaceV8Internal::supplementalNodeAttrGetter(name, info);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void supplementalNodeAttrSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    TestInterface* imp = V8TestInterface::toNative(info.Holder());
    V8TRYCATCH_VOID(Node*, v, V8Node::HasInstance(value, info.GetIsolate(), worldType(info.GetIsolate())) ? V8Node::toNative(v8::Handle<v8::Object>::Cast(value)) : 0);
    TestPartialInterface::setSupplementalNode(imp, WTF::getPtr(v));
    return;
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void supplementalNodeAttrSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    TestInterfaceV8Internal::supplementalNodeAttrSetter(name, value, info);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> Node13AttrGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    TestInterface* imp = V8TestInterface::toNative(info.Holder());
    return toV8Fast(TestPartialInterface::node13(imp), info, imp);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> Node13AttrGetterCallback(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return TestInterfaceV8Internal::Node13AttrGetter(name, info);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void Node13AttrSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    TestInterface* imp = V8TestInterface::toNative(info.Holder());
    V8TRYCATCH_VOID(Node*, v, V8Node::HasInstance(value, info.GetIsolate(), worldType(info.GetIsolate())) ? V8Node::toNative(v8::Handle<v8::Object>::Cast(value)) : 0);
    TestPartialInterface::setNode13(imp, WTF::getPtr(v));
    return;
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void Node13AttrSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    TestInterfaceV8Internal::Node13AttrSetter(name, value, info);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> Node14AttrGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    TestInterface* imp = V8TestInterface::toNative(info.Holder());
    return toV8Fast(TestPartialInterface::node14(imp), info, imp);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> Node14AttrGetterCallback(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return TestInterfaceV8Internal::Node14AttrGetter(name, info);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void Node14AttrSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    TestInterface* imp = V8TestInterface::toNative(info.Holder());
    V8TRYCATCH_VOID(Node*, v, V8Node::HasInstance(value, info.GetIsolate(), worldType(info.GetIsolate())) ? V8Node::toNative(v8::Handle<v8::Object>::Cast(value)) : 0);
    TestPartialInterface::setNode14(imp, WTF::getPtr(v));
    return;
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void Node14AttrSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    TestInterfaceV8Internal::Node14AttrSetter(name, value, info);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> Node15AttrGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    TestInterface* imp = V8TestInterface::toNative(info.Holder());
    return toV8Fast(TestPartialInterface::node15(imp), info, imp);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> Node15AttrGetterCallback(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    return TestInterfaceV8Internal::Node15AttrGetter(name, info);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void Node15AttrSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    TestInterface* imp = V8TestInterface::toNative(info.Holder());
    V8TRYCATCH_VOID(Node*, v, V8Node::HasInstance(value, info.GetIsolate(), worldType(info.GetIsolate())) ? V8Node::toNative(v8::Handle<v8::Object>::Cast(value)) : 0);
    TestPartialInterface::setNode15(imp, WTF::getPtr(v));
    return;
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static void Node15AttrSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    TestInterfaceV8Internal::Node15AttrSetter(name, value, info);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalMethod1Method(const v8::Arguments& args)
{
    TestInterface* imp = V8TestInterface::toNative(args.Holder());
    TestPartialInterface::supplementalMethod1(imp);
    return v8Undefined();
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalMethod1MethodCallback(const v8::Arguments& args)
{
    return TestInterfaceV8Internal::supplementalMethod1Method(args);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalMethod2Method(const v8::Arguments& args)
{
    if (args.Length() < 2)
        return throwNotEnoughArgumentsError(args.GetIsolate());
    TestInterface* imp = V8TestInterface::toNative(args.Holder());
    ExceptionCode ec = 0;
    V8TRYCATCH_FOR_V8STRINGRESOURCE(V8StringResource<>, strArg, args[0]);
    V8TRYCATCH(TestObj*, objArg, V8TestObject::HasInstance(args[1], args.GetIsolate(), worldType(args.GetIsolate())) ? V8TestObject::toNative(v8::Handle<v8::Object>::Cast(args[1])) : 0);
    ScriptExecutionContext* scriptContext = getScriptExecutionContext();
    RefPtr<TestObj> result = TestPartialInterface::supplementalMethod2(scriptContext, imp, strArg, objArg, ec);
    if (UNLIKELY(ec))
        return setDOMException(ec, args.GetIsolate());
    return toV8(result.release(), args.Holder(), args.GetIsolate());
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalMethod2MethodCallback(const v8::Arguments& args)
{
    return TestInterfaceV8Internal::supplementalMethod2Method(args);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalMethod3MethodCallback(const v8::Arguments& args)
{
    return V8TestInterface::supplementalMethod3MethodCustom(args);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalMethod4Method(const v8::Arguments& args)
{
    TestPartialInterface::supplementalMethod4();
    return v8Undefined();
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)

static v8::Handle<v8::Value> supplementalMethod4MethodCallback(const v8::Arguments& args)
{
    return TestInterfaceV8Internal::supplementalMethod4Method(args);
}

#endif // ENABLE(Condition11) || ENABLE(Condition12)

static void constructor(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() < 1) {
        throwNotEnoughArgumentsError(args.GetIsolate());
        return;
    }
    ExceptionCode ec = 0;
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, str1, args[0]);
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, str2, args[1]);

    ScriptExecutionContext* context = getScriptExecutionContext();

    RefPtr<TestInterface> impl = TestInterface::create(context, str1, str2, ec);
    v8::Handle<v8::Object> wrapper = args.Holder();
    if (ec) {
        setDOMException(ec, args.GetIsolate());
        return;
    }

    V8DOMWrapper::associateObjectWithWrapper(impl.release(), &V8TestInterface::info, wrapper, args.GetIsolate(), WrapperConfiguration::Dependent);
    args.GetReturnValue().Set(wrapper);
}

} // namespace TestInterfaceV8Internal

static const V8DOMConfiguration::BatchedAttribute V8TestInterfaceAttrs[] = {
#if ENABLE(Condition11) || ENABLE(Condition12)
    // Attribute 'supplementalStaticReadOnlyAttr' (Type: 'attribute' ExtAttr: 'Conditional ImplementedBy')
    {"supplementalStaticReadOnlyAttr", TestInterfaceV8Internal::supplementalStaticReadOnlyAttrAttrGetterCallback, 0, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
#endif // ENABLE(Condition11) || ENABLE(Condition12)
#if ENABLE(Condition11) || ENABLE(Condition12)
    // Attribute 'supplementalStaticAttr' (Type: 'attribute' ExtAttr: 'Conditional ImplementedBy')
    {"supplementalStaticAttr", TestInterfaceV8Internal::supplementalStaticAttrAttrGetterCallback, TestInterfaceV8Internal::supplementalStaticAttrAttrSetterCallback, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
#endif // ENABLE(Condition11) || ENABLE(Condition12)
#if ENABLE(Condition11) || ENABLE(Condition12)
    // Attribute 'supplementalStr1' (Type: 'attribute' ExtAttr: 'Conditional ImplementedBy')
    {"supplementalStr1", TestInterfaceV8Internal::supplementalStr1AttrGetterCallback, 0, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
#endif // ENABLE(Condition11) || ENABLE(Condition12)
#if ENABLE(Condition11) || ENABLE(Condition12)
    // Attribute 'supplementalStr2' (Type: 'attribute' ExtAttr: 'Conditional ImplementedBy')
    {"supplementalStr2", TestInterfaceV8Internal::supplementalStr2AttrGetterCallback, TestInterfaceV8Internal::supplementalStr2AttrSetterCallback, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
#endif // ENABLE(Condition11) || ENABLE(Condition12)
#if ENABLE(Condition11) || ENABLE(Condition12)
    // Attribute 'supplementalStr3' (Type: 'attribute' ExtAttr: 'CustomSetter CustomGetter Conditional ImplementedBy')
    {"supplementalStr3", TestInterfaceV8Internal::supplementalStr3AttrGetterCallback, TestInterfaceV8Internal::supplementalStr3AttrSetterCallback, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
#endif // ENABLE(Condition11) || ENABLE(Condition12)
#if ENABLE(Condition11) || ENABLE(Condition12)
    // Attribute 'supplementalNode' (Type: 'attribute' ExtAttr: 'Conditional ImplementedBy')
    {"supplementalNode", TestInterfaceV8Internal::supplementalNodeAttrGetterCallback, TestInterfaceV8Internal::supplementalNodeAttrSetterCallback, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
#endif // ENABLE(Condition11) || ENABLE(Condition12)
};

static const V8DOMConfiguration::BatchedMethod V8TestInterfaceMethods[] = {
#if ENABLE(Condition11) || ENABLE(Condition12)
    {"supplementalMethod1", TestInterfaceV8Internal::supplementalMethod1MethodCallback, 0, 0},
#endif
#if ENABLE(Condition11) || ENABLE(Condition12)
    {"supplementalMethod3", TestInterfaceV8Internal::supplementalMethod3MethodCallback, 0, 0},
#endif
};

static const V8DOMConfiguration::BatchedConstant V8TestInterfaceConsts[] = {
#if ENABLE(Condition11) || ENABLE(Condition12)
    {"SUPPLEMENTALCONSTANT1", 1},
#endif
#if ENABLE(Condition11) || ENABLE(Condition12)
    {"SUPPLEMENTALCONSTANT2", 2},
#endif
};


#if ENABLE(Condition11) || ENABLE(Condition12)
COMPILE_ASSERT(1 == TestPartialInterface::SUPPLEMENTALCONSTANT1, TestInterfaceEnumSUPPLEMENTALCONSTANT1IsWrongUseDoNotCheckConstants);
#endif
#if ENABLE(Condition11) || ENABLE(Condition12)
COMPILE_ASSERT(2 == TestPartialInterface::CONST_IMPL, TestInterfaceEnumCONST_IMPLIsWrongUseDoNotCheckConstants);
#endif

void V8TestInterface::constructorCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (!args.IsConstructCall()) {
        throwTypeError("DOM object constructor cannot be called as a function.", args.GetIsolate());
        return;
    }

    if (ConstructorMode::current() == ConstructorMode::WrapExistingObject) {
        args.GetReturnValue().Set(args.Holder());
        return;
    }

    TestInterfaceV8Internal::constructor(args);
}

v8::Handle<v8::Value> V8TestInterface::namedPropertyGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    if (!info.Holder()->GetRealNamedPropertyInPrototypeChain(name).IsEmpty())
        return v8Undefined();
    if (info.Holder()->HasRealNamedCallbackProperty(name))
        return v8Undefined();

    ASSERT(V8DOMWrapper::maybeDOMWrapper(info.Holder()));
    TestInterface* collection = toNative(info.Holder());
    AtomicString propertyName = toWebCoreAtomicString(name);
    bool element0Enabled = false;
    RefPtr<Node> element0;
    bool element1Enabled = false;
    RefPtr<NodeList> element1;
    collection->getItem(propertyName, element0Enabled, element0, element1Enabled, element1);
    if (element0Enabled)
        return toV8Fast(element0.release(), info, collection);
    if (element1Enabled)
        return toV8Fast(element1.release(), info, collection);
    return v8Undefined();
}

static v8::Handle<v8::FunctionTemplate> ConfigureV8TestInterfaceTemplate(v8::Handle<v8::FunctionTemplate> desc, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    desc->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::configureTemplate(desc, "TestInterface", v8::Local<v8::FunctionTemplate>(), V8TestInterface::internalFieldCount,
        V8TestInterfaceAttrs, WTF_ARRAY_LENGTH(V8TestInterfaceAttrs),
        V8TestInterfaceMethods, WTF_ARRAY_LENGTH(V8TestInterfaceMethods), isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature); // In some cases, it will not be used.
    desc->SetCallHandler(V8TestInterface::constructorCallback);
    desc->SetLength(1);
    v8::Local<v8::ObjectTemplate> instance = desc->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> proto = desc->PrototypeTemplate();
    UNUSED_PARAM(instance); // In some cases, it will not be used.
    UNUSED_PARAM(proto); // In some cases, it will not be used.

#if ENABLE(Condition11) || ENABLE(Condition12)
    if (RuntimeEnabledFeatures::condition13Enabled()) {
        static const V8DOMConfiguration::BatchedAttribute attrData =\
        // Attribute 'Node13' (Type: 'attribute' ExtAttr: 'EnabledAtRuntime Conditional ImplementedBy')
        {"Node13", TestInterfaceV8Internal::Node13AttrGetterCallback, TestInterfaceV8Internal::Node13AttrSetterCallback, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */};
        V8DOMConfiguration::configureAttribute(instance, proto, attrData, isolate, currentWorldType);
    }

#endif // ENABLE(Condition11) || ENABLE(Condition12)
    desc->InstanceTemplate()->SetNamedPropertyHandler(V8TestInterface::namedPropertyGetter, V8TestInterface::namedPropertySetter, 0, 0, 0);

    // Custom Signature 'supplementalMethod2'
    const int supplementalMethod2Argc = 2;
    v8::Handle<v8::FunctionTemplate> supplementalMethod2Argv[supplementalMethod2Argc] = { v8::Handle<v8::FunctionTemplate>(), V8PerIsolateData::from(isolate)->rawTemplate(&V8TestObject::info, currentWorldType) };
    v8::Handle<v8::Signature> supplementalMethod2Signature = v8::Signature::New(desc, supplementalMethod2Argc, supplementalMethod2Argv);
#if ENABLE(Condition11) || ENABLE(Condition12)
    proto->Set(v8::String::NewSymbol("supplementalMethod2"), v8::FunctionTemplate::New(TestInterfaceV8Internal::supplementalMethod2MethodCallback, v8Undefined(), supplementalMethod2Signature, 2));
#endif // ENABLE(Condition11) || ENABLE(Condition12)
#if ENABLE(Condition11) || ENABLE(Condition12)
    desc->Set(v8::String::NewSymbol("supplementalMethod4"), v8::FunctionTemplate::New(TestInterfaceV8Internal::supplementalMethod4MethodCallback, v8Undefined(), v8::Local<v8::Signature>(), 0));
#endif // ENABLE(Condition11) || ENABLE(Condition12)
    V8DOMConfiguration::batchConfigureConstants(desc, proto, V8TestInterfaceConsts, WTF_ARRAY_LENGTH(V8TestInterfaceConsts), isolate);

    // Custom toString template
    desc->Set(v8::String::NewSymbol("toString"), V8PerIsolateData::current()->toStringTemplate());
    return desc;
}

v8::Handle<v8::FunctionTemplate> V8TestInterface::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&info);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    v8::HandleScope handleScope(isolate);
    v8::Handle<v8::FunctionTemplate> templ =
        ConfigureV8TestInterfaceTemplate(data->rawTemplate(&info, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&info, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Close(templ);
}

bool V8TestInterface::HasInstance(v8::Handle<v8::Value> value, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, currentWorldType);
}

bool V8TestInterface::HasInstanceInAnyWorld(v8::Handle<v8::Value> value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, WorkerWorld);
}

void V8TestInterface::installPerContextProperties(v8::Handle<v8::Object> instance, TestInterface* impl, v8::Isolate* isolate)
{
    v8::Local<v8::Object> proto = v8::Local<v8::Object>::Cast(instance->GetPrototype());

#if ENABLE(Condition11) || ENABLE(Condition12)
    if (ContextFeatures::condition14Enabled(impl->document())) {
        static const V8DOMConfiguration::BatchedAttribute attrData =\
        // Attribute 'Node14' (Type: 'attribute' ExtAttr: 'EnabledPerContext Conditional ImplementedBy')
        {"Node14", TestInterfaceV8Internal::Node14AttrGetterCallback, TestInterfaceV8Internal::Node14AttrSetterCallback, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */};
        V8DOMConfiguration::configureAttribute(instance, proto, attrData, isolate);
    }
#endif // ENABLE(Condition11) || ENABLE(Condition12)

#if ENABLE(Condition11) || ENABLE(Condition12)
    if (ContextFeatures::condition16Enabled(impl->document()) && RuntimeEnabledFeatures::condition15Enabled()) {
        static const V8DOMConfiguration::BatchedAttribute attrData =\
        // Attribute 'Node15' (Type: 'attribute' ExtAttr: 'EnabledPerContext EnabledAtRuntime Conditional ImplementedBy')
        {"Node15", TestInterfaceV8Internal::Node15AttrGetterCallback, TestInterfaceV8Internal::Node15AttrSetterCallback, 0, 0, 0 /* no data */, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */};
        V8DOMConfiguration::configureAttribute(instance, proto, attrData, isolate);
    }
#endif // ENABLE(Condition11) || ENABLE(Condition12)
}

ActiveDOMObject* V8TestInterface::toActiveDOMObject(v8::Handle<v8::Object> object)
{
    return toNative(object);
}


v8::Handle<v8::Object> V8TestInterface::createWrapper(PassRefPtr<TestInterface> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl.get());
    ASSERT(DOMDataStore::getWrapper(impl.get(), isolate).IsEmpty());

    v8::Handle<v8::Object> wrapper = V8DOMWrapper::createWrapper(creationContext, &info, impl.get(), isolate);
    if (UNLIKELY(wrapper.IsEmpty()))
        return wrapper;

    installPerContextProperties(wrapper, impl.get(), isolate);
    V8DOMWrapper::associateObjectWithWrapper(impl, &info, wrapper, isolate, WrapperConfiguration::Dependent);
    return wrapper;
}
void V8TestInterface::derefObject(void* object)
{
    static_cast<TestInterface*>(object)->deref();
}

} // namespace WebCore

#endif // ENABLE(Condition1) || ENABLE(Condition2)
