/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
    Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#include "config.h"
#include "core/loader/cache/CachedResourceLoader.h"

#include <wtf/MemoryInstrumentationHashMap.h>
#include <wtf/MemoryInstrumentationHashSet.h>
#include <wtf/MemoryInstrumentationListHashSet.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include <wtf/UnusedParam.h>
#include "bindings/v8/ScriptController.h"
#include "core/dom/Document.h"
#include "core/html/HTMLElement.h"
#include "core/html/HTMLFrameOwnerElement.h"
#include "core/loader/DocumentLoader.h"
#include "core/loader/FrameLoader.h"
#include "core/loader/FrameLoaderClient.h"
#include "core/loader/PingLoader.h"
#include "core/loader/UniqueIdentifier.h"
#include "core/loader/cache/CachedCSSStyleSheet.h"
#include "core/loader/cache/CachedDocument.h"
#include "core/loader/cache/CachedFont.h"
#include "core/loader/cache/CachedImage.h"
#include "core/loader/cache/CachedRawResource.h"
#include "core/loader/cache/CachedResourceRequest.h"
#include "core/loader/cache/CachedScript.h"
#include "core/loader/cache/CachedShader.h"
#include "core/loader/cache/CachedTextTrack.h"
#include "core/loader/cache/CachedXSLStyleSheet.h"
#include "core/loader/cache/MemoryCache.h"
#include "core/page/Console.h"
#include "core/page/ContentSecurityPolicy.h"
#include "core/page/DOMWindow.h"
#include "core/page/Frame.h"
#include "core/page/Performance.h"
#include "core/page/Settings.h"
#include "core/platform/Logging.h"
#include "public/platform/Platform.h"
#include "public/platform/WebURL.h"
#include "weborigin/SecurityOrigin.h"
#include "weborigin/SecurityPolicy.h"

#define PRELOAD_DEBUG 0

namespace WebCore {

static CachedResource* createResource(CachedResource::Type type, const ResourceRequest& request, const String& charset)
{
    switch (type) {
    case CachedResource::ImageResource:
        return new CachedImage(request);
    case CachedResource::CSSStyleSheet:
        return new CachedCSSStyleSheet(request, charset);
    case CachedResource::Script:
        return new CachedScript(request, charset);
    case CachedResource::SVGDocumentResource:
        return new CachedDocument(request, CachedResource::SVGDocumentResource);
    case CachedResource::FontResource:
        return new CachedFont(request);
    case CachedResource::RawResource:
    case CachedResource::MainResource:
        return new CachedRawResource(request, type);
    case CachedResource::XSLStyleSheet:
        return new CachedXSLStyleSheet(request);
    case CachedResource::LinkPrefetch:
        return new CachedResource(request, CachedResource::LinkPrefetch);
    case CachedResource::LinkSubresource:
        return new CachedResource(request, CachedResource::LinkSubresource);
    case CachedResource::TextTrackResource:
        return new CachedTextTrack(request);
    case CachedResource::ShaderResource:
        return new CachedShader(request);
    }
    ASSERT_NOT_REACHED();
    return 0;
}

static ResourceLoadPriority loadPriority(CachedResource::Type type, const CachedResourceRequest& request)
{
    if (request.priority() != ResourceLoadPriorityUnresolved)
        return request.priority();

    switch (type) {
    case CachedResource::MainResource:
        return ResourceLoadPriorityVeryHigh;
    case CachedResource::CSSStyleSheet:
        return ResourceLoadPriorityHigh;
    case CachedResource::Script:
    case CachedResource::FontResource:
    case CachedResource::RawResource:
        return ResourceLoadPriorityMedium;
    case CachedResource::ImageResource:
        return request.forPreload() ? ResourceLoadPriorityVeryLow : ResourceLoadPriorityLow;
    case CachedResource::XSLStyleSheet:
        return ResourceLoadPriorityHigh;
    case CachedResource::SVGDocumentResource:
        return ResourceLoadPriorityLow;
    case CachedResource::LinkPrefetch:
        return ResourceLoadPriorityVeryLow;
    case CachedResource::LinkSubresource:
        return ResourceLoadPriorityLow;
    case CachedResource::TextTrackResource:
        return ResourceLoadPriorityLow;
    case CachedResource::ShaderResource:
        return ResourceLoadPriorityMedium;
    }
    ASSERT_NOT_REACHED();
    return ResourceLoadPriorityUnresolved;
}

static CachedResource* resourceFromDataURIRequest(const ResourceRequest& request)
{
    const KURL& url = request.url();
    ASSERT(url.protocolIsData());

    WebKit::WebString mimetype;
    WebKit::WebString charset;
    RefPtr<SharedBuffer> data = PassRefPtr<SharedBuffer>(WebKit::Platform::current()->parseDataURL(url, mimetype, charset));
    if (!data)
        return 0;
    ResourceResponse response(url, mimetype, data->size(), charset, String());

    CachedResource* resource = createResource(CachedResource::ImageResource, request, charset);
    resource->responseReceived(response);
    // FIXME: AppendData causes an unnecessary memcpy.
    if (data->size())
        resource->appendData(data->data(), data->size());
    resource->finish();
    return resource;
}

CachedResourceLoader::CachedResourceLoader(DocumentLoader* documentLoader)
    : m_document(0)
    , m_documentLoader(documentLoader)
    , m_requestCount(0)
    , m_garbageCollectDocumentResourcesTimer(this, &CachedResourceLoader::garbageCollectDocumentResourcesTimerFired)
    , m_autoLoadImages(true)
    , m_imagesEnabled(true)
    , m_allowStaleResources(false)
{
}

CachedResourceLoader::~CachedResourceLoader()
{
    m_documentLoader = 0;
    m_document = 0;

    clearPreloads();

    // Make sure no requests still point to this CachedResourceLoader
    ASSERT(m_requestCount == 0);
}

CachedResource* CachedResourceLoader::cachedResource(const String& resourceURL) const 
{
    KURL url = m_document->completeURL(resourceURL);
    return cachedResource(url); 
}

CachedResource* CachedResourceLoader::cachedResource(const KURL& resourceURL) const
{
    KURL url = MemoryCache::removeFragmentIdentifierIfNeeded(resourceURL);
    return m_documentResources.get(url).get(); 
}

Frame* CachedResourceLoader::frame() const
{
    return m_documentLoader ? m_documentLoader->frame() : 0;
}

CachedResourceHandle<CachedImage> CachedResourceLoader::requestImage(CachedResourceRequest& request)
{
    if (Frame* f = frame()) {
        if (f->loader()->pageDismissalEventBeingDispatched() != FrameLoader::NoDismissal) {
            KURL requestURL = request.resourceRequest().url();
            if (requestURL.isValid() && canRequest(CachedResource::ImageResource, requestURL, request.options(), request.forPreload()))
                PingLoader::loadImage(f, requestURL);
            return 0;
        }
    }

    if (request.resourceRequest().url().protocolIsData())
        preCacheDataURIImage(request);

    request.setDefer(clientDefersImage(request.resourceRequest().url()) ? CachedResourceRequest::DeferredByClient : CachedResourceRequest::NoDefer);
    return static_cast<CachedImage*>(requestResource(CachedResource::ImageResource, request).get());
}

void CachedResourceLoader::preCacheDataURIImage(const CachedResourceRequest& request)
{
    const KURL& url = request.resourceRequest().url();
    ASSERT(url.protocolIsData());

    if (CachedResource* existing = memoryCache()->resourceForURL(url))
        return;

    if (CachedResource* resource = resourceFromDataURIRequest(request.resourceRequest()))
        memoryCache()->add(resource);
}

CachedResourceHandle<CachedFont> CachedResourceLoader::requestFont(CachedResourceRequest& request)
{
    return static_cast<CachedFont*>(requestResource(CachedResource::FontResource, request).get());
}

CachedResourceHandle<CachedTextTrack> CachedResourceLoader::requestTextTrack(CachedResourceRequest& request)
{
    return static_cast<CachedTextTrack*>(requestResource(CachedResource::TextTrackResource, request).get());
}

CachedResourceHandle<CachedShader> CachedResourceLoader::requestShader(CachedResourceRequest& request)
{
    return static_cast<CachedShader*>(requestResource(CachedResource::ShaderResource, request).get());
}

CachedResourceHandle<CachedCSSStyleSheet> CachedResourceLoader::requestCSSStyleSheet(CachedResourceRequest& request)
{
    return static_cast<CachedCSSStyleSheet*>(requestResource(CachedResource::CSSStyleSheet, request).get());
}

CachedResourceHandle<CachedCSSStyleSheet> CachedResourceLoader::requestUserCSSStyleSheet(CachedResourceRequest& request)
{
    KURL url = MemoryCache::removeFragmentIdentifierIfNeeded(request.resourceRequest().url());

    if (CachedResource* existing = memoryCache()->resourceForURL(url)) {
        if (existing->type() == CachedResource::CSSStyleSheet)
            return static_cast<CachedCSSStyleSheet*>(existing);
        memoryCache()->remove(existing);
    }

    request.setOptions(ResourceLoaderOptions(DoNotSendCallbacks, SniffContent, BufferData, AllowStoredCredentials, ClientRequestedCredentials, AskClientForCrossOriginCredentials, SkipSecurityCheck, CheckContentSecurityPolicy, UseDefaultOriginRestrictionsForType));
    return static_cast<CachedCSSStyleSheet*>(requestResource(CachedResource::CSSStyleSheet, request).get());
}

CachedResourceHandle<CachedScript> CachedResourceLoader::requestScript(CachedResourceRequest& request)
{
    return static_cast<CachedScript*>(requestResource(CachedResource::Script, request).get());
}

CachedResourceHandle<CachedXSLStyleSheet> CachedResourceLoader::requestXSLStyleSheet(CachedResourceRequest& request)
{
    return static_cast<CachedXSLStyleSheet*>(requestResource(CachedResource::XSLStyleSheet, request).get());
}

CachedResourceHandle<CachedDocument> CachedResourceLoader::requestSVGDocument(CachedResourceRequest& request)
{
    return static_cast<CachedDocument*>(requestResource(CachedResource::SVGDocumentResource, request).get());
}

CachedResourceHandle<CachedResource> CachedResourceLoader::requestLinkResource(CachedResource::Type type, CachedResourceRequest& request)
{
    ASSERT(frame());
    ASSERT(type == CachedResource::LinkPrefetch || type == CachedResource::LinkSubresource);
    return requestResource(type, request);
}

CachedResourceHandle<CachedRawResource> CachedResourceLoader::requestRawResource(CachedResourceRequest& request)
{
    return static_cast<CachedRawResource*>(requestResource(CachedResource::RawResource, request).get());
}

CachedResourceHandle<CachedRawResource> CachedResourceLoader::requestMainResource(CachedResourceRequest& request)
{
    return static_cast<CachedRawResource*>(requestResource(CachedResource::MainResource, request).get());
}

bool CachedResourceLoader::checkInsecureContent(CachedResource::Type type, const KURL& url) const
{
    switch (type) {
    case CachedResource::Script:
    case CachedResource::XSLStyleSheet:
    case CachedResource::SVGDocumentResource:
    case CachedResource::CSSStyleSheet:
        // These resource can inject script into the current document (Script,
        // XSL) or exfiltrate the content of the current document (CSS).
        if (Frame* f = frame())
            if (!f->loader()->mixedContentChecker()->canRunInsecureContent(m_document->securityOrigin(), url))
                return false;
        break;
    case CachedResource::TextTrackResource:
    case CachedResource::ShaderResource:
    case CachedResource::RawResource:
    case CachedResource::ImageResource:
    case CachedResource::FontResource: {
        // These resources can corrupt only the frame's pixels.
        if (Frame* f = frame()) {
            Frame* top = f->tree()->top();
            if (!top->loader()->mixedContentChecker()->canDisplayInsecureContent(top->document()->securityOrigin(), url))
                return false;
        }
        break;
    }
    case CachedResource::MainResource:
    case CachedResource::LinkPrefetch:
    case CachedResource::LinkSubresource:
        // Prefetch cannot affect the current document.
        break;
    }
    return true;
}

bool CachedResourceLoader::canRequest(CachedResource::Type type, const KURL& url, const ResourceLoaderOptions& options, bool forPreload)
{
    if (document() && !document()->securityOrigin()->canDisplay(url)) {
        if (!forPreload)
            FrameLoader::reportLocalLoadFailed(frame(), url.elidedString());
        LOG(ResourceLoading, "CachedResourceLoader::requestResource URL was not allowed by SecurityOrigin::canDisplay");
        return 0;
    }

    // FIXME: Convert this to check the isolated world's Content Security Policy once webkit.org/b/104520 is solved.
    bool shouldBypassMainWorldContentSecurityPolicy = (frame() && frame()->script()->shouldBypassMainWorldContentSecurityPolicy()) || (options.contentSecurityPolicyOption == DoNotCheckContentSecurityPolicy);

    // Some types of resources can be loaded only from the same origin.  Other
    // types of resources, like Images, Scripts, and CSS, can be loaded from
    // any URL.
    switch (type) {
    case CachedResource::MainResource:
    case CachedResource::ImageResource:
    case CachedResource::CSSStyleSheet:
    case CachedResource::Script:
    case CachedResource::FontResource:
    case CachedResource::RawResource:
    case CachedResource::LinkPrefetch:
    case CachedResource::LinkSubresource:
    case CachedResource::TextTrackResource:
    case CachedResource::ShaderResource:
        // By default these types of resources can be loaded from any origin.
        // FIXME: Are we sure about CachedResource::FontResource?
        if (options.requestOriginPolicy == RestrictToSameOrigin && !m_document->securityOrigin()->canRequest(url)) {
            printAccessDeniedMessage(url);
            return false;
        }
        break;
    case CachedResource::SVGDocumentResource:
    case CachedResource::XSLStyleSheet:
        if (!m_document->securityOrigin()->canRequest(url)) {
            printAccessDeniedMessage(url);
            return false;
        }
        break;
    }

    switch (type) {
    case CachedResource::XSLStyleSheet:
        if (!shouldBypassMainWorldContentSecurityPolicy && !m_document->contentSecurityPolicy()->allowScriptFromSource(url))
            return false;
        break;
    case CachedResource::Script:
        if (!shouldBypassMainWorldContentSecurityPolicy && !m_document->contentSecurityPolicy()->allowScriptFromSource(url))
            return false;

        if (frame()) {
            Settings* settings = frame()->settings();
            if (!frame()->loader()->client()->allowScriptFromSource(!settings || settings->isScriptEnabled(), url)) {
                frame()->loader()->client()->didNotAllowScript();
                return false;
            }
        }
        break;
    case CachedResource::ShaderResource:
        // Since shaders are referenced from CSS Styles use the same rules here.
    case CachedResource::CSSStyleSheet:
        if (!shouldBypassMainWorldContentSecurityPolicy && !m_document->contentSecurityPolicy()->allowStyleFromSource(url))
            return false;
        break;
    case CachedResource::SVGDocumentResource:
    case CachedResource::ImageResource:
        if (!shouldBypassMainWorldContentSecurityPolicy && !m_document->contentSecurityPolicy()->allowImageFromSource(url))
            return false;
        break;
    case CachedResource::FontResource: {
        if (!shouldBypassMainWorldContentSecurityPolicy && !m_document->contentSecurityPolicy()->allowFontFromSource(url))
            return false;
        break;
    }
    case CachedResource::MainResource:
    case CachedResource::RawResource:
    case CachedResource::LinkPrefetch:
    case CachedResource::LinkSubresource:
        break;
    case CachedResource::TextTrackResource:
        // Cues aren't called out in the CPS spec yet, but they only work with a media element
        // so use the media policy.
        if (!shouldBypassMainWorldContentSecurityPolicy && !m_document->contentSecurityPolicy()->allowMediaFromSource(url))
            return false;
        break;
    }

    // Last of all, check for insecure content. We do this last so that when
    // folks block insecure content with a CSP policy, they don't get a warning.
    // They'll still get a warning in the console about CSP blocking the load.

    // FIXME: Should we consider forPreload here?
    if (!checkInsecureContent(type, url))
        return false;

    return true;
}

CachedResourceHandle<CachedResource> CachedResourceLoader::requestResource(CachedResource::Type type, CachedResourceRequest& request)
{
    KURL url = request.resourceRequest().url();
    
    LOG(ResourceLoading, "CachedResourceLoader::requestResource '%s', charset '%s', priority=%d, forPreload=%u", url.elidedString().latin1().data(), request.charset().latin1().data(), request.priority(), request.forPreload());
    
    // If only the fragment identifiers differ, it is the same resource.
    url = MemoryCache::removeFragmentIdentifierIfNeeded(url);

    if (!url.isValid())
        return 0;

    if (!canRequest(type, url, request.options(), request.forPreload()))
        return 0;

    if (Frame* f = frame())
        f->loader()->client()->dispatchWillRequestResource(&request);

    // See if we can use an existing resource from the cache.
    CachedResourceHandle<CachedResource> resource = memoryCache()->resourceForURL(url);

    const RevalidationPolicy policy = determineRevalidationPolicy(type, request.mutableResourceRequest(), request.forPreload(), resource.get(), request.defer());
    switch (policy) {
    case Reload:
        memoryCache()->remove(resource.get());
        // Fall through
    case Load:
        resource = loadResource(type, request, request.charset());
        break;
    case Revalidate:
        resource = revalidateResource(request, resource.get());
        break;
    case Use:
        resource->updateForAccess();
        notifyLoadedFromMemoryCache(resource.get());
        break;
    }

    if (!resource)
        return 0;

    if (policy != Use)
        resource->setIdentifier(createUniqueIdentifier());

    if (!request.forPreload() || policy != Use) {
        ResourceLoadPriority priority = loadPriority(type, request);
        if (priority != resource->resourceRequest().priority()) {
            resource->resourceRequest().setPriority(priority);
            resource->didChangePriority(priority);
        }
    }

    if ((policy != Use || resource->stillNeedsLoad()) && CachedResourceRequest::NoDefer == request.defer()) {
        if (!frame())
            return 0;

        FrameLoader* frameLoader = frame()->loader();
        if (request.options().securityCheck == DoSecurityCheck && (frameLoader->state() == FrameStateProvisional || !frameLoader->activeDocumentLoader() || frameLoader->activeDocumentLoader()->isStopping()))
            return 0;

        if (!m_documentLoader->scheduleArchiveLoad(resource.get(), request.resourceRequest()))
            resource->load(this, request.options());

        // We don't support immediate loads, but we do support immediate failure.
        if (resource->errorOccurred()) {
            if (resource->inCache())
                memoryCache()->remove(resource.get());
            return 0;
        }
    }

    // FIXME: Temporarily leave main resource caching disabled for chromium,
    // see https://bugs.webkit.org/show_bug.cgi?id=107962. Before caching main
    // resources, we should be sure to understand the implications for memory
    // use.
    //
    // Ensure main resources aren't preloaded, and other main resource loads
    // are removed from cache to prevent reuse.
    if (type == CachedResource::MainResource) {
        ASSERT(policy != Use);
        ASSERT(policy != Revalidate);
        memoryCache()->remove(resource.get());
        if (request.forPreload())
            return 0;
    }

    if (!request.resourceRequest().url().protocolIsData())
        m_validatedURLs.add(request.resourceRequest().url());

    ASSERT(resource->url() == url.string());
    m_documentResources.set(resource->url(), resource);
    return resource;
}

void CachedResourceLoader::determineTargetType(ResourceRequest& request, CachedResource::Type type)
{
   ResourceRequest::TargetType targetType;

    switch (type) {
    case CachedResource::MainResource:
        if (frame()->tree()->parent())
            targetType = ResourceRequest::TargetIsSubframe;
        else
            targetType = ResourceRequest::TargetIsMainFrame;
        break;
    case CachedResource::CSSStyleSheet:
    case CachedResource::XSLStyleSheet:
        targetType = ResourceRequest::TargetIsStyleSheet;
        break;
    case CachedResource::Script:
        targetType = ResourceRequest::TargetIsScript;
        break;
    case CachedResource::FontResource:
        targetType = ResourceRequest::TargetIsFontResource;
        break;
    case CachedResource::ImageResource:
        targetType = ResourceRequest::TargetIsImage;
        break;
    case CachedResource::ShaderResource:
    case CachedResource::RawResource:
        targetType = ResourceRequest::TargetIsSubresource;
        break;
    case CachedResource::LinkPrefetch:
        targetType = ResourceRequest::TargetIsPrefetch;
        break;
    case CachedResource::LinkSubresource:
        targetType = ResourceRequest::TargetIsSubresource;
        break;
    case CachedResource::TextTrackResource:
        targetType = ResourceRequest::TargetIsTextTrack;
        break;
    case CachedResource::SVGDocumentResource:
        targetType = ResourceRequest::TargetIsImage;
        break;
    default:
        ASSERT_NOT_REACHED();
        targetType = ResourceRequest::TargetIsSubresource;
        break;
    }
    request.setTargetType(targetType);
}

ResourceRequestCachePolicy CachedResourceLoader::resourceRequestCachePolicy(const ResourceRequest& request, CachedResource::Type type)
{
    if (type == CachedResource::MainResource) {
        FrameLoadType frameLoadType = frame()->loader()->loadType();
        bool isReload = frameLoadType == FrameLoadTypeReload || frameLoadType == FrameLoadTypeReloadFromOrigin;
        if (request.httpMethod() == "POST" && (isReload || frameLoadType == FrameLoadTypeBackForward))
            return ReturnCacheDataDontLoad;
        if (!m_documentLoader->overrideEncoding().isEmpty() || frameLoadType == FrameLoadTypeBackForward)
            return ReturnCacheDataElseLoad;
        if (isReload || frameLoadType == FrameLoadTypeSame || request.isConditional())
            return ReloadIgnoringCacheData;
        return UseProtocolCachePolicy;
    }

    if (request.isConditional())
        return ReloadIgnoringCacheData;

    if (m_documentLoader->isLoadingInAPISense()) {
        // For POST requests, we mutate the main resource's cache policy to avoid form resubmission.
        // This policy should not be inherited by subresources.
        ResourceRequestCachePolicy mainResourceCachePolicy = m_documentLoader->request().cachePolicy();
        if (mainResourceCachePolicy == ReturnCacheDataDontLoad)
            return ReturnCacheDataElseLoad;
        return mainResourceCachePolicy;
    }
    return UseProtocolCachePolicy;
}

void CachedResourceLoader::addAdditionalRequestHeaders(ResourceRequest& request, CachedResource::Type type)
{
    if (!frame())
        return;

    bool isMainResource = type == CachedResource::MainResource;

    FrameLoader* frameLoader = frame()->loader();

    if (!isMainResource) {
        String outgoingReferrer;
        String outgoingOrigin;
        if (request.httpReferrer().isNull()) {
            outgoingReferrer = frameLoader->outgoingReferrer();
            outgoingOrigin = frameLoader->outgoingOrigin();
        } else {
            outgoingReferrer = request.httpReferrer();
            outgoingOrigin = SecurityOrigin::createFromString(outgoingReferrer)->toString();
        }

        outgoingReferrer = SecurityPolicy::generateReferrerHeader(document()->referrerPolicy(), request.url(), outgoingReferrer);
        if (outgoingReferrer.isEmpty())
            request.clearHTTPReferrer();
        else if (!request.httpReferrer())
            request.setHTTPReferrer(outgoingReferrer);

        FrameLoader::addHTTPOriginIfNeeded(request, outgoingOrigin);
    }

    if (request.cachePolicy() == UseProtocolCachePolicy)
        request.setCachePolicy(resourceRequestCachePolicy(request, type));
    if (request.targetType() == ResourceRequest::TargetIsUnspecified)
        determineTargetType(request, type);
    if (type == CachedResource::LinkPrefetch || type == CachedResource::LinkSubresource)
        request.setHTTPHeaderField("Purpose", "prefetch");
    frameLoader->addExtraFieldsToRequest(request);
}

CachedResourceHandle<CachedResource> CachedResourceLoader::revalidateResource(const CachedResourceRequest& request, CachedResource* resource)
{
    ASSERT(resource);
    ASSERT(resource->inCache());
    ASSERT(resource->isLoaded());
    ASSERT(resource->canUseCacheValidator());
    ASSERT(!resource->resourceToRevalidate());

    ResourceRequest revalidatingRequest(resource->resourceRequest());
    addAdditionalRequestHeaders(revalidatingRequest, resource->type());

    const String& lastModified = resource->response().httpHeaderField("Last-Modified");
    const String& eTag = resource->response().httpHeaderField("ETag");
    if (!lastModified.isEmpty() || !eTag.isEmpty()) {
        ASSERT(cachePolicy(resource->type()) != CachePolicyReload);
        if (cachePolicy(resource->type()) == CachePolicyRevalidate)
            revalidatingRequest.setHTTPHeaderField("Cache-Control", "max-age=0");
        if (!lastModified.isEmpty())
            revalidatingRequest.setHTTPHeaderField("If-Modified-Since", lastModified);
        if (!eTag.isEmpty())
            revalidatingRequest.setHTTPHeaderField("If-None-Match", eTag);
    }

    CachedResourceHandle<CachedResource> newResource = createResource(resource->type(), revalidatingRequest, resource->encoding());
    
    LOG(ResourceLoading, "Resource %p created to revalidate %p", newResource.get(), resource);
    newResource->setResourceToRevalidate(resource);
    
    memoryCache()->remove(resource);
    memoryCache()->add(newResource.get());
    storeResourceTimingInitiatorInformation(newResource, request);
    return newResource;
}

CachedResourceHandle<CachedResource> CachedResourceLoader::loadResource(CachedResource::Type type, CachedResourceRequest& request, const String& charset)
{
    ASSERT(!memoryCache()->resourceForURL(request.resourceRequest().url()));

    LOG(ResourceLoading, "Loading CachedResource for '%s'.", request.resourceRequest().url().elidedString().latin1().data());

    addAdditionalRequestHeaders(request.mutableResourceRequest(), type);
    CachedResourceHandle<CachedResource> resource = createResource(type, request.mutableResourceRequest(), charset);

    memoryCache()->add(resource.get());
    storeResourceTimingInitiatorInformation(resource, request);
    return resource;
}

void CachedResourceLoader::storeResourceTimingInitiatorInformation(const CachedResourceHandle<CachedResource>& resource, const CachedResourceRequest& request)
{
    CachedResourceInitiatorInfo info = request.options().initiatorInfo;
    info.startTime = monotonicallyIncreasingTime();

    if (resource->type() == CachedResource::MainResource) {
        // <iframe>s should report the initial navigation requested by the parent document, but not subsequent navigations.
        if (frame()->ownerElement() && !frame()->ownerElement()->loadedNonEmptyDocument()) {
            info.name = frame()->ownerElement()->localName();
            m_initiatorMap.add(resource.get(), info);
            frame()->ownerElement()->didLoadNonEmptyDocument();
        }
    } else {
        m_initiatorMap.add(resource.get(), info);
    }
}

CachedResourceLoader::RevalidationPolicy CachedResourceLoader::determineRevalidationPolicy(CachedResource::Type type, ResourceRequest& request, bool forPreload, CachedResource* existingResource, CachedResourceRequest::DeferOption defer) const
{
    if (!existingResource)
        return Load;

    // We already have a preload going for this URL.
    if (forPreload && existingResource->isPreloaded())
        return Use;

    // If the same URL has been loaded as a different type, we need to reload.
    if (existingResource->type() != type) {
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicy reloading due to type mismatch.");
        return Reload;
    }

    // Do not load from cache if images are not enabled. The load for this image will be blocked
    // in CachedImage::load.
    if (CachedResourceRequest::DeferredByClient == defer)
        return Reload;

    // Always use data uris.
    // FIXME: Extend this to non-images.
    if (type == CachedResource::ImageResource && request.url().protocolIsData())
        return Use;

    if (!existingResource->canReuse(request))
        return Reload;

    // Certain requests (e.g., XHRs) might have manually set headers that require revalidation.
    // FIXME: In theory, this should be a Revalidate case. In practice, the MemoryCache revalidation path assumes a whole bunch
    // of things about how revalidation works that manual headers violate, so punt to Reload instead.
    if (request.isConditional())
        return Reload;

    // Don't reload resources while pasting.
    if (m_allowStaleResources)
        return Use;
    
    // Alwaus use preloads.
    if (existingResource->isPreloaded())
        return Use;
    
    // CachePolicyHistoryBuffer uses the cache no matter what.
    if (cachePolicy(type) == CachePolicyHistoryBuffer)
        return Use;

    // Don't reuse resources with Cache-control: no-store.
    if (existingResource->response().cacheControlContainsNoStore()) {
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicy reloading due to Cache-control: no-store.");
        return Reload;
    }

    // If credentials were sent with the previous request and won't be
    // with this one, or vice versa, re-fetch the resource.
    //
    // This helps with the case where the server sends back
    // "Access-Control-Allow-Origin: *" all the time, but some of the
    // client's requests are made without CORS and some with.
    if (existingResource->resourceRequest().allowCookies() != request.allowCookies()) {
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicy reloading due to difference in credentials settings.");
        return Reload;
    }

    // During the initial load, avoid loading the same resource multiple times for a single document, even if the cache policies would tell us to.
    if (document() && !document()->loadEventFinished() && m_validatedURLs.contains(existingResource->url()))
        return Use;

    // CachePolicyReload always reloads
    if (cachePolicy(type) == CachePolicyReload) {
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicy reloading due to CachePolicyReload.");
        return Reload;
    }
    
    // We'll try to reload the resource if it failed last time.
    if (existingResource->errorOccurred()) {
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicye reloading due to resource being in the error state");
        return Reload;
    }
    
    // For resources that are not yet loaded we ignore the cache policy.
    if (existingResource->isLoading())
        return Use;

    // Check if the cache headers requires us to revalidate (cache expiration for example).
    if (existingResource->mustRevalidateDueToCacheHeaders(cachePolicy(type))) {
        // See if the resource has usable ETag or Last-modified headers.
        if (existingResource->canUseCacheValidator())
            return Revalidate;
        
        // No, must reload.
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicy reloading due to missing cache validators.");            
        return Reload;
    }

    return Use;
}

void CachedResourceLoader::printAccessDeniedMessage(const KURL& url) const
{
    if (url.isNull())
        return;

    if (!frame())
        return;

    String message;
    if (!m_document || m_document->url().isNull())
        message = "Unsafe attempt to load URL " + url.elidedString() + '.';
    else
        message = "Unsafe attempt to load URL " + url.elidedString() + " from frame with URL " + m_document->url().elidedString() + ". Domains, protocols and ports must match.\n";

    frame()->document()->addConsoleMessage(SecurityMessageSource, ErrorMessageLevel, message);
}

void CachedResourceLoader::setAutoLoadImages(bool enable)
{
    if (enable == m_autoLoadImages)
        return;

    m_autoLoadImages = enable;

    if (!m_autoLoadImages)
        return;

    reloadImagesIfNotDeferred();
}

void CachedResourceLoader::setImagesEnabled(bool enable)
{
    if (enable == m_imagesEnabled)
        return;

    m_imagesEnabled = enable;

    if (!m_imagesEnabled)
        return;

    reloadImagesIfNotDeferred();
}

bool CachedResourceLoader::clientDefersImage(const KURL& url) const
{
    return frame() && !frame()->loader()->client()->allowImage(m_imagesEnabled, url);
}

bool CachedResourceLoader::shouldDeferImageLoad(const KURL& url) const
{
    return clientDefersImage(url) || !m_autoLoadImages;
}

void CachedResourceLoader::reloadImagesIfNotDeferred()
{
    DocumentResourceMap::iterator end = m_documentResources.end();
    for (DocumentResourceMap::iterator it = m_documentResources.begin(); it != end; ++it) {
        CachedResource* resource = it->value.get();
        if (resource->type() == CachedResource::ImageResource && resource->stillNeedsLoad() && !clientDefersImage(resource->url()))
            const_cast<CachedResource*>(resource)->load(this, defaultCachedResourceOptions());
    }
}

CachePolicy CachedResourceLoader::cachePolicy(CachedResource::Type type) const
{
    if (!frame())
        return CachePolicyVerify;

    if (type != CachedResource::MainResource)
        return frame()->loader()->subresourceCachePolicy();
    
    if (frame()->loader()->loadType() == FrameLoadTypeReloadFromOrigin || frame()->loader()->loadType() == FrameLoadTypeReload)
        return CachePolicyReload;
    return CachePolicyVerify;
}

void CachedResourceLoader::loadDone(CachedResource* resource)
{
    RefPtr<DocumentLoader> protectDocumentLoader(m_documentLoader);
    RefPtr<Document> protectDocument(m_document);

    if (resource && resource->response().isHTTP() && ((!resource->errorOccurred() && !resource->wasCanceled()) || resource->response().httpStatusCode() == 304)) {
        HashMap<CachedResource*, CachedResourceInitiatorInfo>::iterator initiatorIt = m_initiatorMap.find(resource);
        if (initiatorIt != m_initiatorMap.end()) {
            ASSERT(document());
            Document* initiatorDocument = document();
            if (resource->type() == CachedResource::MainResource)
                initiatorDocument = document()->parentDocument();
            ASSERT(initiatorDocument);
            const CachedResourceInitiatorInfo& info = initiatorIt->value;
            initiatorDocument->domWindow()->performance()->addResourceTiming(info.name, initiatorDocument, resource->resourceRequest(), resource->response(), info.startTime, resource->loadFinishTime());
            m_initiatorMap.remove(initiatorIt);
        }
    }

    if (frame())
        frame()->loader()->loadDone();
    performPostLoadActions();

    if (!m_garbageCollectDocumentResourcesTimer.isActive())
        m_garbageCollectDocumentResourcesTimer.startOneShot(0);
}

// Garbage collecting m_documentResources is a workaround for the
// CachedResourceHandles on the RHS being strong references. Ideally this
// would be a weak map, however CachedResourceHandles perform additional
// bookkeeping on CachedResources, so instead pseudo-GC them -- when the
// reference count reaches 1, m_documentResources is the only reference, so
// remove it from the map.
void CachedResourceLoader::garbageCollectDocumentResourcesTimerFired(Timer<CachedResourceLoader>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_garbageCollectDocumentResourcesTimer);
    garbageCollectDocumentResources();
}

void CachedResourceLoader::garbageCollectDocumentResources()
{
    typedef Vector<String, 10> StringVector;
    StringVector resourcesToDelete;

    for (DocumentResourceMap::iterator it = m_documentResources.begin(); it != m_documentResources.end(); ++it) {
        if (it->value->hasOneHandle())
            resourcesToDelete.append(it->key);
    }

    for (StringVector::const_iterator it = resourcesToDelete.begin(); it != resourcesToDelete.end(); ++it)
        m_documentResources.remove(*it);
}

void CachedResourceLoader::performPostLoadActions()
{
    checkForPendingPreloads();
}

void CachedResourceLoader::notifyLoadedFromMemoryCache(CachedResource* resource)
{
    if (!frame() || resource->status() != CachedResource::Cached || m_validatedURLs.contains(resource->url()))
        return;

    // FIXME: If the WebKit client changes or cancels the request, WebCore does not respect this and continues the load.
    frame()->loader()->loadedResourceFromMemoryCache(resource);
}

void CachedResourceLoader::incrementRequestCount(const CachedResource* res)
{
    if (res->ignoreForRequestCount())
        return;

    ++m_requestCount;
}

void CachedResourceLoader::decrementRequestCount(const CachedResource* res)
{
    if (res->ignoreForRequestCount())
        return;

    --m_requestCount;
    ASSERT(m_requestCount > -1);
}

void CachedResourceLoader::preload(CachedResource::Type type, CachedResourceRequest& request, const String& charset)
{
    bool delaySubresourceLoad = true;
    delaySubresourceLoad = false;
    if (delaySubresourceLoad) {
        bool hasRendering = m_document->body() && m_document->body()->renderer();
        bool canBlockParser = type == CachedResource::Script || type == CachedResource::CSSStyleSheet;
        if (!hasRendering && !canBlockParser) {
            // Don't preload subresources that can't block the parser before we have something to draw.
            // This helps prevent preloads from delaying first display when bandwidth is limited.
            PendingPreload pendingPreload = { type, request, charset };
            m_pendingPreloads.append(pendingPreload);
            return;
        }
    }
    requestPreload(type, request, charset);
}

void CachedResourceLoader::checkForPendingPreloads() 
{
    if (m_pendingPreloads.isEmpty() || !m_document->body() || !m_document->body()->renderer())
        return;
    while (!m_pendingPreloads.isEmpty()) {
        PendingPreload preload = m_pendingPreloads.takeFirst();
        // Don't request preload if the resource already loaded normally (this will result in double load if the page is being reloaded with cached results ignored).
        if (!cachedResource(preload.m_request.resourceRequest().url()))
            requestPreload(preload.m_type, preload.m_request, preload.m_charset);
    }
    m_pendingPreloads.clear();
}

void CachedResourceLoader::requestPreload(CachedResource::Type type, CachedResourceRequest& request, const String& charset)
{
    String encoding;
    if (type == CachedResource::Script || type == CachedResource::CSSStyleSheet)
        encoding = charset.isEmpty() ? m_document->charset() : charset;

    request.setCharset(encoding);
    request.setForPreload(true);

    CachedResourceHandle<CachedResource> resource = requestResource(type, request);
    if (!resource || (m_preloads && m_preloads->contains(resource.get())))
        return;
    resource->increasePreloadCount();

    if (!m_preloads)
        m_preloads = adoptPtr(new ListHashSet<CachedResource*>);
    m_preloads->add(resource.get());

#if PRELOAD_DEBUG
    printf("PRELOADING %s\n",  resource->url().latin1().data());
#endif
}

bool CachedResourceLoader::isPreloaded(const String& urlString) const
{
    const KURL& url = m_document->completeURL(urlString);

    if (m_preloads) {
        ListHashSet<CachedResource*>::iterator end = m_preloads->end();
        for (ListHashSet<CachedResource*>::iterator it = m_preloads->begin(); it != end; ++it) {
            CachedResource* resource = *it;
            if (resource->url() == url)
                return true;
        }
    }

    Deque<PendingPreload>::const_iterator dequeEnd = m_pendingPreloads.end();
    for (Deque<PendingPreload>::const_iterator it = m_pendingPreloads.begin(); it != dequeEnd; ++it) {
        PendingPreload pendingPreload = *it;
        if (pendingPreload.m_request.resourceRequest().url() == url)
            return true;
    }
    return false;
}

void CachedResourceLoader::clearPreloads()
{
#if PRELOAD_DEBUG
    printPreloadStats();
#endif
    if (!m_preloads)
        return;

    ListHashSet<CachedResource*>::iterator end = m_preloads->end();
    for (ListHashSet<CachedResource*>::iterator it = m_preloads->begin(); it != end; ++it) {
        CachedResource* res = *it;
        res->decreasePreloadCount();
        bool deleted = res->deleteIfPossible();
        if (!deleted && res->preloadResult() == CachedResource::PreloadNotReferenced)
            memoryCache()->remove(res);
    }
    m_preloads.clear();
}

void CachedResourceLoader::clearPendingPreloads()
{
    m_pendingPreloads.clear();
}

#if PRELOAD_DEBUG
void CachedResourceLoader::printPreloadStats()
{
    unsigned scripts = 0;
    unsigned scriptMisses = 0;
    unsigned stylesheets = 0;
    unsigned stylesheetMisses = 0;
    unsigned images = 0;
    unsigned imageMisses = 0;
    ListHashSet<CachedResource*>::iterator end = m_preloads.end();
    for (ListHashSet<CachedResource*>::iterator it = m_preloads.begin(); it != end; ++it) {
        CachedResource* res = *it;
        if (res->preloadResult() == CachedResource::PreloadNotReferenced)
            printf("!! UNREFERENCED PRELOAD %s\n", res->url().latin1().data());
        else if (res->preloadResult() == CachedResource::PreloadReferencedWhileComplete)
            printf("HIT COMPLETE PRELOAD %s\n", res->url().latin1().data());
        else if (res->preloadResult() == CachedResource::PreloadReferencedWhileLoading)
            printf("HIT LOADING PRELOAD %s\n", res->url().latin1().data());
        
        if (res->type() == CachedResource::Script) {
            scripts++;
            if (res->preloadResult() < CachedResource::PreloadReferencedWhileLoading)
                scriptMisses++;
        } else if (res->type() == CachedResource::CSSStyleSheet) {
            stylesheets++;
            if (res->preloadResult() < CachedResource::PreloadReferencedWhileLoading)
                stylesheetMisses++;
        } else {
            images++;
            if (res->preloadResult() < CachedResource::PreloadReferencedWhileLoading)
                imageMisses++;
        }
        
        if (res->errorOccurred())
            memoryCache()->remove(res);
        
        res->decreasePreloadCount();
    }
    m_preloads.clear();
    
    if (scripts)
        printf("SCRIPTS: %d (%d hits, hit rate %d%%)\n", scripts, scripts - scriptMisses, (scripts - scriptMisses) * 100 / scripts);
    if (stylesheets)
        printf("STYLESHEETS: %d (%d hits, hit rate %d%%)\n", stylesheets, stylesheets - stylesheetMisses, (stylesheets - stylesheetMisses) * 100 / stylesheets);
    if (images)
        printf("IMAGES:  %d (%d hits, hit rate %d%%)\n", images, images - imageMisses, (images - imageMisses) * 100 / images);
}
#endif

void CachedResourceLoader::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::Loader);
    info.addMember(m_documentResources, "documentResources");
    info.addMember(m_document, "document");
    info.addMember(m_documentLoader, "documentLoader");
    info.addMember(m_validatedURLs, "validatedURLs");
    info.addMember(m_preloads, "preloads");
    info.addMember(m_pendingPreloads, "pendingPreloads");
    info.addMember(m_garbageCollectDocumentResourcesTimer, "garbageCollectDocumentResourcesTimer");
}

const ResourceLoaderOptions& CachedResourceLoader::defaultCachedResourceOptions()
{
    DEFINE_STATIC_LOCAL(ResourceLoaderOptions, options, (SendCallbacks, SniffContent, BufferData, AllowStoredCredentials, ClientRequestedCredentials, AskClientForCrossOriginCredentials, DoSecurityCheck, CheckContentSecurityPolicy, UseDefaultOriginRestrictionsForType));
    return options;
}

}
