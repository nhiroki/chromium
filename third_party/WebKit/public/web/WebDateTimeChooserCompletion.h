/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef WebDateTimeChooserCompletion_h
#define WebDateTimeChooserCompletion_h

#include "public/platform/WebString.h"

namespace blink {

// Gets called back when WebViewClient finished choosing a date/time value.
class WebDateTimeChooserCompletion {
public:

    // Called with a date/time value in the HTML format. The callback instance
    // is destroyed when this method is called.
    // FIXME: Remove. Deprecated in favor of double version.
    virtual void didChooseValue(const WebString&) = 0;

    // Called with a date/time value in the HTML format. The callback instance
    // is destroyed when this method is called. If the value is NaN it means an
    // empty value. Value should not be infinity.
    virtual void didChooseValue(double) = 0;

    // Called when a user closed the chooser without choosing a value. The
    // callback instance is destroyed when this method is called.
    virtual void didCancelChooser() = 0;

protected:
    virtual ~WebDateTimeChooserCompletion() { }
};

} // namespace blink

#endif
