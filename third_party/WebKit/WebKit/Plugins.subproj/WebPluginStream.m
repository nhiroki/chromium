/*	
        WebPluginStream.m
	Copyright (c) 2002, Apple, Inc. All rights reserved.
*/

#import <WebKit/WebBaseNetscapePluginView.h>
#import <WebKit/WebControllerPrivate.h>
#import <WebKit/WebDataSource.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebFrame.h>
#import <WebKit/WebKitLogging.h>
#import <WebKit/WebPlugin.h>
#import <WebKit/WebPluginStream.h>
#import <WebKit/WebView.h>

#import <WebFoundation/WebError.h>
#import <WebFoundation/WebNSFileManagerExtras.h>
#import <WebFoundation/WebResourceHandle.h>
#import <WebFoundation/WebResourceRequest.h>
#import <WebFoundation/WebResourceResponse.h>
#import <WebFoundation/WebHTTPResourceResponse.h>

@interface WebNetscapePluginStream (ClassInternal)
- (void)receivedData:(NSData *)data;
- (void)receivedError:(NPError)error;
- (void)finishedLoadingWithData:(NSData *)data;
- (void)setResponse:(WebResourceResponse *)theReponse;
- (void)cancel;
@end

@interface WebNetscapePluginStream (WebResourceHandleDelegate) <WebResourceHandleDelegate>
@end

@implementation WebNetscapePluginStream

- (void) getFunctionPointersFromPluginView:(WebBaseNetscapePluginView *)pluginView
{
    ASSERT(pluginView);
    
    WebNetscapePlugin *plugin = [pluginView plugin];
    
    NPP_NewStream = 	[plugin NPP_NewStream];
    NPP_WriteReady = 	[plugin NPP_WriteReady];
    NPP_Write = 	[plugin NPP_Write];
    NPP_StreamAsFile = 	[plugin NPP_StreamAsFile];
    NPP_DestroyStream = [plugin NPP_DestroyStream];
    NPP_URLNotify = 	[plugin NPP_URLNotify];
}

- init
{
    [super init];

    isFirstChunk = YES;
    
    return self;
}

- initWithURL:(NSURL *)theURL pluginPointer:(NPP)thePluginPointer notifyData:(void *)theNotifyData
{
    [super init];

    if(!theURL || !thePluginPointer){
       return nil;
    }

    request = [[WebResourceRequest alloc] initWithURL:theURL];
    if(![WebResourceHandle canInitWithRequest:request]){
        [request release];
        return nil;
    }
       
    view = [(WebBaseNetscapePluginView *)thePluginPointer->ndata retain];
    ASSERT(view);
    URL = [theURL retain];
    instance = thePluginPointer;
    notifyData = theNotifyData;
    resourceData = [[NSMutableData alloc] init];

    [self getFunctionPointersFromPluginView:view];
    
    isFirstChunk = YES;
    
    return self;
}

- (void) dealloc
{
    [self stop];
    
    if(path){
        [[NSFileManager defaultManager] removeFileAtPath:path handler:nil];
        [path release];
    }
    free((void *)npStream.URL);
    [URL release];
    [resourceData release];
    [request release];
    [super dealloc];
}

- (void)startLoad
{
    resource = [[WebResourceHandle alloc] initWithRequest:request];
    [resource loadWithDelegate:self];
    [[view controller] _didStartLoading:[request URL]];
}

- (void)stop
{
    [self cancel];
    [resource release];
    resource = nil;
    [view release];
    view = nil;
}

- (void)receivedData:(NSData *)data
{    
    if(isFirstChunk){

        NSString *mimeType = [response contentType];
        NSString *URLString = [[response URL] absoluteString];
        char *cURL = (char *)malloc([URLString cStringLength]+1);
        [URLString getCString:cURL];

        NSNumber *timeInterval = nil;
        uint32 lastModified = 0;
        
        if ([response isKindOfClass:[WebHTTPResourceResponse class]]) {
            timeInterval = [[(WebHTTPResourceResponse *)response headers] objectForKey:@"Last-Modified"];
            if(timeInterval){
                NSTimeInterval lastModifiedInterval;
                lastModifiedInterval = [[NSDate dateWithTimeIntervalSinceReferenceDate:[timeInterval doubleValue]] timeIntervalSince1970];
                if(lastModifiedInterval > 0){
                    lastModified = (uint32)lastModifiedInterval;
                }
            }
        }
        
        npStream.ndata = self;
        npStream.URL = cURL;
        npStream.end = [response contentLength];
        npStream.lastmodified = lastModified;
        npStream.notifyData = notifyData;
        
        offset = 0;
        
        // FIXME: Need a way to check if stream is seekable
        
        NPError npErr = NPP_NewStream(instance, (char *)[mimeType cString], &npStream, NO, &transferMode);
        LOG(Plugins, "NPP_NewStream: %d %s", npErr, [[URL absoluteString] cString]);
        
        if(npErr != NPERR_NO_ERROR){
            [self stop];
            return;
        }
        
        if(transferMode == NP_NORMAL)
            LOG(Plugins, "Stream type: NP_NORMAL");
        else if(transferMode == NP_ASFILEONLY)
            LOG(Plugins, "Stream type: NP_ASFILEONLY");
        else if(transferMode == NP_ASFILE)
            LOG(Plugins, "Stream type: NP_ASFILE");
        else if(transferMode == NP_SEEK){
            LOG(Plugins, "Stream type: NP_SEEK not yet supported");
            [self stop];
            return;
        }
        
        isFirstChunk = NO;
    }

    if(transferMode == NP_ASFILE || transferMode == NP_ASFILEONLY) {
        // only need to buffer data in this case
        [resourceData appendData:data];
    }

    if(transferMode != NP_ASFILEONLY){
        int32 numBytes = NPP_WriteReady(instance, &npStream);
        LOG(Plugins, "NPP_WriteReady bytes=%lu", numBytes);
        
        numBytes = NPP_Write(instance, &npStream, offset, [data length], (void *)[data bytes]);
        LOG(Plugins, "NPP_Write bytes=%lu", numBytes);
        
        offset += [data length];
    }
}

- (void)receivedError:(NPError)error
{
    // Don't report error before we've called NPP_NewStream
    if(!isFirstChunk){
#if !LOG_DISABLED
        NPError npErr =
#endif
        NPP_DestroyStream(instance, &npStream, error);
        LOG(Plugins, "NPP_DestroyStream: %d", npErr);
    }
}

- (void)finishedLoadingWithData:(NSData *)data
{
    NPError npErr;
    NSFileManager *fileManager;
    NSString *filename, *carbonPath;
    
    filename = [[URL path] lastPathComponent];
    if(transferMode == NP_ASFILE || transferMode == NP_ASFILEONLY) {
        // FIXME: Need to use something like mkstemp?
        path = [[NSString stringWithFormat:@"/tmp/%@", filename] retain];        
        fileManager = [NSFileManager defaultManager];
        [fileManager removeFileAtPath:path handler:nil];
        [fileManager createFileAtPath:path contents:data attributes:nil];
        
        // FIXME: Will cString use the correct character set?
        carbonPath = [[NSFileManager defaultManager] _web_carbonPathForPath:path];
        NPP_StreamAsFile(instance, &npStream, [carbonPath cString]);
        LOG(Plugins, "NPP_StreamAsFile: %s", [carbonPath cString]);
    }
    npErr = NPP_DestroyStream(instance, &npStream, NPRES_DONE);
    LOG(Plugins, "NPP_DestroyStream: %d", npErr);
    
    if(notifyData){
        NPP_URLNotify(instance, [[URL absoluteString] cString], NPRES_DONE, notifyData);
        LOG(Plugins, "NPP_URLNotify");
    }
    
    [self stop];
}

- (void)setResponse:(WebResourceResponse *)theResponse
{
    [response release];
    response = [theResponse retain];
}

#pragma mark WebDocumentRepresentation

- (void)setDataSource:(WebDataSource *)dataSource
{
    [self setResponse:[dataSource response]];
}

- (void)receivedData:(NSData *)data withDataSource:(WebDataSource *)dataSource
{
    ASSERT(dataSource);
    ASSERT([dataSource webFrame]);
    ASSERT([[dataSource webFrame] webView]);
    ASSERT([[[dataSource webFrame] webView] documentView]);
    ASSERT([(WebBaseNetscapePluginView *)[[[dataSource webFrame] webView] documentView] pluginInstance]);
    
    if(isFirstChunk){
        WebFrame *frame = [dataSource webFrame];
        WebView *webView = [frame webView];
        view = [[webView documentView] retain];
        instance = [view pluginInstance];

        [self getFunctionPointersFromPluginView:view];
    }

    [self receivedData:data];
}

- (void)receivedError:(WebError *)error withDataSource:(WebDataSource *)dataSource
{
    if([error errorCode] == WebErrorCodeCancelled){
        [self receivedError:NPRES_USER_BREAK];
    } else {
        [self receivedError:NPRES_NETWORK_ERR];
    }
}

- (void)finishedLoadingWithDataSource:(WebDataSource *)dataSource
{
    [self finishedLoadingWithData:[dataSource data]];
}

@end

#pragma mark WebResourceHandle

@implementation WebNetscapePluginStream (WebResourceHandleDelegate)

-(WebResourceRequest *)handle:(WebResourceHandle *)handle willSendRequest:(WebResourceRequest *)theRequest
{
    WebController *webController = [view controller];
    
    [webController _didStopLoading:URL];
    // FIXME: This next line is not sufficient. We don't do anything to remember the new URL.
    [webController _didStartLoading:[theRequest URL]];
    
    return theRequest;
}

- (void)handle:(WebResourceHandle *)handle didReceiveResponse:(WebResourceResponse *)theResponse
{
    [self setResponse:theResponse];
}

- (void)handle:(WebResourceHandle *)handle didReceiveData:(NSData *)data
{
    ASSERT(resource == handle);

    [self receivedData:data];
    
    [[view controller] _receivedProgressForResourceHandle: handle fromDataSource: [view dataSource] complete: NO];
}

- (void)handleDidFinishLoading:(WebResourceHandle *)handle
{
    ASSERT(resource == handle);

    [resource release];    
    resource = nil;

    WebController *controller = [view controller];
    
    [controller _receivedProgressForResourceHandle: handle fromDataSource: [view dataSource] complete: YES];
 
    [self finishedLoadingWithData:resourceData];
          
    [controller _didStopLoading:URL];
}

- (void)cancel
{
    if (resource == nil) {
        return;
    }
    
    [resource cancel];
    
    WebController *controller = [view controller];
    
    WebError *cancelError = [[WebError alloc] initWithErrorCode:WebErrorCodeCancelled
                                                       inDomain:WebErrorDomainWebFoundation
                                                     failingURL:nil];
    [controller _receivedError: cancelError forResourceHandle: resource 
        fromDataSource: [view dataSource]];
    
    [cancelError release];

    [self receivedError:NPRES_USER_BREAK];
    
    [controller _didStopLoading:URL];

    [resource release];
    resource = nil;
}

- (void)handle:(WebResourceHandle *)handle didFailLoadingWithError:(WebError *)result
{
    ASSERT(resource == handle);
    
    [resource release];
    resource = nil;
    
    WebController *controller = [view controller];
        
    [controller _receivedError: result forResourceHandle: handle 
        fromDataSource: [view dataSource]];

    [self receivedError:NPRES_NETWORK_ERR];
    
    [controller _didStopLoading:URL];
}

@end
