/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2009 Google Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef UString_h
#define UString_h

#include "Collector.h"
#include <stdint.h>
#include <string.h>
#include <wtf/Assertions.h>
#include <wtf/CrossThreadRefCounted.h>
#include <wtf/OwnFastMallocPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringImpl.h>
#include <wtf/unicode/Unicode.h>

namespace JSC {

class UString {
public:
    // Construct a null string, distinguishable from an empty string.
    UString() { }

    // Construct a string with UTF-16 data.
    UString(const UChar* characters, unsigned length);

    // Construct a string with UTF-16 data, from a null-terminated source.
    UString(const UChar*);

    // Construct a string with latin1 data.
    UString(const char* characters);

    // Construct a string with latin1 data, from a null-terminated source.
    UString(const char* characters, unsigned length);

    // Construct a string referencing an existing StringImpl.
    UString(StringImpl* impl) : m_impl(impl) { }
    UString(PassRefPtr<StringImpl> impl) : m_impl(impl) { }
    UString(RefPtr<StringImpl> impl) : m_impl(impl) { }

    // Inline the destructor.
    ALWAYS_INLINE ~UString() { }

    void swap(UString& o) { m_impl.swap(o.m_impl); }

    template<size_t inlineCapacity>
    static UString adopt(Vector<UChar, inlineCapacity>& vector) { return StringImpl::adopt(vector); }

    bool isNull() const { return !m_impl; }
    bool isEmpty() const { return !m_impl || !m_impl->length(); }

    StringImpl* impl() const { return m_impl.get(); }

    unsigned length() const
    {
        if (!m_impl)
            return 0;
        return m_impl->length();
    }

    const UChar* characters() const
    {
        if (!m_impl)
            return 0;
        return m_impl->characters();
    }

    CString utf8(bool strict = false) const;

    UChar operator[](unsigned index) const
    {
        if (!m_impl || index >= m_impl->length())
            return 0;
        return m_impl->characters()[index];
    }

    static UString number(int);
    static UString number(unsigned);
    static UString number(long);
    static UString number(long long);
    static UString number(double);



    double toDouble(bool tolerateTrailingJunk, bool tolerateEmptyString) const;
    double toDouble(bool tolerateTrailingJunk) const;
    double toDouble() const;

    uint32_t toUInt32(bool* ok = 0) const;
    uint32_t toUInt32(bool* ok, bool tolerateEmptyString) const;
    uint32_t toStrictUInt32(bool* ok = 0) const;

    static const unsigned NotFound = 0xFFFFFFFFu;
    unsigned find(const UString& f, unsigned pos = 0) const;
    unsigned find(UChar, unsigned pos = 0) const;
    unsigned rfind(const UString& f, unsigned pos) const;
    unsigned rfind(UChar, unsigned pos) const;

    UString substr(unsigned pos = 0, unsigned len = 0xFFFFFFFF) const;

    // NOTE: This method should only be used for *debugging* purposes as it
    // is neither Unicode safe nor free from side effects nor thread-safe.
    char* ascii() const;

private:
    RefPtr<StringImpl> m_impl;
};

ALWAYS_INLINE bool operator==(const UString& s1, const UString& s2)
{
    StringImpl* rep1 = s1.impl();
    StringImpl* rep2 = s2.impl();
    unsigned size1 = 0;
    unsigned size2 = 0;

    if (rep1 == rep2) // If they're the same rep, they're equal.
        return true;
    
    if (rep1)
        size1 = rep1->length();
        
    if (rep2)
        size2 = rep2->length();
        
    if (size1 != size2) // If the lengths are not the same, we're done.
        return false;
    
    if (!size1)
        return true;
    
    // At this point we know 
    //   (a) that the strings are the same length and
    //   (b) that they are greater than zero length.
    const UChar* d1 = rep1->characters();
    const UChar* d2 = rep2->characters();
    
    if (d1 == d2) // Check to see if the data pointers are the same.
        return true;
    
    // Do quick checks for sizes 1 and 2.
    switch (size1) {
    case 1:
        return d1[0] == d2[0];
    case 2:
        return (d1[0] == d2[0]) & (d1[1] == d2[1]);
    default:
        return memcmp(d1, d2, size1 * sizeof(UChar)) == 0;
    }
}


inline bool operator!=(const UString& s1, const UString& s2)
{
    return !JSC::operator==(s1, s2);
}

bool operator<(const UString& s1, const UString& s2);
bool operator>(const UString& s1, const UString& s2);

bool operator==(const UString& s1, const char* s2);

inline bool operator!=(const UString& s1, const char* s2)
{
    return !JSC::operator==(s1, s2);
}

inline bool operator==(const char *s1, const UString& s2)
{
    return operator==(s2, s1);
}

inline bool operator!=(const char *s1, const UString& s2)
{
    return !JSC::operator==(s1, s2);
}

inline int codePointCompare(const UString& s1, const UString& s2)
{
    return codePointCompare(s1.impl(), s2.impl());
}

// We'd rather not do shared substring append for small strings, since
// this runs too much risk of a tiny initial string holding down a
// huge buffer.
static const unsigned minShareSize = Heap::minExtraCost / sizeof(UChar);

struct IdentifierRepHash : PtrHash<RefPtr<StringImpl> > {
    static unsigned hash(const RefPtr<StringImpl>& key) { return key->existingHash(); }
    static unsigned hash(StringImpl* key) { return key->existingHash(); }
};

} // namespace JSC

namespace WTF {

template<typename T> struct DefaultHash;
template<typename T> struct StrHash;

template<> struct StrHash<StringImpl*> {
    static unsigned hash(const StringImpl* key) { return key->hash(); }
    static bool equal(const StringImpl* a, const StringImpl* b) { return ::equal(a, b); }
    static const bool safeToCompareToEmptyOrDeleted = false;
};

template<> struct StrHash<RefPtr<StringImpl> > : public StrHash<StringImpl*> {
    using StrHash<StringImpl*>::hash;
    static unsigned hash(const RefPtr<StringImpl>& key) { return key->hash(); }
    using StrHash<StringImpl*>::equal;
    static bool equal(const RefPtr<StringImpl>& a, const RefPtr<StringImpl>& b) { return ::equal(a.get(), b.get()); }
    static bool equal(const StringImpl* a, const RefPtr<StringImpl>& b) { return ::equal(a, b.get()); }
    static bool equal(const RefPtr<StringImpl>& a, const StringImpl* b) { return ::equal(a.get(), b); }

    static const bool safeToCompareToEmptyOrDeleted = false;
};

template <> struct VectorTraits<JSC::UString> : SimpleClassVectorTraits
{
    static const bool canInitializeWithMemset = true;
};
    
} // namespace WTF

#endif

