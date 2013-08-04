/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef DocumentOrderedMap_h
#define DocumentOrderedMap_h

#include "wtf/HashCountedSet.h"
#include "wtf/HashMap.h"
#include "wtf/text/StringImpl.h"

namespace WebCore {

class Element;
class TreeScope;

class DocumentOrderedMap {
public:
    void add(StringImpl*, Element*);
    void remove(StringImpl*, Element*);
    void clear();

    bool contains(StringImpl*) const;
    bool mightContainMultiple(StringImpl*) const;
    // concrete instantiations of the get<>() method template
    Element* getElementById(StringImpl*, const TreeScope*) const;
    Element* getElementByName(StringImpl*, const TreeScope*) const;
    Element* getElementByMapName(StringImpl*, const TreeScope*) const;
    Element* getElementByLowercasedMapName(StringImpl*, const TreeScope*) const;
    Element* getElementByLabelForAttribute(StringImpl*, const TreeScope*) const;
    Element* getElementByWindowNamedItem(StringImpl*, const TreeScope*) const;
    Element* getElementByDocumentNamedItem(StringImpl*, const TreeScope*) const;

    void checkConsistency() const;

private:
    template<bool keyMatches(StringImpl*, Element*)> Element* get(StringImpl*, const TreeScope*) const;

    typedef HashMap<StringImpl*, Element*> Map;

    // We maintain the invariant that m_duplicateCounts is the count of all elements with a given key
    // excluding the one referenced in m_map, if any. This means it one less than the total count
    // when the first node with a given key is cached, otherwise the same as the total count.
    mutable Map m_map;
    mutable HashCountedSet<StringImpl*> m_duplicateCounts;
};

inline bool DocumentOrderedMap::contains(StringImpl* id) const
{
    return m_map.contains(id) || m_duplicateCounts.contains(id);
}

inline bool DocumentOrderedMap::mightContainMultiple(StringImpl* id) const
{
    return (m_map.contains(id) ? 1 : 0) + m_duplicateCounts.count(id) > 1;
}

} // namespace WebCore

#endif // DocumentOrderedMap_h
