/*	
        WebLocationChangeDelegate.h
	Copyright 2001, 2002, Apple, Inc. All rights reserved.

        Public header file.
*/

#import <Foundation/Foundation.h>

@class WebView;
@class WebDataSource;
@class WebError;
@class WebFrame;

/*!
    @category WebLocationChangeDelegate
    @discussion A WebView's WebLocationChangeDelegate track changes its frame's location. 
*/
@interface NSObject (WebLocationChangeDelegate)

/*!
    @method locationChangeStartedForDataSource:
    @abstract Notify that a location change has started on a given data source
    @param webView The WebView sending the message.
    @param dataSource The data source for which the location change has started
    @discussion You can find out the URL and other parameters of the location change
    from the data source object.
*/
- (void)webView:(WebView *)sender locationChangeStartedForDataSource:(WebDataSource *)dataSource;

/*!
    @method serverRedirectedTo:forDataSource:
    @abstract Notify that the data source has received a server redirect.
    @param dataSource The data source for which the redirect occurred
    @discussion You can find the new URL from the data source object.
*/
- (void)webView:(WebView *)sender serverRedirectedForDataSource:(WebDataSource *)dataSource;

/*!
    @method locationChangeCommittedForDataSource:
    @abstract Notify that a location change has been committed on a given data source
    @param webView The WebView sending the message.
    @param dataSource The data source for which the location change has started
    @discussion When a location change starts, it is considered
    "provisional" until at least one byte of the new page is
    received. This is done so the old page will not be lost if the new
    page fails to load completely.

    In some cases, a single location change may be committed more than once. This happens
    in the case of multipart/x-mixed-replace, also known as "server push". In this case,
    a single location change leads to multiple documents that are loaded in sequence. When
    this happens, a new commit will be sent for each document.
*/
- (void)webView:(WebView *)sender locationChangeCommittedForDataSource:(WebDataSource *)dataSource;

/*!
    @method receivedPageTitle:forDataSource:
    @abstract Notify that the page title has been determined or has changed
    @param title The new page title
    @param dataSource The data source for which the title changed
    @discussion The title may update during loading; clients should be prepared for this.
*/
- (void)webView:(WebView *)sender receivedPageTitle:(NSString *)title forDataSource:(WebDataSource *)dataSource;

/*!
    @method receivedPageIcon:forDataSource:
    @abstract Notify that a page icon image is available or has changed
    @param webView The WebView sending the message.
    @param image The icon image
    @param dataSource The data source for which a page icon is ready
    @discussion It's possible the image will change, starting out as a
    default or stored image and changing on a refresh.

*/
- (void)webView:(WebView *)sender receivedPageIcon:(NSImage *)image forDataSource:(WebDataSource *)dataSource;

/*!
    @method locationChangeDone:forDataSource:
    @abstract Notify that a location change is done (possibly with an error) for a data source
    @param webView The WebView sending the message.
    @param error The error, if one occurred, or nil if none
    @param dataSource The data source that finished changing location
    @discussion This callback will only be received when all
    subresources are done loading.
*/
- (void)webView:(WebView *)sender locationChangeDone:(WebError *)error forDataSource:(WebDataSource *)dataSource;

/*!
    @method willCloseLocationForDataSource:
    @abstract Notify that a location will be closed.
    @param webView The WebView sending the message.
    @param dataSource The data source that where the location change originally happened
    @discussion This callback happens right before WebKit is done with the data source
    and the document representation that it contains.
 */
- (void)webView:(WebView *)sender willCloseLocationForDataSource:(WebDataSource *)dataSource;

/*!
    @method locationChangedWithinPageForDataSource:
    @abstract Notify that a further location change within the page
    has occurred for an already loaded data source
    @param webView The WebView sending the message.
    @param dataSource The data source that had a location change
    @discussion This is normally used for clicks on anchors within a page
    that is already displayed. You can find the new URL from the data source object.
*/
- (void)webView:(WebView *)sender locationChangedWithinPageForDataSource:(WebDataSource *)dataSource;

/*!
    @method clientWillRedirectTo:delay:fireDate:forFrame:
    @abstract Notify that the frame has received a client-side redirect that may trigger soon
    @param webView The WebView sending the message.
    @param URL The URL to be redirected to
    @param seconds Seconds in which the redirect will happen
    @param date The fire date
    @param frame The frame on which the redirect will occur
    @discussion This method can be used to keep progress feedback
    going while a client redirect is pending. A client redirect might
    be cancelled before it fires - see clientRedirectCancelledForFrame.
*/
- (void)webView:(WebView *)sender clientWillRedirectTo:(NSURL *)URL delay:(NSTimeInterval)seconds fireDate:(NSDate *)date forFrame:(WebFrame *)frame;

/*!
    @method clientRedirectCancelledForFrame:
    @abstract Notify that a pending client redirect has been cancelled.
    @param webView The WebView sending the message.
    @param frame The frame for which the pending redirect was cancelled
    @discussion A client redirect can be cancelled if the frame
    changes locations before the timeout.
*/
- (void)webView:(WebView *)sender clientRedirectCancelledForFrame:(WebFrame *)frame;

@end

