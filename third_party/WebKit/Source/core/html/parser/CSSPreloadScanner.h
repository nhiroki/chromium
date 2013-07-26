/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2010 Google Inc. All Rights Reserved.
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

#ifndef CSSPreloadScanner_h
#define CSSPreloadScanner_h

#include "core/html/parser/HTMLResourcePreloader.h"
#include "core/html/parser/HTMLToken.h"
#include "wtf/text/StringBuilder.h"

namespace WebCore {

class HTMLIdentifier;
class SegmentedString;

class CSSPreloadScanner {
    WTF_MAKE_NONCOPYABLE(CSSPreloadScanner);
public:
    CSSPreloadScanner();
    ~CSSPreloadScanner();

    void reset();

    void scan(const HTMLToken::DataVector&, const SegmentedString&, PreloadRequestStream&);
    void scan(const HTMLIdentifier&, const SegmentedString&, PreloadRequestStream&);

private:
    enum State {
        Initial,
        MaybeComment,
        Comment,
        MaybeCommentEnd,
        RuleStart,
        Rule,
        AfterRule,
        RuleValue,
        AfterRuleValue,
        DoneParsingImportRules,
    };

    template<typename Char>
    void scanCommon(const Char* begin, const Char* end, const SegmentedString&, PreloadRequestStream&);

    inline void tokenize(UChar, const SegmentedString&);
    void emitRule(const SegmentedString&);

    State m_state;
    StringBuilder m_rule;
    StringBuilder m_ruleValue;

    // Only non-zero during scan()
    PreloadRequestStream* m_requests;
};

}

#endif
