//
//  WebCoreStatistics.m
//  WebKit
//
//  Created by Darin Adler on Thu Mar 28 2002.
//  Copyright (c) 2002 Apple Computer, Inc. All rights reserved.
//

#import "WebCoreStatistics.h"

#import <WebCore/WebCoreCache.h>
#import <WebCore/WebCoreJavaScript.h>

@implementation WebCoreStatistics

+ (NSArray *)statistics
{
    return [WebCoreCache statistics];
}

+ (void)empty
{
    [WebCoreCache empty];
}

+ (void)setDisabled:(BOOL)disabled
{
    [WebCoreCache setDisabled:disabled];
}

+ (int)javaScriptObjectsCount
{
    return [WebCoreJavaScript objectCount];
}

+ (int)javaScriptInterpretersCount
{
    return [WebCoreJavaScript interpreterCount];
}

+ (int)javaScriptNoGCAllowedObjectsCount
{
    return [WebCoreJavaScript noGCAllowedObjectCount];
}

+ (int)javaScriptReferencedObjectsCount
{
    return [WebCoreJavaScript referencedObjectCount];
}

+ (void)garbageCollectJavaScriptObjects
{
    [WebCoreJavaScript garbageCollect];
}

@end
