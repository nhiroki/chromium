/*
 * Copyright (C) 2001, 2002, 2003 Apple Computer, Inc.  All rights reserved.
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

#include "KWQXmlAttributes.h"

#include "KWQAssertions.h"

QXmlAttributes::QXmlAttributes(const char **expatStyleAttributes)
    : _ref(0)
{
    int length = 0;
    for (const char **p = expatStyleAttributes; *p; p += 2) {
        ASSERT(p[1]);
        ++length;
    }

    _length = length;
    if (!length) {
        _names = 0;
        _values = 0;
    } else {
        _names = new QString [length];
        _values = new QString [length];
    }

    int i = 0;
    for (const char **p = expatStyleAttributes; *p; p += 2) {
        _names[i] = QString::fromUtf8(p[0]);
        _values[i] = QString::fromUtf8(p[1]);
        ++i;
    }
}

QXmlAttributes::~QXmlAttributes()
{
    if (_ref && !--*_ref) {
        delete _ref;
        _ref = 0;
    }
    if (!_ref) {
        delete [] _names;
        delete [] _values;
    }
}

QXmlAttributes::QXmlAttributes(const QXmlAttributes &other)
    : _ref(other._ref)
    , _length(other._length)
    , _names(other._names)
    , _values(other._values)
{
    if (!_ref) {
        _ref = new int (2);
        other._ref = _ref;
    } else {
        ++*_ref;
    }
}

QXmlAttributes &QXmlAttributes::operator=(const QXmlAttributes &other)
{
    if (_ref && !--*_ref) {
        delete _ref;
        _ref = 0;
    }
    if (!_ref) {
        delete [] _names;
        delete [] _values;
    }

    _ref = other._ref;
    _length = other._length;
    _names = other._names;
    _values = other._values;

    if (!_ref) {
        _ref = new int (2);
        other._ref = _ref;
    } else {
        ++*_ref;
    }
    
    return *this;
}
    
QString QXmlAttributes::uri(int index) const
{
    return QString::null;
}

QString QXmlAttributes::value(const QString &name) const
{
    for (int i = 0; i != _length; ++i) {
        if (name == _names[i]) {
            return _values[i];
        }
    }
    return QString::null;
}

