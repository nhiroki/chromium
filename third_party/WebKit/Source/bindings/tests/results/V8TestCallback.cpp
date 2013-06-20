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
#include "V8TestCallback.h"

#include "V8DOMStringList.h"
#include "V8TestObject.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8Callback.h"
#include "core/dom/ScriptExecutionContext.h"
#include "wtf/Assertions.h"
#include "wtf/GetPtr.h"
#include "wtf/RefCounted.h"
#include "wtf/RefPtr.h"
namespace WebCore {

V8TestCallback::V8TestCallback(v8::Handle<v8::Object> callback, ScriptExecutionContext* context)
    : ActiveDOMCallback(context)
    , m_callback(callback)
    , m_world(DOMWrapperWorld::current())
{
}

V8TestCallback::~V8TestCallback()
{
}

// Functions

bool V8TestCallback::callbackWithNoParam()
{
    if (!canInvokeCallback())
        return true;

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope handleScope(isolate);

    v8::Handle<v8::Context> v8Context = toV8Context(scriptExecutionContext(), m_world.get());
    if (v8Context.IsEmpty())
        return true;

    v8::Context::Scope scope(v8Context);


    v8::Handle<v8::Value> *argv = 0;

    bool callbackReturnValue = false;
    return !invokeCallback(m_callback.newLocal(isolate), 0, argv, callbackReturnValue, scriptExecutionContext());
}

bool V8TestCallback::callbackWithTestObjectParam(TestObj* class1Param)
{
    if (!canInvokeCallback())
        return true;

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope handleScope(isolate);

    v8::Handle<v8::Context> v8Context = toV8Context(scriptExecutionContext(), m_world.get());
    if (v8Context.IsEmpty())
        return true;

    v8::Context::Scope scope(v8Context);

    v8::Handle<v8::Value> class1ParamHandle = toV8(class1Param, v8::Handle<v8::Object>(), isolate);
    if (class1ParamHandle.IsEmpty()) {
        if (!isScriptControllerTerminating())
            CRASH();
        return true;
    }

    v8::Handle<v8::Value> argv[] = {
        class1ParamHandle
    };

    bool callbackReturnValue = false;
    return !invokeCallback(m_callback.newLocal(isolate), 1, argv, callbackReturnValue, scriptExecutionContext());
}

bool V8TestCallback::callbackWithTestObjectParam(TestObj* class2Param, const String& strArg)
{
    if (!canInvokeCallback())
        return true;

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope handleScope(isolate);

    v8::Handle<v8::Context> v8Context = toV8Context(scriptExecutionContext(), m_world.get());
    if (v8Context.IsEmpty())
        return true;

    v8::Context::Scope scope(v8Context);

    v8::Handle<v8::Value> class2ParamHandle = toV8(class2Param, v8::Handle<v8::Object>(), isolate);
    if (class2ParamHandle.IsEmpty()) {
        if (!isScriptControllerTerminating())
            CRASH();
        return true;
    }
    v8::Handle<v8::Value> strArgHandle = v8String(strArg, isolate);
    if (strArgHandle.IsEmpty()) {
        if (!isScriptControllerTerminating())
            CRASH();
        return true;
    }

    v8::Handle<v8::Value> argv[] = {
        class2ParamHandle,
        strArgHandle
    };

    bool callbackReturnValue = false;
    return !invokeCallback(m_callback.newLocal(isolate), 2, argv, callbackReturnValue, scriptExecutionContext());
}

bool V8TestCallback::callbackWithStringList(RefPtr<DOMStringList> listParam)
{
    if (!canInvokeCallback())
        return true;

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope handleScope(isolate);

    v8::Handle<v8::Context> v8Context = toV8Context(scriptExecutionContext(), m_world.get());
    if (v8Context.IsEmpty())
        return true;

    v8::Context::Scope scope(v8Context);

    v8::Handle<v8::Value> listParamHandle = toV8(listParam, v8::Handle<v8::Object>(), isolate);
    if (listParamHandle.IsEmpty()) {
        if (!isScriptControllerTerminating())
            CRASH();
        return true;
    }

    v8::Handle<v8::Value> argv[] = {
        listParamHandle
    };

    bool callbackReturnValue = false;
    return !invokeCallback(m_callback.newLocal(isolate), 1, argv, callbackReturnValue, scriptExecutionContext());
}

bool V8TestCallback::callbackWithBoolean(bool boolParam)
{
    if (!canInvokeCallback())
        return true;

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope handleScope(isolate);

    v8::Handle<v8::Context> v8Context = toV8Context(scriptExecutionContext(), m_world.get());
    if (v8Context.IsEmpty())
        return true;

    v8::Context::Scope scope(v8Context);

    v8::Handle<v8::Value> boolParamHandle = v8Boolean(boolParam, isolate);
    if (boolParamHandle.IsEmpty()) {
        if (!isScriptControllerTerminating())
            CRASH();
        return true;
    }

    v8::Handle<v8::Value> argv[] = {
        boolParamHandle
    };

    bool callbackReturnValue = false;
    return !invokeCallback(m_callback.newLocal(isolate), 1, argv, callbackReturnValue, scriptExecutionContext());
}

bool V8TestCallback::callbackWithSequence(Vector<RefPtr<TestObj> > sequenceParam)
{
    if (!canInvokeCallback())
        return true;

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope handleScope(isolate);

    v8::Handle<v8::Context> v8Context = toV8Context(scriptExecutionContext(), m_world.get());
    if (v8Context.IsEmpty())
        return true;

    v8::Context::Scope scope(v8Context);

    v8::Handle<v8::Value> sequenceParamHandle = v8Array(sequenceParam, isolate);
    if (sequenceParamHandle.IsEmpty()) {
        if (!isScriptControllerTerminating())
            CRASH();
        return true;
    }

    v8::Handle<v8::Value> argv[] = {
        sequenceParamHandle
    };

    bool callbackReturnValue = false;
    return !invokeCallback(m_callback.newLocal(isolate), 1, argv, callbackReturnValue, scriptExecutionContext());
}

} // namespace WebCore

