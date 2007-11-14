/*
 * Copyright 2005 Frerich Raabe <raabe@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef XPathEvaluator_h
#define XPathEvaluator_h

#if ENABLE(XPATH)

#include <wtf/Shared.h>
#include <wtf/Forward.h>

namespace WebCore {

    typedef int ExceptionCode;

    class Node;
    class String;
    class XPathExpression;
    class XPathNSResolver;
    class XPathResult;

    // FIXME: Should these exception codes move to another header?
    const int XPathExceptionOffset = 400;
    const int XPathExceptionMax = 499;
    enum XPathExceptionCode { INVALID_EXPRESSION_ERR = XPathExceptionOffset + 51, TYPE_ERR };

    class XPathEvaluator : public Shared<XPathEvaluator> {
    public:
        PassRefPtr<XPathExpression> createExpression(const String& expression, XPathNSResolver*, ExceptionCode&);
        PassRefPtr<XPathNSResolver> createNSResolver(Node* nodeResolver);
        PassRefPtr<XPathResult> evaluate(const String& expression, Node* contextNode,
            XPathNSResolver*, unsigned short type, XPathResult*, ExceptionCode&);
    };

}

#endif // ENABLE(XPATH)

#endif // XPathEvaluator_h
