/*	
    WebBaseResourceHandleDelegate.m
    Copyright (c) 2002, Apple Computer, Inc. All rights reserved.
*/

#import <WebFoundation/WebAssertions.h>
#import <WebFoundation/WebError.h>
#import <WebFoundation/WebHTTPResourceRequest.h>
#import <WebFoundation/WebResourceHandle.h>
#import <WebFoundation/WebResourceRequest.h>
#import <WebFoundation/WebResourceResponse.h>

#import <WebKit/WebBaseResourceHandleDelegate.h>
#import <WebKit/WebController.h>
#import <WebKit/WebDataSource.h>
#import <WebKit/WebResourceLoadDelegate.h>
#import <WebKit/WebStandardPanelsPrivate.h>


@implementation WebBaseResourceHandleDelegate

- init
{
    self = [super init];
    
    [self setIsDownload: NO];
    
    return self;
}

- (void)_releaseResources
{
    [identifier release];
    identifier = nil;

    [handle release];
    handle = nil;

    [dataSource release];
    dataSource = nil;
    
    [resourceLoadDelegate release];
    resourceLoadDelegate = nil;

    [downloadDelegate release];
    downloadDelegate = nil;
    
    reachedTerminalState = YES;
}

- (void)dealloc
{
    [self _releaseResources];
    [request release];
    [response release];
    [currentURL release];
}

- (void)setDataSource: (WebDataSource *)d
{
    if (d != dataSource){
        [dataSource release];
        dataSource = [d retain];
    }
    
    if (resourceLoadDelegate != [[dataSource controller] resourceLoadDelegate]){
        [resourceLoadDelegate release];
        resourceLoadDelegate = [[[dataSource controller] resourceLoadDelegate] retain];
    }

    if (downloadDelegate != [[dataSource controller] downloadDelegate]){
        [downloadDelegate release];
        downloadDelegate = [[[dataSource controller] downloadDelegate] retain];
    }    
}

- (WebDataSource *)dataSource
{
    return dataSource;
}

- (id <WebResourceLoadDelegate>)resourceLoadDelegate
{
    return resourceLoadDelegate;
}

- (id <WebResourceLoadDelegate>)downloadDelegate
{
    return downloadDelegate;
}

- (void)setIsDownload: (BOOL)f
{
    isDownload = f;
}

- (BOOL)isDownload
{
    return isDownload;
}

-(WebResourceRequest *)handle:(WebResourceHandle *)h willSendRequest:(WebResourceRequest *)newRequest
{
    ASSERT (!reachedTerminalState);

    if (!handle){
        // Retained so we can cancel if necessary.  Released when we
        // reach a terminal state.
        handle = [h retain];
    }
        
    [newRequest setUserAgent:[[dataSource controller] userAgentForURL:[newRequest URL]]];

    [newRequest retain];
    [request release];
    request = newRequest;

    if (identifier == nil){
        // The identifier is released after the last callback, rather than in dealloc
        // to avoid potential cycles.
        identifier = [[resourceLoadDelegate identifierForInitialRequest: newRequest fromDataSource: dataSource] retain];
    }

    if (resourceLoadDelegate)
        newRequest = [resourceLoadDelegate resource: identifier willSendRequest: newRequest fromDataSource: dataSource];

    [[WebStandardPanels sharedStandardPanels] _didStopLoadingURL:currentURL inController: [dataSource controller]];
    
    if ([newRequest URL] != currentURL){
        [currentURL release];
        currentURL = [[newRequest URL] retain];
    }
    
    [[WebStandardPanels sharedStandardPanels] _didStartLoadingURL:currentURL inController:[dataSource controller]];

    return newRequest;
}

-(void)handle:(WebResourceHandle *)h didReceiveResponse:(WebResourceResponse *)r
{
    ASSERT (handle == h);
    ASSERT (!reachedTerminalState);

    [r retain];
    [response release];
    response = r;

    if (isDownload)
        [downloadDelegate resource:identifier didReceiveResponse:r fromDataSource: dataSource];
    else
        [resourceLoadDelegate resource:identifier didReceiveResponse:r fromDataSource: dataSource];
}

- (void)handle:(WebResourceHandle *)h didReceiveData:(NSData *)data
{
    ASSERT (handle == h);
    ASSERT (!reachedTerminalState);

    if ([self isDownload])
        [downloadDelegate resource: identifier didReceiveContentLength: [data length] fromDataSource: dataSource];
    else
        [resourceLoadDelegate resource: identifier didReceiveContentLength: [data length] fromDataSource: dataSource];
}

- (void)handleDidFinishLoading:(WebResourceHandle *)h
{
    ASSERT (handle == h);
    ASSERT (!reachedTerminalState);

    if ([self isDownload])
        [downloadDelegate resource:identifier didFinishLoadingFromDataSource:dataSource];
    else
        [resourceLoadDelegate resource:identifier didFinishLoadingFromDataSource:dataSource];

    [[WebStandardPanels sharedStandardPanels] _didStopLoadingURL:currentURL inController:[dataSource controller]];

    [self _releaseResources];
}

- (void)handle:(WebResourceHandle *)h didFailLoadingWithError:(WebError *)result
{
    ASSERT (handle == h);
    ASSERT (!reachedTerminalState);
    
    if ([self isDownload])
        [downloadDelegate resource: identifier didFailLoadingWithError: result fromDataSource: dataSource];
    else
        [resourceLoadDelegate resource: identifier didFailLoadingWithError: result fromDataSource: dataSource];

    [[WebStandardPanels sharedStandardPanels] _didStopLoadingURL:currentURL inController:[dataSource controller]];

    [self _releaseResources];
}

- (void)cancel
{
    ASSERT (!reachedTerminalState);

    [handle cancel];
    
    [[WebStandardPanels sharedStandardPanels] _didStopLoadingURL:currentURL inController:[dataSource controller]];

    WebError *error = [[WebError alloc] initWithErrorCode:WebErrorCodeCancelled 
        inDomain:WebErrorDomainWebFoundation failingURL:[[request URL] absoluteString]];
    if (![self isDownload])
        [resourceLoadDelegate resource: identifier didFailLoadingWithError: error fromDataSource: dataSource];

    [self _releaseResources];
}

- (WebResourceHandle *)handle
{
    return handle;
}

@end
