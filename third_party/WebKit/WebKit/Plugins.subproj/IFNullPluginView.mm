/*	
    IFNullPluginView.mm
	Copyright 2002, Apple, Inc. All rights reserved.
*/

#import <WebKit/IFNullPluginView.h>

#import <WebKit/IFWebView.h>
#import <WebKit/IFWebController.h>
#import <WebKit/IFNSViewExtras.h>

static NSImage *image = nil;

@implementation IFNullPluginView

- initWithFrame:(NSRect)frame mimeType:(NSString *)mime arguments:(NSDictionary *)arguments
{
    NSBundle *bundle;
    NSString *imagePath, *pluginPageString;
    
    self = [super initWithFrame:frame];
    if (self) {
        // Set the view's image to the null plugin icon
        if (!image) {
            bundle = [NSBundle bundleWithIdentifier:@"com.apple.webkit"];
            imagePath = [bundle pathForResource:@"nullplugin" ofType:@"tiff"];
            image = [[NSImage alloc] initWithContentsOfFile:imagePath];
        }
        [self setImage:image];
        
        pluginPageString = [arguments objectForKey:@"pluginspage"];
        if(pluginPageString)
            pluginPage = [[NSURL URLWithString:pluginPageString] retain];
        if(mime)
            mimeType = [mime retain];
        
        errorSent = NO;
    }
    return self;
}

- (NSView *) findSuperview:(NSString *) viewName
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

- (void)drawRect:(NSRect)rect {
    IFWebView *webView;
    IFWebController *webController;
    
    [super drawRect:rect];
    if(!errorSent){
        errorSent = YES;
        webView = [self _IF_parentWebView];
        webController = [webView controller];
        [[webController policyHandler] pluginNotFoundForMIMEType:mimeType pluginPageURL:pluginPage];
    }
}

@end
