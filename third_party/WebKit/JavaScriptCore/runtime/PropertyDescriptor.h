/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef PropertyDescriptor_h
#define PropertyDescriptor_h

#include "JSValue.h"

namespace JSC {
    class PropertyDescriptor {
    public:
        PropertyDescriptor()
            : m_attributes(0)
        {
        }
        bool writable() const;
        bool enumerable() const;
        bool configurable() const;
        bool hasAccessors() const;
        unsigned attributes() const { return m_attributes; }
#ifndef NDEBUG
        bool isValid() const { return m_value || ((m_getter || m_setter) && hasAccessors()); }
#endif
        JSValue value() const { ASSERT(m_value); return m_value; }
        JSValue getter() const;
        JSValue setter() const;
        void setUndefined();
        void setDescriptor(JSValue value, unsigned attributes);
        void setAccessorDescriptor(JSValue getter, JSValue setter, unsigned attributes);
    private:
        // May be a getter/setter
        JSValue m_value;
        JSValue m_getter;
        JSValue m_setter;
        unsigned m_attributes;
    };
}

#endif
