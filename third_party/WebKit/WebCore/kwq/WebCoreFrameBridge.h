/*
 * Copyright (C) 2002 Apple Computer, Inc.  All rights reserved.
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

#import <WebCore/WebCoreBridge.h>

// The main difference between a WebCoreFrameBridge and a WebCoreBridge
// is that there's no guarantee a WebCoreFrameBridge will have any HTML in
// it, thus no guarantee that it will have a KHTMLPart.

// The WebCoreFrameBridge interface contains methods for use by the non-WebCore side of the bridge.

@interface WebCoreFrameBridge : NSObject
{
    KHTMLRenderPart *renderPart;
}

- (void)setRenderPart:(KHTMLRenderPart *)renderPart;
- (KHTMLRenderPart *)renderPart;

@end

// The WebCoreFrameBridge protocol contains methods for use by the WebCore side of the bridge.

@protocol WebCoreFrameBridge

- (void)loadURL:(NSURL *)URL;
- (void)postWithURL:(NSURL *)URL data:(NSData *)data;

- (WebCoreBridge *)bridge;

@end

// This interface definition allows those who hold a WebCoreFrameBridge * to call all the methods
// in the WebCoreBridge protocol without requiring the base implementation to supply the methods.
// This idiom is appropriate because WebCoreFrameBridge is an abstract class.

@interface WebCoreFrameBridge (SubclassResponsibility) <WebCoreFrameBridge>
@end
