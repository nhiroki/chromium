/*	WebLoadProgress.mm

        Copyright 2001, 2002, Apple, Inc. All rights reserved.
*/

#import <WebKit/WebLoadProgress.h>

#import <WebFoundation/WebResourceHandle.h>

@implementation WebLoadProgress

- (id)init
{
    return [self initWithBytesSoFar:-1 totalToLoad:-1];
}

- (id)initWithBytesSoFar:(int)bytes totalToLoad:(int)total
{
    if (![super init]) {
        return nil;
    }

    bytesSoFar = bytes;
    totalToLoad = total;

    return self;
}

- (id)initWithResourceHandle:(WebResourceHandle *)handle
{
    int b = [handle contentLengthReceived];
    int t = [handle statusCode] == WebResourceHandleStatusLoadComplete ? b : [handle contentLength];
    return [self initWithBytesSoFar:b totalToLoad:t];
}

+ (WebLoadProgress *)progress
{
    return [[[WebLoadProgress alloc] init] autorelease];
}

+ (WebLoadProgress *)progressWithBytesSoFar:(int)bytes totalToLoad:(int)total
{
    return [[[WebLoadProgress alloc] initWithBytesSoFar:bytes totalToLoad:total] autorelease];
}

+ (WebLoadProgress *)progressWithResourceHandle:(WebResourceHandle *)handle
{
    return [[[WebLoadProgress alloc] initWithResourceHandle:handle] autorelease];
}

- (int)bytesSoFar
{
    return bytesSoFar;
}

- (int)totalToLoad
{
    return totalToLoad;
}

@end
