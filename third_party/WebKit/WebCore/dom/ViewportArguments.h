/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef ViewportArguments_h
#define ViewportArguments_h

#include <wtf/Forward.h>

namespace WebCore {

class Document;

enum ViewportErrorCode {
    DeviceWidthShouldBeUsedWarning,
    DeviceHeightShouldBeUsedWarning,
    UnrecognizedViewportArgumentError,
    MaximumScaleTooLargeError,
    TargetDensityDpiTooSmallOrLargeError
};

struct ViewportArguments {

    enum { ValueUndefined = -1 };

    ViewportArguments()
        : initialScale(ValueUndefined)
        , minimumScale(ValueUndefined)
        , maximumScale(ValueUndefined)
        , width(ValueUndefined)
        , height(ValueUndefined)
        , targetDensityDpi(ValueUndefined)
        , userScalable(ValueUndefined)
    {
    }

    float initialScale;
    float minimumScale;
    float maximumScale;
    float width;
    float height;
    float targetDensityDpi;

    float userScalable;

    bool hasCustomArgument() const
    {
        return initialScale != ValueUndefined || minimumScale != ValueUndefined || maximumScale != ValueUndefined || width != ValueUndefined || height != ValueUndefined || userScalable != ValueUndefined || targetDensityDpi != ValueUndefined;
    }
};

void setViewportFeature(const String& keyString, const String& valueString, Document*, void* data);
void reportViewportWarning(Document*, ViewportErrorCode, const String& replacement);

} // namespace WebCore

#endif // ViewportArguments_h
