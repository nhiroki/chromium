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

#ifndef QAPPLICATION_H_
#define QAPPLICATION_H_

#include <qobject.h>
#include <qwidget.h>
#include <qpalette.h>
#include <qsize.h>

class QDesktopWidget;

class QApplication : public QObject {
public:
    static QPalette palette(const QWidget *p=0);
    static QDesktopWidget *desktop();
    static int startDragDistance();
    static QSize globalStrut();
    static void	setOverrideCursor(const QCursor &);
    static void restoreOverrideCursor();
    static bool sendEvent(QObject *, QEvent *);
    static void sendPostedEvents(QObject *receiver, int event_type);

    QStyle &style() const;
};

extern QApplication *qApp;

class QDesktopWidget : public QWidget {
public:
    int screenNumber(QWidget *) const;
    QRect screenGeometry(int screenNumber);
    int width() const;
    int height() const;
};

#endif
