/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "WebProcess.h"

#import "FullKeyboardAccessWatcher.h"
#import "SandboxExtension.h"
#import "SecItemRequestData.h"
#import "SecItemResponseData.h"
#import "WebPage.h"
#import "WebProcessCreationParameters.h"
#import "WebProcessProxyMessages.h"
#import "WebProcessShim.h"
#import <WebCore/FileSystem.h>
#import <WebCore/LocalizedStrings.h>
#import <WebCore/MemoryCache.h>
#import <WebCore/PageCache.h>
#import <WebKitSystemInterface.h>
#import <algorithm>
#import <dispatch/dispatch.h>
#import <dlfcn.h>
#import <mach/host_info.h>
#import <mach/mach.h>
#import <mach/mach_error.h>
#import <objc/runtime.h>

#if ENABLE(WEB_PROCESS_SANDBOX)
#import <sandbox.h>
#import <stdlib.h>
#import <sysexits.h>
#endif

using namespace WebCore;
using namespace std;

namespace WebKit {

static uint64_t memorySize()
{
    static host_basic_info_data_t hostInfo;

    static dispatch_once_t once;
    dispatch_once(&once, ^() {
        mach_port_t host = mach_host_self();
        mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
        kern_return_t r = host_info(host, HOST_BASIC_INFO, (host_info_t)&hostInfo, &count);
        mach_port_deallocate(mach_task_self(), host);

        if (r != KERN_SUCCESS)
            LOG_ERROR("%s : host_info(%d) : %s.\n", __FUNCTION__, r, mach_error_string(r));
    });

    return hostInfo.max_mem;
}

static uint64_t volumeFreeSize(NSString *path)
{
    NSDictionary *fileSystemAttributesDictionary = [[NSFileManager defaultManager] attributesOfFileSystemForPath:path error:NULL];
    return [[fileSystemAttributesDictionary objectForKey:NSFileSystemFreeSize] unsignedLongLongValue];
}

void WebProcess::platformSetCacheModel(CacheModel cacheModel)
{
    RetainPtr<NSString> nsurlCacheDirectory(AdoptNS, (NSString *)WKCopyFoundationCacheDirectory());
    if (!nsurlCacheDirectory)
        nsurlCacheDirectory = NSHomeDirectory();

    // As a fudge factor, use 1000 instead of 1024, in case the reported byte 
    // count doesn't align exactly to a megabyte boundary.
    uint64_t memSize = memorySize() / 1024 / 1000;
    uint64_t diskFreeSize = volumeFreeSize(nsurlCacheDirectory.get()) / 1024 / 1000;

    unsigned cacheTotalCapacity = 0;
    unsigned cacheMinDeadCapacity = 0;
    unsigned cacheMaxDeadCapacity = 0;
    double deadDecodedDataDeletionInterval = 0;
    unsigned pageCacheCapacity = 0;
    unsigned long urlCacheMemoryCapacity = 0;
    unsigned long urlCacheDiskCapacity = 0;

    calculateCacheSizes(cacheModel, memSize, diskFreeSize,
        cacheTotalCapacity, cacheMinDeadCapacity, cacheMaxDeadCapacity, deadDecodedDataDeletionInterval,
        pageCacheCapacity, urlCacheMemoryCapacity, urlCacheDiskCapacity);


    memoryCache()->setCapacities(cacheMinDeadCapacity, cacheMaxDeadCapacity, cacheTotalCapacity);
    memoryCache()->setDeadDecodedDataDeletionInterval(deadDecodedDataDeletionInterval);
    pageCache()->setCapacity(pageCacheCapacity);

    NSURLCache *nsurlCache = [NSURLCache sharedURLCache];
    [nsurlCache setMemoryCapacity:urlCacheMemoryCapacity];
    [nsurlCache setDiskCapacity:max<unsigned long>(urlCacheDiskCapacity, [nsurlCache diskCapacity])]; // Don't shrink a big disk cache, since that would cause churn.
}

void WebProcess::platformClearResourceCaches(ResourceCachesToClear cachesToClear)
{
    if (cachesToClear == InMemoryResourceCachesOnly)
        return;
    [[NSURLCache sharedURLCache] removeAllCachedResponses];
}

bool WebProcess::fullKeyboardAccessEnabled()
{
    return [FullKeyboardAccessWatcher fullKeyboardAccessEnabled];
}

#if ENABLE(WEB_PROCESS_SANDBOX)
static void appendSandboxParameterPathInternal(Vector<const char*>& vector, const char* name, const char* path)
{
    char normalizedPath[PATH_MAX];
    if (!realpath(path, normalizedPath))
        normalizedPath[0] = '\0';

    vector.append(name);
    vector.append(fastStrDup(normalizedPath));
}

static void appendReadwriteConfDirectory(Vector<const char*>& vector, const char* name, int confID)
{
    char path[PATH_MAX];
    if (confstr(confID, path, PATH_MAX) <= 0)
        path[0] = '\0';

    appendSandboxParameterPathInternal(vector, name, path);
}

static void appendReadonlySandboxDirectory(Vector<const char*>& vector, const char* name, NSString *path)
{
    appendSandboxParameterPathInternal(vector, name, [(NSString *)path fileSystemRepresentation]);
}

static void appendReadwriteSandboxDirectory(Vector<const char*>& vector, const char* name, NSString *path)
{
    NSError *error = nil;

    // This is very unlikely to fail, but in case it actually happens, we'd like some sort of output in the console.
    if (![[NSFileManager defaultManager] createDirectoryAtPath:path withIntermediateDirectories:YES attributes:nil error:&error])
        NSLog(@"could not create \"%@\", error %@", path, error);

    appendSandboxParameterPathInternal(vector, name, [(NSString *)path fileSystemRepresentation]);
}

#endif

static void initializeSandbox(const WebProcessCreationParameters& parameters)
{
#if ENABLE(WEB_PROCESS_SANDBOX)
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DisableSandbox"]) {
        fprintf(stderr, "Bypassing sandbox due to DisableSandbox user default.\n");
        return;
    }

    Vector<const char*> sandboxParameters;

    // These are read-only.
    appendReadonlySandboxDirectory(sandboxParameters, "WEBKIT2_FRAMEWORK_DIR", [[[NSBundle bundleForClass:NSClassFromString(@"WKView")] bundlePath] stringByDeletingLastPathComponent]);
    appendReadonlySandboxDirectory(sandboxParameters, "UI_PROCESS_BUNDLE_RESOURCE_DIR", parameters.uiProcessBundleResourcePath);

    // These are read-write getconf paths.
    appendReadwriteConfDirectory(sandboxParameters, "DARWIN_USER_TEMP_DIR", _CS_DARWIN_USER_TEMP_DIR);
    appendReadwriteConfDirectory(sandboxParameters, "DARWIN_USER_CACHE_DIR", _CS_DARWIN_USER_CACHE_DIR);

    // These are read-write paths.
    appendReadwriteSandboxDirectory(sandboxParameters, "HOME_DIR", NSHomeDirectory());
    appendReadwriteSandboxDirectory(sandboxParameters, "WEBKIT_DATABASE_DIR", parameters.databaseDirectory);
    appendReadwriteSandboxDirectory(sandboxParameters, "WEBKIT_LOCALSTORAGE_DIR", parameters.localStorageDirectory);
    appendReadwriteSandboxDirectory(sandboxParameters, "WEBKIT_APPLICATION_CACHE_DIR", parameters.applicationCacheDirectory);
    appendReadwriteSandboxDirectory(sandboxParameters, "NSURL_CACHE_DIR", parameters.nsURLCachePath);

    sandboxParameters.append(static_cast<const char*>(0));

    const char* profilePath = [[[NSBundle mainBundle] pathForResource:@"com.apple.WebProcess" ofType:@"sb"] fileSystemRepresentation];

    char* errorBuf;
    if (sandbox_init_with_parameters(profilePath, SANDBOX_NAMED_EXTERNAL, sandboxParameters.data(), &errorBuf)) {
        fprintf(stderr, "WebProcess: couldn't initialize sandbox profile [%s]\n", profilePath);
        for (size_t i = 0; sandboxParameters[i]; i += 2)
            fprintf(stderr, "%s=%s\n", sandboxParameters[i], sandboxParameters[i + 1]);
        exit(EX_NOPERM);
    }

    for (size_t i = 0; sandboxParameters[i]; i += 2)
        fastFree(const_cast<char*>(sandboxParameters[i + 1]));
#endif
}

static id NSApplicationAccessibilityFocusedUIElement(NSApplication*, SEL)
{
    WebPage* page = WebProcess::shared().focusedWebPage();
    if (!page || !page->accessibilityRemoteObject())
        return 0;

    return [page->accessibilityRemoteObject() accessibilityFocusedUIElement];
}
    
void WebProcess::platformInitializeWebProcess(const WebProcessCreationParameters& parameters, CoreIPC::ArgumentDecoder*)
{
    [[NSFileManager defaultManager] changeCurrentDirectoryPath:[[NSBundle mainBundle] bundlePath]];

    initializeSandbox(parameters);

    if (!parameters.parentProcessName.isNull()) {
        NSString *applicationName = [NSString stringWithFormat:WEB_UI_STRING("%@ Web Content", "Visible name of the web process. The argument is the application name."), (NSString *)parameters.parentProcessName];
        WKSetVisibleApplicationName((CFStringRef)applicationName);
    }

    if (!parameters.nsURLCachePath.isNull()) {
        NSUInteger cacheMemoryCapacity = parameters.nsURLCacheMemoryCapacity;
        NSUInteger cacheDiskCapacity = parameters.nsURLCacheDiskCapacity;

        RetainPtr<NSURLCache> parentProcessURLCache(AdoptNS, [[NSURLCache alloc] initWithMemoryCapacity:cacheMemoryCapacity diskCapacity:cacheDiskCapacity diskPath:parameters.nsURLCachePath]);
        [NSURLCache setSharedURLCache:parentProcessURLCache.get()];
    }

    m_compositingRenderServerPort = parameters.acceleratedCompositingPort.port();

    // rdar://9118639 accessibilityFocusedUIElement in NSApplication defaults to use the keyWindow. Since there's
    // no window in WK2, NSApplication needs to use the focused page's focused element.
    Method methodToPatch = class_getInstanceMethod([NSApplication class], @selector(accessibilityFocusedUIElement));
    method_setImplementation(methodToPatch, (IMP)NSApplicationAccessibilityFocusedUIElement);
}

// FIXME (https://bugs.webkit.org/show_bug.cgi?id=60975) - Once CoreIPC supports sync messaging from a secondary thread,
// we can remove SecItemAPIContext and these 4 main-thread methods, and we can have the shim methods call out directly 
// from whatever thread they're on.

struct SecItemAPIContext
{
    CFDictionaryRef query;
    CFDictionaryRef attributesToUpdate;
    CFTypeRef resultObject;
    OSStatus resultCode;
};

static void WebSecItemCopyMatchingMainThread(void* voidContext)
{
    SecItemAPIContext* context = (SecItemAPIContext*)voidContext;
    
    SecItemRequestData requestData(context->query);
    SecItemResponseData response;
    if (!WebProcess::shared().connection()->sendSync(Messages::WebProcessProxy::SecItemCopyMatching(requestData), Messages::WebProcessProxy::SecItemCopyMatching::Reply(response), 0)) {
        context->resultCode = errSecInteractionNotAllowed;
        ASSERT_NOT_REACHED();
        return;
    }
    
    context->resultObject = response.resultObject().leakRef();
    context->resultCode = response.resultCode();
}

static OSStatus WebSecItemCopyMatching(CFDictionaryRef query, CFTypeRef* result)
{
    SecItemAPIContext context;
    context.query = query;
    
    callOnMainThreadAndWait(WebSecItemCopyMatchingMainThread, &context);
    
    if (result)
        *result = context.resultObject;
    return context.resultCode;
}

static void WebSecItemAddOnMainThread(void* voidContext)
{
    SecItemAPIContext* context = (SecItemAPIContext*)voidContext;
    
    SecItemRequestData requestData(context->query);
    SecItemResponseData response;
    if (!WebProcess::shared().connection()->sendSync(Messages::WebProcessProxy::SecItemAdd(requestData), Messages::WebProcessProxy::SecItemAdd::Reply(response), 0)) {
        context->resultCode = errSecInteractionNotAllowed;
        ASSERT_NOT_REACHED();
        return;
    }
    
    context->resultObject = response.resultObject().leakRef();
    context->resultCode = response.resultCode();
}

static OSStatus WebSecItemAdd(CFDictionaryRef query, CFTypeRef* result)
{    
    SecItemAPIContext context;
    context.query = query;
    
    callOnMainThreadAndWait(WebSecItemAddOnMainThread, &context);
    
    if (result)
        *result = context.resultObject;
    return context.resultCode;
}

static void WebSecItemUpdateOnMainThread(void* voidContext)
{
    SecItemAPIContext* context = (SecItemAPIContext*)voidContext;
    
    SecItemRequestData requestData(context->query, context->attributesToUpdate);
    SecItemResponseData response;
    if (!WebProcess::shared().connection()->sendSync(Messages::WebProcessProxy::SecItemUpdate(requestData), Messages::WebProcessProxy::SecItemUpdate::Reply(response), 0)) {
        context->resultCode = errSecInteractionNotAllowed;
        ASSERT_NOT_REACHED();
        return;
    }
    
    context->resultCode = response.resultCode();
}

static OSStatus WebSecItemUpdate(CFDictionaryRef query, CFDictionaryRef attributesToUpdate)
{    
    SecItemAPIContext context;
    context.query = query;
    context.attributesToUpdate = attributesToUpdate;
    
    callOnMainThreadAndWait(WebSecItemUpdateOnMainThread, &context);
    
    return context.resultCode;
}

static void WebSecItemDeleteOnMainThread(void* voidContext)
{
    SecItemAPIContext* context = (SecItemAPIContext*)voidContext;
    
    SecItemRequestData requestData(context->query);
    SecItemResponseData response;
    if (!WebProcess::shared().connection()->sendSync(Messages::WebProcessProxy::SecItemDelete(requestData), Messages::WebProcessProxy::SecItemDelete::Reply(response), 0)) {
        context->resultCode = errSecInteractionNotAllowed;
        ASSERT_NOT_REACHED();
        return;
    }
    
    context->resultCode = response.resultCode();
}

static OSStatus WebSecItemDelete(CFDictionaryRef query)
{    
    SecItemAPIContext context;
    context.query = query;
    
    callOnMainThreadAndWait(WebSecItemDeleteOnMainThread, &context);

    return context.resultCode;
}

void WebProcess::initializeShim()
{
    const WebProcessShimCallbacks callbacks = {
        WebSecItemCopyMatching,
        WebSecItemAdd,
        WebSecItemUpdate,
        WebSecItemDelete
    };
    
    WebProcessShimInitializeFunc initFunc = reinterpret_cast<WebProcessShimInitializeFunc>(dlsym(RTLD_DEFAULT, "WebKitWebProcessShimInitialize"));
    initFunc(callbacks);
}

void WebProcess::platformTerminate()
{
}

} // namespace WebKit
