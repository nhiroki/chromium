/*	
    IFWebFramePrivate.mm
	    
    Copyright 2001, Apple, Inc. All rights reserved.
*/
#import <WebKit/IFWebDataSource.h>
#import <WebKit/IFWebDataSourcePrivate.h>
#import <WebKit/IFWebViewPrivate.h>
#import <WebKit/IFWebFramePrivate.h>
#import <WebKit/IFError.h>

#import <WebKit/WebKitDebug.h>

// includes from kde
#include <khtmlview.h>
#include <rendering/render_frames.h>

@implementation IFWebFramePrivate

- (void)dealloc
{
    [name autorelease];
    [view autorelease];
    [dataSource autorelease];
    [errors release];
    [mainDocumentError release];
    [super dealloc];
}

- (NSString *)name { return name; }
- (void)setName: (NSString *)n 
{
    [name autorelease];
    name = [n retain];
}


- view { return view; }
- (void)setView: v 
{ 
    [view autorelease];
    view = [v retain];
}

- (IFWebDataSource *)dataSource { return dataSource; }
- (void)setDataSource: (IFWebDataSource *)d
{ 
    [dataSource autorelease];
    dataSource = [d retain];
}


- (id <IFWebController>)controller { return controller; }
- (void)setController: (id <IFWebController>)c
{ 
    // Warning:  non-retained reference
    controller = c;
}


- (IFWebDataSource *)provisionalDataSource { return provisionalDataSource; }
- (void)setProvisionalDataSource: (IFWebDataSource *)d
{ 
    [provisionalDataSource autorelease];
    provisionalDataSource = [d retain];
}


- (void *)renderFramePart { return renderFramePart; }
- (void)setRenderFramePart: (void *)p 
{
    renderFramePart = p;
}


@end


@implementation IFWebFrame (IFPrivate)


// renderFramePart is a pointer to a RenderPart
- (void)_setRenderFramePart: (void *)p
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;
    [data setRenderFramePart: p];
}

- (void *)_renderFramePart
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;
    return [data renderFramePart];
}

- (void)_setDataSource: (IFWebDataSource *)ds
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;
    [data setDataSource: ds];
    [ds _setController: [self controller]];
}


- (void)_transitionProvisionalToCommitted
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;

    WEBKIT_ASSERT ([self controller] != nil);

    switch ([self _state]){
    	case IFWEBFRAMESTATE_PROVISIONAL:
        {
            id view = [self view];

            WEBKIT_ASSERT (view != nil);

            // Make sure any plugsin are shut down cleanly.
            [view _stopPlugins];
            
            // Remove any widgets.
            [view _removeSubviews];
            
            // Set the committed data source on the frame.
            [self _setDataSource: data->provisionalDataSource];
            
            // If we're a frame (not the main frame) hookup the kde internals.  This introduces a nasty dependency 
            // in kde on the view.
            khtml::RenderPart *renderPartFrame = [self _renderFramePart];
            if (renderPartFrame && [view isKindOfClass: NSClassFromString(@"IFWebView")]){
                // Setting the widget will delete the previous KHTMLView associated with the frame.
                renderPartFrame->setWidget ([view _provisionalWidget]);
            }
        
            // dataSourceChanged: will reset the view and begin trying to
            // display the new new datasource.
            [view dataSourceChanged: data->provisionalDataSource];
        
            
            // Now that the provisional data source is committed, release it.
            [data setProvisionalDataSource: nil];
        
            [self _setState: IFWEBFRAMESTATE_COMMITTED];
        
            [[self controller] locationChangeCommittedForFrame: self];
            
            break;
        }
        
        case IFWEBFRAMESTATE_UNINITIALIZED:
        case IFWEBFRAMESTATE_COMMITTED:
        case IFWEBFRAMESTATE_COMPLETE:
        default:
        {
            [[NSException exceptionWithName:NSGenericException reason:@"invalid state attempting to transition to IFWEBFRAMESTATE_COMMITTED" userInfo: nil] raise];
            return;
        }
    }
}

- (IFWebFrameState)_state
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;
    
    return data->state;
}

char *stateNames[5] = {
    "zero state",
    "IFWEBFRAMESTATE_UNINITIALIZED",
    "IFWEBFRAMESTATE_PROVISIONAL",
    "IFWEBFRAMESTATE_COMMITTED",
    "IFWEBFRAMESTATE_COMPLETE" };

- (void)_setState: (IFWebFrameState)newState
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;

    WEBKITDEBUGLEVEL3 (WEBKIT_LOG_LOADING, "%s:  transition from %s to %s\n", [[self name] cString], stateNames[data->state], stateNames[newState]);
    
    data->state = newState;
}

- (void)_addError: (IFError *)error forResource: (NSString *)resourceDescription
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;
    if (data->errors == 0)
        data->errors = [[NSMutableDictionary alloc] init];
        
    [data->errors setObject: error forKey: resourceDescription];
}

- (void)_isLoadComplete
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;
    
    WEBKIT_ASSERT ([self controller] != nil);

    switch ([self _state]){
        // Shouldn't ever be in this state.
        case IFWEBFRAMESTATE_UNINITIALIZED:
        {
            [[NSException exceptionWithName:NSGenericException reason:@"invalid state IFWEBFRAMESTATE_UNINITIALIZED during completion check with error" userInfo: nil] raise];
            return;
        }
        
        case IFWEBFRAMESTATE_PROVISIONAL:
        {
            WEBKITDEBUGLEVEL1 (WEBKIT_LOG_LOADING, "%s:  checking complete in IFWEBFRAMESTATE_PROVISIONAL\n", [[self name] cString]);
            // If we've received any errors we may be stuck in the provisional state and actually
            // complete.
            if ([[self errors] count] != 0){
                // Check all children first.
                WEBKITDEBUGLEVEL2 (WEBKIT_LOG_LOADING, "%s:  checking complete, current state IFWEBFRAMESTATE_PROVISIONAL, %d errors\n", [[self name] cString], [[self errors] count]);
                if (![[self provisionalDataSource] isLoading]){
                    WEBKITDEBUGLEVEL1 (WEBKIT_LOG_LOADING, "%s:  checking complete in IFWEBFRAMESTATE_PROVISIONAL, load done\n", [[self name] cString]);
                    // We now the provisional data source didn't cut the mustard, release it.
                    [data setProvisionalDataSource: nil];
                    
                    [self _setState: IFWEBFRAMESTATE_COMPLETE];
                    [[self controller] locationChangeDone: [self mainDocumentError] forFrame: self];
                    return;
                }
            }
            return;
        }
        
        case IFWEBFRAMESTATE_COMMITTED:
        {
            WEBKITDEBUGLEVEL1 (WEBKIT_LOG_LOADING, "%s:  checking complete, current state IFWEBFRAMESTATE_COMMITTED\n", [[self name] cString]);
            if (![[self dataSource] isLoading]){
               [self _setState: IFWEBFRAMESTATE_COMPLETE];
                id mainView = [[[self controller] mainFrame] view];
                id thisView = [self view];
                
                WEBKIT_ASSERT ([self dataSource] != nil);
                
                [[self dataSource] _part]->end();
                
                // May need to relayout each time a frame is completely
                // loaded.
                [mainView setNeedsLayout: YES];
                
                // Layout this view (eventually).
                [thisView setNeedsLayout: YES];
                
                // Draw this view (eventually), and it's scroll view
                // (eventually).
                [thisView setNeedsDisplay: YES];
                if ([thisView _frameScrollView])
                    [[thisView _frameScrollView] setNeedsDisplay: YES];
                
                if ([[self controller] mainFrame] == self){
                    [mainView layout];
                    [mainView display];
                }
                
                [[self controller] locationChangeDone: [self mainDocumentError] forFrame: self];
                
                return;
            }
            return;
        }
        
        case IFWEBFRAMESTATE_COMPLETE:
        {
            WEBKITDEBUGLEVEL1 (WEBKIT_LOG_LOADING, "%s:  checking complete, current state IFWEBFRAMESTATE_COMPLETE\n", [[self name] cString]);
            return;
        }
        
        // Yikes!  Serious horkage.
        default:
        {
            [[NSException exceptionWithName:NSGenericException reason:@"invalid state during completion check with error" userInfo: nil] raise];
            break;
        }
    }
}

+ (void)_recursiveCheckCompleteFromFrame: (IFWebFrame *)fromFrame
{
    int i, count;
    NSArray *childFrames;
    
    childFrames = [[fromFrame dataSource] children];
    count = [childFrames count];
    for (i = 0; i < count; i++){
        IFWebFrame *childFrame;
        
        childFrame = [childFrames objectAtIndex: i];
        [IFWebFrame _recursiveCheckCompleteFromFrame: childFrame];
        [childFrame _isLoadComplete];
    }
    [fromFrame _isLoadComplete];
}

// Called every time a resource is completely loaded, or an error is received.
- (void)_checkLoadCompleteResource: (NSString *)resourceDescription error: (IFError *)error isMainDocument: (BOOL)mainDocument
{

    WEBKIT_ASSERT ([self controller] != nil);

    if (error) {
        if (mainDocument)
            [self _setMainDocumentError: error];
        [self _addError: error forResource: resourceDescription];
    }

    // Now walk the frame tree to see if any frame that may have initiated a load is done.
    [IFWebFrame _recursiveCheckCompleteFromFrame: [[self controller] mainFrame]];
}


- (void)_setMainDocumentError: (IFError *)error
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;
    data->mainDocumentError = [error retain];
}

- (void)_clearErrors
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;

    [data->errors release];
    data->errors = nil;
    [data->mainDocumentError release];
    data->mainDocumentError = nil;
}

@end

