/*	
        WebFramePrivate.h
	    
	    Copyright 2001, Apple, Inc. All rights reserved.

        Private header file.
*/

#import <WebKit/WebFrame.h>

@class WebBridge;
@class WebFrameBridge;
@class WebView;
@protocol WebDocumentLoading;

typedef enum {
    WebFrameStateUninitialized = 1,
    WebFrameStateProvisional = 2,
    
    // This state indicates we are ready to commit to a page,
    // that means the view will transition to use the new
    // datasource.
    WebFrameStateCommittedPage = 3,
    
    // This state indicates that it is reasonable to perform
    // a layout.
    WebFrameStateLayoutAcceptable = 4,
    
    WebFrameStateComplete = 5
} WebFrameState;

typedef enum {
    WebFrameLoadTypeUninitialized = 0,
    WebFrameLoadTypeStandard = 1,
    WebFrameLoadTypeBack = 2,
    WebFrameLoadTypeForward = 3,
    WebFrameLoadTypeIndexedBack = 4,
    WebFrameLoadTypeIndexedForward = 5,
    WebFrameLoadTypeRefresh = 6,
    WebFrameLoadTypeInternal = 7
} WebFrameLoadType;

#define WebFrameStateChangedNotification @"WebFrameStateChangedNotification"

#define WebPreviousFrameState @"WebPreviousFrameState"
#define WebCurrentFrameState  @"WebCurrentFrameState"

@interface WebFramePrivate : NSObject
{
@public
    NSString *name;
    WebView *webView;
    WebDataSource *dataSource;
    WebDataSource *provisionalDataSource;
    WebBridge *bridge;
    WebController *controller;
    WebFrameState state;
    NSTimer *scheduledLayoutTimer;
    WebFrameLoadType loadType;
}

- (void)setName: (NSString *)n;
- (NSString *)name;
- (void)setController: (WebController *)c;
- (WebController *)controller;
- (void)setWebView: (WebView *)v;
- (WebView *)webView;
- (void)setDataSource: (WebDataSource *)d;
- (WebDataSource *)dataSource;
- (void)setProvisionalDataSource: (WebDataSource *)d;
- (WebDataSource *)provisionalDataSource;
- (WebFrameLoadType)loadType;
- (void)setLoadType: (WebFrameLoadType)loadType;

@end

@interface WebFrame (WebPrivate)
- (void)_parentDataSourceWillBeDeallocated;
- (void)_setController: (WebController *)controller;
- (void)_setDataSource: (WebDataSource *)d;
- (void)_transitionToCommitted;
- (void)_transitionToLayoutAcceptable;
- (WebFrameState)_state;
- (void)_setState: (WebFrameState)newState;
+ (void)_recursiveCheckCompleteFromFrame: (WebFrame *)fromFrame;
- (void)_isLoadComplete;
- (void)_checkLoadComplete;
- (void)_timedLayout: userInfo;
- (void)_changeBridge;
- (WebBridge *)_bridge;
- (BOOL)_shouldShowDataSource:(WebDataSource *)dataSource;
- (void)_setProvisionalDataSource:(WebDataSource *)d;
- (void)_setLoadType: (WebFrameLoadType)loadType;
- (WebFrameLoadType)_loadType;
- (void)_goToURL: (NSURL *)url withFrameLoadType: (WebFrameLoadType)type;
@end
