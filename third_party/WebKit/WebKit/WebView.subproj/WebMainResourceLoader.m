/*	
    IFMainURLHandleClient.mm
	    
    Copyright 2001, Apple, Inc. All rights reserved.
*/

#import <WebKit/IFDocument.h>
#import <WebKit/IFHTMLRepresentation.h>
#import <WebKit/IFLoadProgress.h>
#import <WebKit/IFLocationChangeHandler.h>
#import <WebKit/IFMainURLHandleClient.h>
#import <WebKit/IFWebController.h>
#import <WebKit/IFWebControllerPrivate.h>
#import <WebKit/IFWebDataSource.h>
#import <WebKit/IFWebDataSourcePrivate.h>
#import <WebKit/IFWebFrame.h>
#import <WebKit/IFWebFramePrivate.h>
#import <WebKit/IFWebView.h>
#import <WebKit/WebKitDebug.h>

#import <KWQKHTMLPartImpl.h>

#import <WebFoundation/IFError.h>



@implementation IFMainURLHandleClient

- initWithDataSource: (IFWebDataSource *)ds
{
    if ((self = [super init])) {
        dataSource = [ds retain];
        examinedInitialData = NO;
        processedBufferedData = NO;
        isFirstChunk = YES;
        return self;
    }

    return nil;
}

- (void)dealloc
{
    WEBKIT_ASSERT(url == nil);
    
    [dataSource release];
    [super dealloc];
}

- (void)IFURLHandleResourceDidBeginLoading:(IFURLHandle *)sender
{
    WEBKITDEBUGLEVEL (WEBKIT_LOG_LOADING, "url = %s\n", DEBUG_OBJECT([sender url]));
    
    WEBKIT_ASSERT(url == nil);
    
    url = [[sender url] retain];
    [(IFWebController *)[dataSource controller] _didStartLoading:url];
}


- (void)IFURLHandleResourceDidCancelLoading:(IFURLHandle *)sender
{
    WEBKITDEBUGLEVEL (WEBKIT_LOG_LOADING, "url = %s\n", DEBUG_OBJECT([sender url]));
    
    WEBKIT_ASSERT([url isEqual:[sender redirectedURL] ? [sender redirectedURL] : [sender url]]);
    
    IFLoadProgress *loadProgress = [[IFLoadProgress alloc] init];
    loadProgress->totalToLoad = -1;
    loadProgress->bytesSoFar = -1;
    [(IFWebController *)[dataSource controller] _mainReceivedProgress: (IFLoadProgress *)loadProgress 
        forResource: [[sender url] absoluteString] fromDataSource: dataSource];
    [loadProgress release];
    [(IFWebController *)[dataSource controller] _didStopLoading:url];
    [url release];
    url = nil;
}


- (void)IFURLHandleResourceDidFinishLoading:(IFURLHandle *)sender data: (NSData *)data
{
    WEBKITDEBUGLEVEL (WEBKIT_LOG_LOADING, "url = %s\n", DEBUG_OBJECT([sender url]));
    
    WEBKIT_ASSERT([url isEqual:[sender redirectedURL] ? [sender redirectedURL] : [sender url]]);
    
    [dataSource _setResourceData:data];
    
    // update progress
    IFLoadProgress *loadProgress = [[IFLoadProgress alloc] init];
    loadProgress->totalToLoad = [data length];
    loadProgress->bytesSoFar = [data length];
    [(IFWebController *)[dataSource controller] _mainReceivedProgress: (IFLoadProgress *)loadProgress 
        forResource: [[sender url] absoluteString] fromDataSource: dataSource];
    [loadProgress release];
    [(IFWebController *)[dataSource controller] _didStopLoading:url];
    [url release];
    url = nil;
}


- (void)IFURLHandle:(IFURLHandle *)sender resourceDataDidBecomeAvailable:(NSData *)data
{
    int contentLength = [sender contentLength];
    int contentLengthReceived = [sender contentLengthReceived];
    BOOL isComplete = (contentLength == contentLengthReceived);
    NSString *contentType = [sender contentType];
    IFWebFrame *frame = [dataSource webFrame];
    IFWebView *view = [frame view];
    IFContentPolicy contentPolicy;
    
    WEBKITDEBUGLEVEL(WEBKIT_LOG_LOADING, "url = %s, data = %p, length %d\n", DEBUG_OBJECT([sender url]), data, [data length]);
    
    WEBKIT_ASSERT([url isEqual:[sender redirectedURL] ? [sender redirectedURL] : [sender url]]);
    
    // Check the mime type and ask the client for the content policy.
    if(!examinedInitialData){
        WEBKITDEBUGLEVEL(WEBKIT_LOG_DOWNLOAD, "main content type: %s", DEBUG_OBJECT(contentType));
        [dataSource _setContentType:contentType];
        [dataSource _setEncoding:[sender characterSet]];
        [[dataSource _locationChangeHandler] requestContentPolicyForMIMEType:contentType];
        examinedInitialData = YES;
    }
    
    contentPolicy = [dataSource contentPolicy];
    if(contentPolicy == IFContentPolicyIgnore){
        [sender cancelLoadInBackground];
        return;
    }
    
    if(contentPolicy != IFContentPolicyNone){
        if(!processedBufferedData){
            // Process all data that has been received now that we have a content policy
            // and don't call resourceData if this is the first chunk since resourceData is a copy
            if(isFirstChunk){
                [[dataSource representation] receivedData:data withDataSource:dataSource isComplete:isComplete];
            }else{
                [[dataSource representation] receivedData:[sender resourceData] 
                    withDataSource:dataSource isComplete:isComplete];
            }
            processedBufferedData = YES;          
        }else{
            [[dataSource representation] receivedData:data withDataSource:dataSource isComplete:isComplete];
        }
        [[view documentView] dataSourceUpdated:dataSource];
    }
    
    WEBKITDEBUGLEVEL(WEBKIT_LOG_DOWNLOAD, "%d of %d", contentLengthReceived, contentLength);
    
    // Don't send the last progress message, it will be sent via
    // IFURLHandleResourceDidFinishLoading
    if (contentLength == contentLengthReceived &&
    	contentLength != -1){
    	return;
    }
    
    // update progress
    IFLoadProgress *loadProgress = [[IFLoadProgress alloc] init];
    loadProgress->totalToLoad = contentLength;
    loadProgress->bytesSoFar = contentLengthReceived;
    [(IFWebController *)[dataSource controller] _mainReceivedProgress: (IFLoadProgress *)loadProgress 
        forResource: [[sender url] absoluteString] fromDataSource: dataSource];
    [loadProgress release];
    
    isFirstChunk = NO;
}


- (void)IFURLHandle:(IFURLHandle *)sender resourceDidFailLoadingWithResult:(IFError *)result
{
    WEBKITDEBUGLEVEL (WEBKIT_LOG_LOADING, "url = %s, result = %s\n", DEBUG_OBJECT([sender url]), DEBUG_OBJECT([result errorDescription]));

    WEBKIT_ASSERT([url isEqual:[sender redirectedURL] ? [sender redirectedURL] : [sender url]]);

    IFLoadProgress *loadProgress = [[IFLoadProgress alloc] init];
    loadProgress->totalToLoad = [sender contentLength];
    loadProgress->bytesSoFar = [sender contentLengthReceived];

    [(IFWebController *)[dataSource controller] _mainReceivedError:result forResource:[[sender url] absoluteString] partialProgress:loadProgress fromDataSource:dataSource];
    [(IFWebController *)[dataSource controller] _didStopLoading:url];
    [url release];
    url = nil;
}


- (void)IFURLHandle:(IFURLHandle *)sender didRedirectToURL:(NSURL *)newURL
{
    WEBKITDEBUGLEVEL (WEBKIT_LOG_REDIRECT, "url = %s\n", DEBUG_OBJECT(newURL));
    
    WEBKIT_ASSERT(url != nil);
    
    [(IFWebController *)[dataSource controller] _didStopLoading:url];
    [newURL retain];
    [url release];
    url = newURL;
    [(IFWebController *)[dataSource controller] _didStartLoading:url];

    if([dataSource _isDocumentHTML]) 
        [[dataSource representation] part]->impl->setBaseURL([[url absoluteString] cString]);
    [dataSource _setFinalURL:url];
    
    [[dataSource _locationChangeHandler] serverRedirectTo:url forDataSource:dataSource];
}

@end
