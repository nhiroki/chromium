/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#include "config.h"
#include "V8CustomEvent.h"

#include "V8Event.h"
#include "bindings/v8/BindingState.h"
#include "bindings/v8/Dictionary.h"
#include "bindings/v8/RuntimeEnabledFeatures.h"
#include "bindings/v8/ScriptState.h"
#include "bindings/v8/ScriptValue.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "bindings/v8/V8HiddenPropertyName.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/ExceptionCode.h"
#include "core/page/Frame.h"

namespace WebCore {

v8::Handle<v8::Value> V8CustomEvent::detailAttrGetterCustom(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    CustomEvent* imp = V8CustomEvent::toNative(info.Holder());
    RefPtr<SerializedScriptValue> serialized = imp->serializedScriptValue();
    if (serialized) {
        v8::Handle<v8::Value> value = info.Holder()->GetHiddenValue(V8HiddenPropertyName::detail());
        if (value.IsEmpty()) {
            value = serialized->deserialize();
            info.Holder()->SetHiddenValue(V8HiddenPropertyName::detail(), value);
        }
        return value;
    }
    return imp->detail().v8Value();
}

} // namespace WebCore
