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

#ifndef BROWSEREXTENSION_H_
#define BROWSEREXTENSION_H_

// Added for compilation of khtml/dom/html_document.cpp:184
#include <qdatastream.h>

// This is a bad hack to get some rendering code to work
#include <kxmlguiclient.h>

// classes and includes added for ecma directory
#include "part.h"
#include "browserinterface.h"
class KURL;

// Added for compilation of khtml/khtml_part.h:734
class QPoint;

// Added for compilation of khtml/khtml_part.h:755
class QEvent;

namespace KParts {

// Added for compilation of khtml/khtml_part.h:695
struct URLArgs {
    QString frameName;
    QString serviceType;
};

struct WindowArgs {
    int x;
    int y;
    int width;
    int height;
    bool menuBarVisible;
    bool statusBarVisible;
    bool toolBarsVisible;
    bool resizable;
    bool fullscreen;
};

class BrowserExtension {
public:
     BrowserInterface *browserInterface() const;
     void openURLRequest(const KURL &url, const KParts::URLArgs &args = KParts::URLArgs());
     void createNewWindow(const KURL &url, const KParts::URLArgs &args, const KParts::WindowArgs &windowArgs, KParts::ReadOnlyPart *&part);
};

class BrowserHostExtension {
};

}; // namespace KParts

#endif
