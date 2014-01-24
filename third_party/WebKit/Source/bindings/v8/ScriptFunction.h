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

#ifndef ScriptFunction_h
#define ScriptFunction_h

#include "bindings/v8/ScriptValue.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8GarbageCollected.h"
#include "wtf/RefCounted.h"
#include <v8.h>

namespace WebCore {

class ScriptFunction;
v8::Handle<v8::Function> adoptByGarbageCollector(PassOwnPtr<ScriptFunction>);

class ScriptFunction : public V8GarbageCollected<ScriptFunction> {
public:
    virtual ~ScriptFunction() { }

protected:
    ScriptFunction(v8::Isolate* isolate) : V8GarbageCollected<ScriptFunction>(isolate) { }

private:
    friend v8::Handle<v8::Function> adoptByGarbageCollector(PassOwnPtr<ScriptFunction>);

    virtual ScriptValue call(ScriptValue) = 0;

    static void callCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
    {
        v8::Isolate* isolate = args.GetIsolate();
        ASSERT(!args.Data().IsEmpty());
        ScriptFunction* function = ScriptFunction::Cast(args.Data());
        v8::Local<v8::Value> value = args.Length() > 0 ? args[0] : v8::Local<v8::Value>(v8::Undefined(isolate));

        ScriptValue result = function->call(ScriptValue(value, isolate));

        v8SetReturnValue(args, result.v8Value());
    }
};

inline v8::Handle<v8::Function> adoptByGarbageCollector(PassOwnPtr<ScriptFunction> function)
{
    if (!function)
        return v8::Handle<v8::Function>();
    v8::Isolate* isolate = function->isolate();
    return createClosure(&ScriptFunction::callCallback, function.leakPtr()->releaseToV8GarbageCollector(), isolate);
}

} // namespace WebCore

#endif
