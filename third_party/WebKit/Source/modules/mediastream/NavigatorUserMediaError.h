/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NavigatorUserMediaError_h
#define NavigatorUserMediaError_h

#include "bindings/v8/ScriptWrappable.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefCounted.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

class NavigatorUserMediaError : public RefCounted<NavigatorUserMediaError>, public ScriptWrappable {
public:
    enum Name {
        NamePermissionDenied,
        NameConstraintNotSatisfied
    };

    static PassRefPtr<NavigatorUserMediaError> create(Name name, const String& message, const String& constraintName)
    {
        return adoptRef(new NavigatorUserMediaError(name, message, constraintName));
    }

    virtual ~NavigatorUserMediaError() { }

    String name() const;
    const String& message() const { return m_message; }
    const String& constraintName() const { return m_constraintName; }

private:
    NavigatorUserMediaError(Name name, const String& message, const String& constraintName)
        : m_name(name), m_message(message), m_constraintName(constraintName)
    {
        ScriptWrappable::init(this);
    }

    Name m_name;
    String m_message;
    String m_constraintName;
};

} // namespace WebCore

#endif // NavigatorUserMediaError_h
