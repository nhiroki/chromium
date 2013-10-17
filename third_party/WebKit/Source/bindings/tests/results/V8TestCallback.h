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

#ifndef V8TestCallback_h
#define V8TestCallback_h

#include "bindings/tests/idls/TestCallback.h"
#include "bindings/v8/ActiveDOMCallback.h"
#include "bindings/v8/DOMWrapperWorld.h"
#include "bindings/v8/ScopedPersistent.h"

namespace WebCore {

class ExecutionContext;


class V8TestCallback : public TestCallback, public ActiveDOMCallback {
public:
    static PassRefPtr<V8TestCallback> create(v8::Handle<v8::Value> jsValue, ExecutionContext* context)
    {
        ASSERT(jsValue->IsObject());
        ASSERT(context);
        return adoptRef(new V8TestCallback(v8::Handle<v8::Object>::Cast(jsValue), context));
    }

    virtual ~V8TestCallback();

    // Functions
    virtual bool callbackWithNoParam();
    virtual bool callbackWithTestObjectParam(TestObj* class1Param);
    virtual bool callbackWithTestObjectParam(TestObj* class2Param, const String& strArg);
    virtual int customCallback(TestObj* testObjParam, TestObj* testObjParam);
    virtual bool callbackWithStringList(RefPtr<DOMStringList> listParam);
    virtual bool callbackWithBoolean(bool boolParam);
    virtual bool callbackWithSequence(const Vector<RefPtr<TestObj> >& sequenceParam);
    virtual bool callbackWithFloat(float floatParam);
    virtual bool callbackWithThisArg(ScriptValue thisValue, int param);

    virtual ExecutionContext* executionContext() const { return ContextLifecycleObserver::executionContext(); }

private:
    V8TestCallback(v8::Handle<v8::Object>, ExecutionContext*);

    ScopedPersistent<v8::Object> m_callback;
    RefPtr<DOMWrapperWorld> m_world;
};

}

#endif // V8TestCallback_h

