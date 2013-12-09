/*
 * Copyright (C) 2007 Alexey Proskuryakov (ap@nypop.com)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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

#ifndef NativeXPathNSResolver_h
#define NativeXPathNSResolver_h

#include "core/xml/XPathNSResolver.h"
#include "wtf/RefPtr.h"

namespace WebCore {

class Node;

class NativeXPathNSResolver : public XPathNSResolver {
public:
    static PassRefPtr<NativeXPathNSResolver> create(PassRefPtr<Node> node) { return adoptRef(new NativeXPathNSResolver(node)); }
    virtual ~NativeXPathNSResolver();

    virtual AtomicString lookupNamespaceURI(const String& prefix) OVERRIDE;

private:
    explicit NativeXPathNSResolver(PassRefPtr<Node>);
    RefPtr<Node> m_node;
};

} // namespace WebCore

#endif // NativeXPathNSResolver_h
