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

#ifndef QCOMBOBOX_H_
#define QCOMBOBOX_H_

#include "KWQListBox.h"
#include "KWQWidget.h"

#ifdef __OBJC__
@class KWQComboBoxAdapter;
#else
class KWQComboBoxAdapter;
#endif

class QComboBox : public QWidget {
public:
    QComboBox();
    ~QComboBox();
    
    void clear();
    void insertItem(const QString &text, int index=-1);

    int currentItem() const;
    void setCurrentItem(int);

    QListBox *listBox() const { return 0; }
    void popup() { }
    
    QSize sizeHint() const;
    QRect frameGeometry() const;
    void setFrameGeometry(const QRect &);
    int baselinePosition() const;

    bool eventFilter(QObject *object, QEvent *event) { return false; }

    void activated() { _activated.call(currentItem()); }

private:
    KWQComboBoxAdapter *_adapter;
    mutable float _width;
    mutable bool _widthGood;

    KWQSignal _activated;
};

#endif
