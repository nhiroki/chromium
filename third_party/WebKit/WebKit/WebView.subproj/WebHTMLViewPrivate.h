/*	WebHTMLViewPrivate.h
	Copyright 2002, Apple, Inc. All rights reserved.
        
        Private header file.  This file may reference classes (both ObjectiveC and C++)
        in WebCore.  Instances of this class are referenced by _private in 
        WebHTMLView.
*/

#import <WebKit/WebHTMLView.h>

@class WebBridge;

@interface WebHTMLViewPrivate : NSObject
{
@public
    WebController *controller;
    BOOL needsLayout;
    BOOL needsToApplyStyles;
    BOOL canDragTo;
    BOOL canDragFrom;
    NSCursor *cursor;
    BOOL liveAllowsScrolling;
    BOOL inWindow;
}
@end

@interface WebHTMLView (WebPrivate)
- (void)_reset;
- (void)_setController: (WebController *)controller;
- (WebBridge *)_bridge;
- (void)_adjustFrames;
@end
