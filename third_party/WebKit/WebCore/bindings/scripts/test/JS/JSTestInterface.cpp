/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "JSTestInterface.h"

#include "TestInterface.h"
#include <wtf/GetPtr.h>

using namespace JSC;

namespace WebCore {

ASSERT_CLASS_FITS_IN_CELL(JSTestInterface);

/* Hash table */
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const HashTableValue JSTestInterfaceTableValues[2] =
{
    { "constructor", DontEnum | ReadOnly, (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestInterfaceConstructor), (intptr_t)0 THUNK_GENERATOR(0) },
    { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
static JSC_CONST_HASHTABLE HashTable JSTestInterfaceTable = { 2, 1, JSTestInterfaceTableValues, 0 };
/* Hash table for constructor */
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const HashTableValue JSTestInterfaceConstructorTableValues[1] =
{
    { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
static JSC_CONST_HASHTABLE HashTable JSTestInterfaceConstructorTable = { 1, 0, JSTestInterfaceConstructorTableValues, 0 };
class JSTestInterfaceConstructor : public DOMConstructorObject {
public:
    JSTestInterfaceConstructor(ExecState* exec, JSDOMGlobalObject* globalObject)
        : DOMConstructorObject(JSTestInterfaceConstructor::createStructure(globalObject->objectPrototype()), globalObject)
    {
        putDirect(exec->propertyNames().prototype, JSTestInterfacePrototype::self(exec, globalObject), None);
    }
    virtual bool getOwnPropertySlot(ExecState*, const Identifier&, PropertySlot&);
    virtual bool getOwnPropertyDescriptor(ExecState*, const Identifier&, PropertyDescriptor&);
    virtual const ClassInfo* classInfo() const { return &s_info; }
    static const ClassInfo s_info;

    static PassRefPtr<Structure> createStructure(JSValue proto) 
    { 
        return Structure::create(proto, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount); 
    }
    
protected:
    static const unsigned StructureFlags = OverridesGetOwnPropertySlot | ImplementsHasInstance | DOMConstructorObject::StructureFlags;
    static JSObject* constructTestInterface(ExecState* exec, JSObject* constructor, const ArgList&)
    {
        ScriptExecutionContext* context = static_cast<JSTestInterfaceConstructor*>(constructor)->scriptExecutionContext();
        if (!context)
            return throwError(exec, ReferenceError);
        return asObject(toJS(exec, static_cast<JSTestInterfaceConstructor*>(constructor)->globalObject(), TestInterface::create(context)));
    }
    virtual ConstructType getConstructData(ConstructData& constructData)
    {
        constructData.native.function = constructTestInterface;
        return ConstructTypeHost;
    }
};

const ClassInfo JSTestInterfaceConstructor::s_info = { "TestInterfaceConstructor", 0, &JSTestInterfaceConstructorTable, 0 };

bool JSTestInterfaceConstructor::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<JSTestInterfaceConstructor, DOMObject>(exec, &JSTestInterfaceConstructorTable, this, propertyName, slot);
}

bool JSTestInterfaceConstructor::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticValueDescriptor<JSTestInterfaceConstructor, DOMObject>(exec, &JSTestInterfaceConstructorTable, this, propertyName, descriptor);
}

/* Hash table for prototype */
#if ENABLE(JIT)
#define THUNK_GENERATOR(generator) , generator
#else
#define THUNK_GENERATOR(generator)
#endif

static const HashTableValue JSTestInterfacePrototypeTableValues[1] =
{
    { 0, 0, 0, 0 THUNK_GENERATOR(0) }
};

#undef THUNK_GENERATOR
static JSC_CONST_HASHTABLE HashTable JSTestInterfacePrototypeTable = { 1, 0, JSTestInterfacePrototypeTableValues, 0 };
const ClassInfo JSTestInterfacePrototype::s_info = { "TestInterfacePrototype", 0, &JSTestInterfacePrototypeTable, 0 };

JSObject* JSTestInterfacePrototype::self(ExecState* exec, JSGlobalObject* globalObject)
{
    return getDOMPrototype<JSTestInterface>(exec, globalObject);
}

const ClassInfo JSTestInterface::s_info = { "TestInterface", 0, &JSTestInterfaceTable, 0 };

JSTestInterface::JSTestInterface(NonNullPassRefPtr<Structure> structure, JSDOMGlobalObject* globalObject, PassRefPtr<TestInterface> impl)
    : DOMObjectWithGlobalPointer(structure, globalObject)
    , m_impl(impl)
{
}

JSTestInterface::~JSTestInterface()
{
    forgetDOMObject(this, impl());
}

JSObject* JSTestInterface::createPrototype(ExecState* exec, JSGlobalObject* globalObject)
{
    return new (exec) JSTestInterfacePrototype(JSTestInterfacePrototype::createStructure(globalObject->objectPrototype()));
}

bool JSTestInterface::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return getStaticValueSlot<JSTestInterface, Base>(exec, &JSTestInterfaceTable, this, propertyName, slot);
}

bool JSTestInterface::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticValueDescriptor<JSTestInterface, Base>(exec, &JSTestInterfaceTable, this, propertyName, descriptor);
}

JSValue jsTestInterfaceConstructor(ExecState* exec, JSValue slotBase, const Identifier&)
{
    JSTestInterface* domObject = static_cast<JSTestInterface*>(asObject(slotBase));
    return JSTestInterface::getConstructor(exec, domObject->globalObject());
}
JSValue JSTestInterface::getConstructor(ExecState* exec, JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSTestInterfaceConstructor>(exec, static_cast<JSDOMGlobalObject*>(globalObject));
}

JSC::JSValue toJS(JSC::ExecState* exec, JSDOMGlobalObject* globalObject, TestInterface* object)
{
    return getDOMObjectWrapper<JSTestInterface>(exec, globalObject, object);
}
TestInterface* toTestInterface(JSC::JSValue value)
{
    return value.inherits(&JSTestInterface::s_info) ? static_cast<JSTestInterface*>(asObject(value))->impl() : 0;
}

}
