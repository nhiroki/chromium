/*
        WebNSViewExtras.mm
        Copyright (c) 2002, Apple, Inc. All rights reserved.
*/

#import <WebKit/WebNSViewExtras.h>
#import <WebKit/WebView.h>

#ifdef DEBUG_VIEWS
@interface NSObject (Foo)
- (void*)_renderFramePart;
- (id)frameForView: (id)aView;
- (id)_controller;
@end
#endif

@implementation NSView (WebExtras)

- (NSView *) _web_superviewWithName:(NSString *)viewName
{
    NSView *view;
    
    view = self;
    while(view){
        view = [view superview];
        if([[view className] isEqualToString:viewName]){
            return view;
        }
    }
    return nil;
}

- (WebView *)_web_parentWebView
{
    WebView *view = (WebView *)[[[self superview] superview] superview];
    
    if ([view isKindOfClass: [WebView class]])
        return view;
    return nil;
}

#ifdef DEBUG_VIEWS
- (void)_web_printViewHierarchy: (int)level
{
    NSArray *subviews;
    int _level = level, i;
    NSRect f;
    NSView *subview;
    void *rfp = 0;
    
    subviews = [self subviews];
    _level = level;
    while (_level-- > 0)
        printf (" ");
    f = [self frame];
    
    if ([self respondsToSelector: @selector(_controller)]){
        id aController = [self _controller];
        id aFrame = [aController frameForView: self];
        rfp = [aFrame _renderFramePart];
    }
    
    printf ("%s renderFramePart %p (%f,%f) w %f, h %f\n", [[[self class] className] cString], rfp, f.origin.x, f.origin.y, f.size.width, f.size.height);
    for (i = 0; i < (int)[subviews count]; i++){
        subview = [subviews objectAtIndex: i];
        [subview _web_printViewHierarchy: level + 1];
    }
}
#endif

@end
