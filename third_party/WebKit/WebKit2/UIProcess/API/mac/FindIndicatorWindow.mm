/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "FindIndicatorWindow.h"

#include "FindIndicator.h"
#include <WebCore/GraphicsContext.h>

using namespace WebCore;

@interface WebFindIndicatorView : NSView {
    RefPtr<WebKit::FindIndicator> _findIndicator;
}

- (id)_initWithFindIndicator:(PassRefPtr<WebKit::FindIndicator>)findIndicator;
@end

@implementation WebFindIndicatorView

- (id)_initWithFindIndicator:(PassRefPtr<WebKit::FindIndicator>)findIndicator
{
    if ((self = [super initWithFrame:NSZeroRect]))
        _findIndicator = findIndicator;

    return self;
}

- (void)drawRect:(NSRect)rect
{
    GraphicsContext graphicsContext(static_cast<CGContextRef>([[NSGraphicsContext currentContext] graphicsPort]));

    _findIndicator->draw(graphicsContext, enclosingIntRect(rect));
}

- (BOOL)isFlipped
{
    return YES;
}

@end

namespace WebKit {

PassOwnPtr<FindIndicatorWindow> FindIndicatorWindow::create(WKView *wkView)
{
    return adoptPtr(new FindIndicatorWindow(wkView));
}

FindIndicatorWindow::FindIndicatorWindow(WKView *wkView)
    : m_wkView(wkView)
{
}

FindIndicatorWindow::~FindIndicatorWindow()
{
    closeWindow();
}

void FindIndicatorWindow::setFindIndicator(PassRefPtr<FindIndicator> findIndicator, bool fadeOut)
{
    if (m_findIndicator == findIndicator)
        return;

    m_findIndicator = findIndicator;

    // Get rid of the old window.
    closeWindow();

    if (!m_findIndicator)
        return;

    NSRect contentRect = m_findIndicator->frameRect();
    NSSize findIndicatorViewSize = contentRect.size;
    
    NSRect windowFrameRect = NSIntegralRect([m_wkView convertRect:contentRect toView:nil]);
    windowFrameRect.origin = [[m_wkView window] convertBaseToScreen:windowFrameRect.origin];

    NSRect windowContentRect = [NSWindow contentRectForFrameRect:windowFrameRect styleMask:NSBorderlessWindowMask];
    
    m_findIndicatorWindow.adoptNS([[NSWindow alloc] initWithContentRect:windowContentRect 
                                                              styleMask:NSBorderlessWindowMask 
                                                                backing:NSBackingStoreBuffered
                                                                  defer:NO]);

    [m_findIndicatorWindow.get() setBackgroundColor:[NSColor clearColor]];
    [m_findIndicatorWindow.get() setOpaque:NO];
    [m_findIndicatorWindow.get() setIgnoresMouseEvents:YES];

    RetainPtr<WebFindIndicatorView> findIndicatorView(AdoptNS, [[WebFindIndicatorView alloc] _initWithFindIndicator:m_findIndicator]);
    [m_findIndicatorWindow.get() setContentView:findIndicatorView.get()];

    [[m_wkView window] addChildWindow:m_findIndicatorWindow.get() ordered:NSWindowAbove];
    [m_findIndicatorWindow.get() setReleasedWhenClosed:NO];
}

void FindIndicatorWindow::closeWindow()
{
    if (!m_findIndicatorWindow)
        return;

    [[m_findIndicatorWindow.get() parentWindow] removeChildWindow:m_findIndicatorWindow.get()];
    [m_findIndicatorWindow.get() close];
    m_findIndicatorWindow.clear();
    
}

} // namespace WebKit
