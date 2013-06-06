/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "bindings/v8/V8ScriptRunner.h"

#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8GCController.h"
#include "bindings/v8/V8RecursionScope.h"
#include "core/dom/ScriptExecutionContext.h"
#include "core/loader/CachedMetadata.h"
#include "core/loader/cache/CachedScript.h"
#include "core/platform/chromium/TraceEvent.h"

namespace WebCore {

v8::Local<v8::Script> V8ScriptRunner::compileScript(v8::Handle<v8::String> code, const String& fileName, const TextPosition& scriptStartPosition, v8::Isolate* isolate)
{
    TRACE_EVENT0("v8", "v8.compile");
    v8::Handle<v8::String> name = v8String(fileName, isolate);
    v8::Handle<v8::Integer> line = v8Integer(scriptStartPosition.m_line.zeroBasedInt(), isolate);
    v8::Handle<v8::Integer> column = v8Integer(scriptStartPosition.m_column.zeroBasedInt(), isolate);
    v8::ScriptOrigin origin(name, line, column);
    return v8::Script::Compile(code, &origin);
}

v8::Local<v8::Value> V8ScriptRunner::runCompiledScript(v8::Handle<v8::Script> script, ScriptExecutionContext* context)
{
    TRACE_EVENT0("v8", "v8.run");
    if (script.IsEmpty())
        return v8::Local<v8::Value>();

    V8GCController::checkMemoryUsage();
    if (V8RecursionScope::recursionLevel() >= kMaxRecursionDepth)
        return handleMaxRecursionDepthExceeded();

    if (handleOutOfMemory())
        return v8::Local<v8::Value>();

    // Run the script and keep track of the current recursion depth.
    v8::Local<v8::Value> result;
    {
        V8RecursionScope recursionScope(context);
        result = script->Run();
    }

    if (handleOutOfMemory())
        ASSERT(result.IsEmpty());

    if (result.IsEmpty())
        return v8::Local<v8::Value>();

    crashIfV8IsDead();
    return result;
}

v8::Local<v8::Value> V8ScriptRunner::compileAndRunInternalScript(v8::Handle<v8::String> source, v8::Isolate* isolate, v8::Local<v8::Context> context, const String& fileName, const TextPosition& scriptStartPosition)
{
    TRACE_EVENT0("v8", "v8.run");
    v8::Local<v8::Value> result;
    if (context.IsEmpty())
        context = v8::Context::New(isolate);
    if (context.IsEmpty())
        return result;

    v8::Context::Scope scope(context);
    v8::Handle<v8::Script> script = V8ScriptRunner::compileScript(source, fileName, scriptStartPosition, isolate);
    if (script.IsEmpty())
        return result;

    V8RecursionScope::MicrotaskSuppression recursionScope;
    result = script->Run();
    return result;
}

static String functionInfo(const v8::Handle<v8::Function> function)
{
    String resourceName = "undefined";
    int lineNumber = 1;
    v8::ScriptOrigin origin = function->GetScriptOrigin();
    if (!origin.ResourceName().IsEmpty()) {
        resourceName = toWebCoreString(origin.ResourceName());
        lineNumber = function->GetScriptLineNumber() + 1;
    }

    StringBuilder builder;
    builder.append(resourceName);
    builder.append(':');
    builder.appendNumber(lineNumber);
    return builder.toString();
}

v8::Local<v8::Value> V8ScriptRunner::callFunction(v8::Handle<v8::Function> function, ScriptExecutionContext* context, v8::Handle<v8::Object> receiver, int argc, v8::Handle<v8::Value> args[])
{
    TRACE_EVENT0("v8", "v8.callFunction");
    V8GCController::checkMemoryUsage();

    if (V8RecursionScope::recursionLevel() >= kMaxRecursionDepth)
        return handleMaxRecursionDepthExceeded();

    V8RecursionScope recursionScope(context);
    v8::Local<v8::Value> result = function->Call(receiver, argc, args);

    crashIfV8IsDead();
    return result;
}

v8::Local<v8::Value> V8ScriptRunner::callInternalFunction(v8::Handle<v8::Function> function, v8::Local<v8::Context> context, v8::Handle<v8::Object> receiver, int argc, v8::Handle<v8::Value> args[], v8::Isolate* isolate)
{
    TRACE_EVENT0("v8", "v8.callFunction");
    v8::Local<v8::Value> result;
    if (context.IsEmpty())
        context = v8::Context::New(isolate);
    if (context.IsEmpty())
        return result;

    v8::Context::Scope scope(context);
    V8RecursionScope::MicrotaskSuppression recursionScope;
    result = function->Call(receiver, argc, args);
    crashIfV8IsDead();
    return result;
}

v8::Local<v8::Value> V8ScriptRunner::callAsFunction(v8::Handle<v8::Object> object, v8::Handle<v8::Object> receiver, int argc, v8::Handle<v8::Value> args[])
{
    V8RecursionScope::MicrotaskSuppression recursionScope;
    v8::Local<v8::Value> result = object->CallAsFunction(receiver, argc, args);
    crashIfV8IsDead();
    return result;
}

v8::Local<v8::Value> V8ScriptRunner::callAsConstructor(v8::Handle<v8::Object> object, int argc, v8::Handle<v8::Value> args[])
{
    V8RecursionScope::MicrotaskSuppression recursionScope;
    v8::Local<v8::Value> result = object->CallAsConstructor(argc, args);
    crashIfV8IsDead();
    return result;
}

} // namespace WebCore
