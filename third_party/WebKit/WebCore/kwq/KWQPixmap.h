/*
 * Copyright (C) 2001, 2002 Apple Computer, Inc.  All rights reserved.
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

#ifndef QPIXMAP_H_
#define QPIXMAP_H_

#include "qpaintdevice.h"
#include "qcolor.h"
#include "qstring.h"
#include "qnamespace.h"
#include "qimage.h"
#include "qsize.h"
#include "qrect.h"
#include "qpainter.h"

#ifdef __OBJC__
@protocol WebCoreImageRenderer;
typedef id <WebCoreImageRenderer> WebCoreImageRendererPtr;
#else
class WebCoreImageRenderer;
typedef WebCoreImageRenderer *WebCoreImageRendererPtr;
#endif

class QWMatrix;

class QPixmap : public QPaintDevice, public Qt {
public:
    QPixmap();
    QPixmap(const QSize&);
    QPixmap(const QByteArray&);
    QPixmap(int, int);
    QPixmap(const QPixmap &);
    ~QPixmap();

    bool isNull() const;

    QSize size() const;
    QRect rect() const;
    int width() const;
    int height() const;
    void resize(const QSize &);
    void resize(int, int);

    QPixmap xForm(const QWMatrix &) const;
    QImage convertToImage() const;
    
    bool mask() const;

    QPixmap &operator=(const QPixmap &);

    WebCoreImageRendererPtr getImageRenderer() const { return imageRenderer; }

private:
    WebCoreImageRendererPtr imageRenderer;
    bool needCopyOnWrite;

    friend class QPainter;

};

#endif
