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
#include "V8Blob.h"

#include "bindings/v8/Dictionary.h"
#include "bindings/v8/ExceptionMessages.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8Utilities.h"
#include "bindings/v8/custom/V8ArrayBufferCustom.h"
#include "bindings/v8/custom/V8ArrayBufferViewCustom.h"
#include "core/fileapi/BlobBuilder.h"
#include "wtf/RefPtr.h"

namespace WebCore {

void V8Blob::constructorCustom(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (!info.Length()) {
        RefPtr<Blob> blob = Blob::create();
        info.GetReturnValue().Set(toV8(blob.get(), info.Holder(), info.GetIsolate()));
        return;
    }

    uint32_t length = 0;
    if (info[0]->IsArray()) {
        length = v8::Local<v8::Array>::Cast(info[0])->Length();
    } else {
        const int sequenceArgumentIndex = 0;
        if (toV8Sequence(info[sequenceArgumentIndex], length, info.GetIsolate()).IsEmpty()) {
            throwTypeError(ExceptionMessages::failedToConstruct("Blob", ExceptionMessages::notAnArrayTypeArgumentOrValue(sequenceArgumentIndex + 1)), info.GetIsolate());
            return;
        }
    }

    String type;
    String endings = "transparent";

    if (info.Length() > 1) {
        if (!info[1]->IsObject()) {
            throwTypeError(ExceptionMessages::failedToConstruct("Blob", "The 2nd argument is not of type Object."), info.GetIsolate());
            return;
        }

        V8TRYCATCH_VOID(Dictionary, dictionary, Dictionary(info[1], info.GetIsolate()));

        V8TRYCATCH_VOID(bool, containsEndings, dictionary.get("endings", endings));
        if (containsEndings) {
            if (endings != "transparent" && endings != "native") {
                throwTypeError(ExceptionMessages::failedToConstruct("Blob", "The 2nd argument's \"endings\" property must be either \"transparent\" or \"native\"."), info.GetIsolate());
                return;
            }
        }

        V8TRYCATCH_VOID(bool, containsType, dictionary.get("type", type));
        UNUSED_PARAM(containsType);
        if (!type.containsOnlyASCII()) {
            throwError(v8SyntaxError, ExceptionMessages::failedToConstruct("Blob", "The 2nd argument's \"type\" property must consist of ASCII characters."), info.GetIsolate());
            return;
        }
        type = type.lower();
    }

    ASSERT(endings == "transparent" || endings == "native");

    BlobBuilder blobBuilder;
    v8::Local<v8::Object> blobParts = v8::Local<v8::Object>::Cast(info[0]);
    for (uint32_t i = 0; i < length; ++i) {
        v8::Local<v8::Value> item = blobParts->Get(v8::Uint32::New(i, info.GetIsolate()));
        if (item.IsEmpty())
            return;

        if (V8ArrayBuffer::hasInstance(item, info.GetIsolate(), worldType(info.GetIsolate()))) {
            ArrayBuffer* arrayBuffer = V8ArrayBuffer::toNative(v8::Handle<v8::Object>::Cast(item));
            ASSERT(arrayBuffer);
            blobBuilder.append(arrayBuffer);
        } else if (V8ArrayBufferView::hasInstance(item, info.GetIsolate(), worldType(info.GetIsolate()))) {
            ArrayBufferView* arrayBufferView = V8ArrayBufferView::toNative(v8::Handle<v8::Object>::Cast(item));
            ASSERT(arrayBufferView);
            blobBuilder.append(arrayBufferView);
        } else if (V8Blob::hasInstance(item, info.GetIsolate(), worldType(info.GetIsolate()))) {
            Blob* blob = V8Blob::toNative(v8::Handle<v8::Object>::Cast(item));
            ASSERT(blob);
            blobBuilder.append(blob);
        } else {
            V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, stringValue, item);
            blobBuilder.append(stringValue, endings);
        }
    }

    RefPtr<Blob> blob = blobBuilder.getBlob(type);
    info.GetReturnValue().Set(toV8(blob.get(), info.Holder(), info.GetIsolate()));
}

} // namespace WebCore
