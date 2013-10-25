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

#include "config.h"
#include "bindings/v8/ExceptionMessages.h"

namespace WebCore {

String ExceptionMessages::failedToConstruct(const String& type, const String& detail)
{
    return "Failed to construct '" + type + (!detail.isEmpty() ? String("': " + detail) : String("'"));
}

String ExceptionMessages::failedToExecute(const String& method, const String& type, const String& detail)
{
    return "Failed to execute '" + method + "' on '" + type + (!detail.isEmpty() ? String("': " + detail) : String("'"));
}

String ExceptionMessages::failedToGet(const String& property, const String& type, const String& detail)
{
    return "Failed to read the '" + property + "' property from '" + type + "': " + detail;
}

String ExceptionMessages::failedToSet(const String& property, const String& type, const String& detail)
{
    return "Failed to set the '" + property + "' property on '" + type + "': " + detail;
}

String ExceptionMessages::failedToDelete(const String& property, const String& type, const String& detail)
{
    return "Failed to delete the '" + property + "' property from '" + type + "': " + detail;
}

String ExceptionMessages::notASequenceTypeArgumentOrValue(int argumentIndexOrValue)
{
    String kind(" argument");

    String prefix;
    switch (argumentIndexOrValue) {
    case 1:
        prefix = "First";
        break;
    case 2:
        prefix = "Second";
        break;
    case 3:
        prefix = "Third";
        break;
    default:
        if (argumentIndexOrValue <= 0)
            kind = "The value provided";
        else
            prefix = String::number(argumentIndexOrValue) + "th";
        break;
    }
    return prefix + kind + " is neither an array, nor does it have indexed properties.";
}

String ExceptionMessages::notASequenceTypeProperty(const String& propertyName)
{
    return "'" + propertyName + "' property is neither an array, nor does it have indexed properties.";
}

String ExceptionMessages::notEnoughArguments(unsigned expected, unsigned provided)
{
    return String::number(expected) + " argument" + (expected > 1 ? "s" : "") + " required, but only " + String::number(provided) + " present.";
}

} // namespace WebCore
