/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTAwBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WKIconDatabaseCG.h"

#include "WebIconDatabase.h"
#include "WKAPICast.h"
#include "WKSharedAPICast.h"
#include <WebCore/Image.h>

using namespace WebKit;
using namespace WebCore;

CGImageRef WKIconDatabaseTryGetCGImageForURL(WKIconDatabaseRef iconDatabaseRef, WKURLRef urlRef, WKSize size)
{
    Image* image = toImpl(iconDatabaseRef)->imageForPageURL(toWTFString(urlRef));
    return image ? image->getFirstCGImageRefOfSize(IntSize(static_cast<int>(size.width), static_cast<int>(size.height))) : 0;
}

CFArrayRef WKIconDatabaseTryCopyCGImageArrayForURL(WKIconDatabaseRef iconDatabaseRef, WKURLRef urlRef)
{
    Image* image = toImpl(iconDatabaseRef)->imageForPageURL(toWTFString(urlRef));
    return image ? image->getCGImageArray().leakRef() : 0;
}

