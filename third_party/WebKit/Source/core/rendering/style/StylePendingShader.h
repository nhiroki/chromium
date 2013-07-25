/*
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef StylePendingShader_h
#define StylePendingShader_h

#include "core/css/CSSShaderValue.h"
#include "core/css/CSSValue.h"
#include "core/rendering/style/StyleShader.h"
#include "wtf/PassRefPtr.h"

namespace WebCore {

class WebKitCSSShaderValue;

class StylePendingShader : public StyleShader {
public:
    static PassRefPtr<StylePendingShader> create(CSSShaderValue* value) { return adoptRef(new StylePendingShader(value)); }
    
    virtual PassRefPtr<CSSValue> cssValue() const { return m_value; }
    CSSShaderValue* cssShaderValue() const { return m_value; }
private:
    StylePendingShader(CSSShaderValue* value)
        : m_value(value)
    {
         m_isPendingShader = true;
    }
    
    CSSShaderValue* m_value; // Not retained; it owns us.
};

}

#endif // StylePendingShader_h
