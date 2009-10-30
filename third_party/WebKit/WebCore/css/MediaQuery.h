/*
 * CSS Media Query
 *
 * Copyright (C) 2006 Kimmo Kinnunen <kimmo.t.kinnunen@nokia.com>.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef MediaQuery_h
#define MediaQuery_h

#include "PlatformString.h"
#include <wtf/Vector.h>

namespace WebCore {
class MediaQueryExp;

class MediaQuery : public Noncopyable {
public:
    enum Restrictor {
        Only, Not, None
    };

    MediaQuery(Restrictor r, const String& mediaType, Vector<MediaQueryExp*>* exprs);
    ~MediaQuery();

    Restrictor restrictor() const { return m_restrictor; }
    const Vector<MediaQueryExp*>* expressions() const { return m_expressions; }
    String mediaType() const { return m_mediaType; }
    bool operator==(const MediaQuery& other) const;
    void append(MediaQueryExp* newExp) { m_expressions->append(newExp); }
    String cssText() const;

 private:
    Restrictor m_restrictor;
    String m_mediaType;
    Vector<MediaQueryExp*>* m_expressions;
};

} // namespace

#endif
