/*
 * Copyright (C) 2001 Apple Computer, Inc.  All rights reserved.
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

#ifndef QFONT_H_
#define QFONT_H_

class QString;

#ifdef __OBJC__
@class NSFont;
@class NSString;
#else
class NSFont;
class NSString;
#endif

class QFont {
public:
    enum Weight { Normal = 50, Bold = 63 };

    QFont();

    void setFamily(const QString &);
    QString family() const;

    void setWeight(int);
    int weight() const;
    bool bold() const;

    void setItalic(bool);
    bool italic() const;

    void setPixelSize(float s) { _size = s; }
    int pixelSize() const { return (int)_size; }

    bool operator==(const QFont &x) const;
    bool operator!=(const QFont &x) const { return !(*this == x); }
    
    NSString *getNSFamily() const { return _family; }
    int getNSTraits() const { return _trait; }
    float getNSSize() const { return _size; }
    
    NSFont *getNSFont() const;

private:
    NSString *_family;
    int _trait;
    float _size;
};

#endif
