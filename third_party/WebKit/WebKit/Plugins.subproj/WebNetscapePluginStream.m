/*
        WebNetscapePluginStream.h
	Copyright 2002, Apple, Inc. All rights reserved.
*/

#import <WebKit/WebNetscapePluginStream.h>

#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebKitLogging.h>
#import <WebKit/WebNetscapePluginEmbeddedView.h>
#import <WebKit/WebViewPrivate.h>

#import <Foundation/NSURLRequest.h>
#import <Foundation/NSURLConnection.h>

@interface WebNetscapePluginConnectionDelegate : WebBaseResourceHandleDelegate
{
    WebNetscapePluginStream *stream;
    WebBaseNetscapePluginView *view;
    NSMutableData *resourceData;
}
- initWithStream:(WebNetscapePluginStream *)theStream view:(WebBaseNetscapePluginView *)theView;
@end

@implementation WebNetscapePluginStream

- initWithRequest:(NSURLRequest *)theRequest
    pluginPointer:(NPP)thePluginPointer
       notifyData:(void *)theNotifyData
{
    [super init];

    if (!theRequest || !thePluginPointer || ![WebView _canHandleRequest:theRequest]) {
        [self release];
        return nil;
    }

    _startingRequest = [theRequest copy];

    [self setPluginPointer:thePluginPointer];

    WebBaseNetscapePluginView *view = (WebBaseNetscapePluginView *)instance->ndata;
    _loader = [[WebNetscapePluginConnectionDelegate alloc] initWithStream:self view:view]; 
    [_loader setDataSource:[view dataSource]];

    notifyData = theNotifyData;

    return self;
}

- (void)dealloc
{
    [_loader release];
    [_startingRequest release];
    [super dealloc];
}

- (void)start
{
    ASSERT(_startingRequest);
    [_loader loadWithRequest:_startingRequest];
    [_startingRequest release];
    _startingRequest = nil;
}

- (void)stop
{
    [_loader cancel];
}

@end

@implementation WebNetscapePluginConnectionDelegate

- initWithStream:(WebNetscapePluginStream *)theStream view:(WebBaseNetscapePluginView *)theView
{
    [super init];
    stream = [theStream retain];
    view = [theView retain];
    resourceData = [[NSMutableData alloc] init];
    return self;
}

- (void)releaseResources
{
    [stream release];
    stream = nil;
    [view release];
    view = nil;
    [resourceData release];
    resourceData = nil;
    [super releaseResources];
}

- (void)connection:(NSURLConnection *)con didReceiveResponse:(NSURLResponse *)theResponse
{
    [stream setResponse:theResponse];
    [super connection:con didReceiveResponse:theResponse];
}

- (void)connection:(NSURLConnection *)con didReceiveData:(NSData *)data
{
    if ([stream transferMode] == NP_ASFILE || [stream transferMode] == NP_ASFILEONLY) {
        [resourceData appendData:data];
    }

    [stream receivedData:data];
    [super connection:con didReceiveData:data];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)con
{
    [[view webView] _finishedLoadingResourceFromDataSource:[view dataSource]];
    [stream finishedLoadingWithData:resourceData];
    [super connectionDidFinishLoading:con];
}

- (void)connection:(NSURLConnection *)con didFailWithError:(NSError *)result
{
    [[view webView] _receivedError:result fromDataSource:[view dataSource]];
    [stream receivedError:NPRES_NETWORK_ERR];
    [super connection:con didFailWithError:result];
}

- (void)cancel
{
    // Since the plug-in is notified of the stream when the response is received,
    // only report an error if the response has been received.
    if ([self response]) {
        [stream receivedError:NPRES_USER_BREAK];
    }

    [super cancel];
}

@end
