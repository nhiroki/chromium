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

#ifndef PART_H_
#define PART_H_

#include <kurl.h>
#include <qobject.h>
#include <qvariant.h>
#include <qlist.h>
#include <qstringlist.h>
#include <qvaluelist.h>

// added to help in compilation of khtml/khtml_part.h:867
namespace KIO {
class Job;
} // namespace KIO
 
// forward declaration hack to help in compilation of khtml/khtml_part.h:166
class QWidget;

// forward declaration hack to help in compilation of khtml/khtml_part.h:552
class QCursor;

// forward declaration hack to help in compilation of khtml/khtml_part.h:631
class QDataStream;

// forward declaration hack to help in compilation of khtml/ecma/kjs_binding.cpp:28
class QPainter;

namespace KParts {

class Part : public QObject {
public:
    QWidget *widget();
    void setWindowCaption(const QString &);
};

class ReadOnlyPart : public Part {
public:
    virtual const KURL & url() const;
};

// hack to help in compilation of khtml/khtml_part.h:785
class GUIActivateEvent {
};

} // namespace KParts

#endif
