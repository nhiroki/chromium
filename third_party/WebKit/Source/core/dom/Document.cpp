/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2008, 2009, 2011, 2012 Google Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) Research In Motion Limited 2010-2011. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "core/dom/Document.h"

#include "HTMLElementFactory.h"
#include "HTMLNames.h"
#include "RuntimeEnabledFeatures.h"
#include "SVGElementFactory.h"
#include "SVGNames.h"
#include "XMLNSNames.h"
#include "XMLNames.h"
#include "bindings/v8/CustomElementConstructorBuilder.h"
#include "bindings/v8/Dictionary.h"
#include "bindings/v8/ScriptController.h"
#include "core/accessibility/AXObjectCache.h"
#include "core/animation/DocumentTimeline.h"
#include "core/css/CSSStyleDeclaration.h"
#include "core/css/CSSStyleSheet.h"
#include "core/css/FontLoader.h"
#include "core/css/MediaQueryMatcher.h"
#include "core/css/StylePropertySet.h"
#include "core/css/StyleSheetContents.h"
#include "core/css/StyleSheetList.h"
#include "core/css/resolver/StyleResolver.h"
#include "core/dom/Attr.h"
#include "core/dom/CDATASection.h"
#include "core/dom/Comment.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/CustomElementRegistrationContext.h"
#include "core/dom/DOMImplementation.h"
#include "core/dom/DOMNamedFlowCollection.h"
#include "core/dom/DocumentEventQueue.h"
#include "core/dom/DocumentFragment.h"
#include "core/dom/DocumentLifecycleObserver.h"
#include "core/dom/DocumentMarkerController.h"
#include "core/dom/DocumentSharedObjectPool.h"
#include "core/dom/DocumentStyleSheetCollection.h"
#include "core/dom/DocumentType.h"
#include "core/dom/Element.h"
#include "core/dom/Event.h"
#include "core/dom/EventFactory.h"
#include "core/dom/EventListener.h"
#include "core/dom/EventNames.h"
#include "core/dom/ExceptionCode.h"
#include "core/dom/ExceptionCodePlaceholder.h"
#include "core/dom/HashChangeEvent.h"
#include "core/dom/NamedFlowCollection.h"
#include "core/dom/NodeFilter.h"
#include "core/dom/NodeIterator.h"
#include "core/dom/NodeRareData.h"
#include "core/dom/NodeRenderingTraversal.h"
#include "core/dom/NodeTraversal.h"
#include "core/dom/NodeWithIndex.h"
#include "core/dom/PageTransitionEvent.h"
#include "core/dom/PopStateEvent.h"
#include "core/dom/ProcessingInstruction.h"
#include "core/dom/QualifiedName.h"
#include "core/dom/RequestAnimationFrameCallback.h"
#include "core/dom/ScopedEventQueue.h"
#include "core/dom/ScriptRunner.h"
#include "core/dom/ScriptedAnimationController.h"
#include "core/dom/SelectorQuery.h"
#include "core/dom/TouchList.h"
#include "core/dom/TransformSource.h"
#include "core/dom/TreeWalker.h"
#include "core/dom/UserActionElementSet.h"
#include "core/dom/VisitedLinkState.h"
#include "core/dom/shadow/ElementShadow.h"
#include "core/dom/shadow/ShadowRoot.h"
#include "core/editing/Editor.h"
#include "core/editing/FrameSelection.h"
#include "core/html/FormController.h"
#include "core/html/HTMLAllCollection.h"
#include "core/html/HTMLAnchorElement.h"
#include "core/html/HTMLCanvasElement.h"
#include "core/html/HTMLCollection.h"
#include "core/html/HTMLDocument.h"
#include "core/html/HTMLFrameOwnerElement.h"
#include "core/html/HTMLHeadElement.h"
#include "core/html/HTMLHtmlElement.h"
#include "core/html/HTMLIFrameElement.h"
#include "core/html/HTMLImport.h"
#include "core/html/HTMLLinkElement.h"
#include "core/html/HTMLNameCollection.h"
#include "core/html/HTMLScriptElement.h"
#include "core/html/HTMLStyleElement.h"
#include "core/html/HTMLTitleElement.h"
#include "core/html/PluginDocument.h"
#include "core/html/parser/HTMLDocumentParser.h"
#include "core/html/parser/HTMLParserIdioms.h"
#include "core/html/parser/NestingLevelIncrementer.h"
#include "core/inspector/InspectorCounters.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "core/inspector/ScriptCallStack.h"
#include "core/loader/CookieJar.h"
#include "core/loader/DocumentLoader.h"
#include "core/loader/FrameLoader.h"
#include "core/loader/ImageLoader.h"
#include "core/loader/Prerenderer.h"
#include "core/loader/TextResourceDecoder.h"
#include "core/loader/cache/CachedResourceLoader.h"
#include "core/page/Chrome.h"
#include "core/page/ChromeClient.h"
#include "core/page/ContentSecurityPolicy.h"
#include "core/page/DOMSecurityPolicy.h"
#include "core/page/DOMWindow.h"
#include "core/page/EventHandler.h"
#include "core/page/Frame.h"
#include "core/page/FrameTree.h"
#include "core/page/FrameView.h"
#include "core/page/History.h"
#include "core/page/MouseEventWithHitTestResults.h"
#include "core/page/Page.h"
#include "core/page/PageConsole.h"
#include "core/page/PointerLockController.h"
#include "core/page/Settings.h"
#include "core/page/ValidationMessageClient.h"
#include "core/page/animation/AnimationController.h"
#include "core/page/scrolling/ScrollingCoordinator.h"
#include "core/platform/DateComponents.h"
#include "core/platform/HistogramSupport.h"
#include "core/platform/Language.h"
#include "core/platform/Timer.h"
#include "core/platform/chromium/TraceEvent.h"
#include "core/platform/network/HTTPParsers.h"
#include "core/platform/text/PlatformLocale.h"
#include "core/platform/text/SegmentedString.h"
#include "core/rendering/HitTestRequest.h"
#include "core/rendering/HitTestResult.h"
#include "core/rendering/RenderArena.h"
#include "core/rendering/RenderView.h"
#include "core/rendering/RenderWidget.h"
#include "core/rendering/TextAutosizer.h"
#include "core/svg/SVGDocumentExtensions.h"
#include "core/svg/SVGStyleElement.h"
#include "core/workers/SharedWorkerRepository.h"
#include "core/xml/XSLTProcessor.h"
#include "core/xml/parser/XMLDocumentParser.h"
#include "weborigin/SchemeRegistry.h"
#include "weborigin/SecurityOrigin.h"
#include "wtf/CurrentTime.h"
#include "wtf/HashFunctions.h"
#include "wtf/MainThread.h"
#include "wtf/PassRefPtr.h"
#include "wtf/StdLibExtras.h"
#include "wtf/UnusedParam.h"
#include "wtf/text/StringBuffer.h"

using namespace std;
using namespace WTF;
using namespace Unicode;

namespace WebCore {

using namespace HTMLNames;

static const double cDefaultIncrementalRenderingSuppressionTimeoutInSeconds = 5;

static const unsigned cMaxWriteRecursionDepth = 21;

// This amount of time must have elapsed before we will even consider scheduling a layout without a delay.
// FIXME: For faster machines this value can really be lowered to 200.  250 is adequate, but a little high
// for dual G5s. :)
static const int cLayoutScheduleThreshold = 250;

// DOM Level 2 says (letters added):
//
// a) Name start characters must have one of the categories Ll, Lu, Lo, Lt, Nl.
// b) Name characters other than Name-start characters must have one of the categories Mc, Me, Mn, Lm, or Nd.
// c) Characters in the compatibility area (i.e. with character code greater than #xF900 and less than #xFFFE) are not allowed in XML names.
// d) Characters which have a font or compatibility decomposition (i.e. those with a "compatibility formatting tag" in field 5 of the database -- marked by field 5 beginning with a "<") are not allowed.
// e) The following characters are treated as name-start characters rather than name characters, because the property file classifies them as Alphabetic: [#x02BB-#x02C1], #x0559, #x06E5, #x06E6.
// f) Characters #x20DD-#x20E0 are excluded (in accordance with Unicode, section 5.14).
// g) Character #x00B7 is classified as an extender, because the property list so identifies it.
// h) Character #x0387 is added as a name character, because #x00B7 is its canonical equivalent.
// i) Characters ':' and '_' are allowed as name-start characters.
// j) Characters '-' and '.' are allowed as name characters.
//
// It also contains complete tables. If we decide it's better, we could include those instead of the following code.

static inline bool isValidNameStart(UChar32 c)
{
    // rule (e) above
    if ((c >= 0x02BB && c <= 0x02C1) || c == 0x559 || c == 0x6E5 || c == 0x6E6)
        return true;

    // rule (i) above
    if (c == ':' || c == '_')
        return true;

    // rules (a) and (f) above
    const uint32_t nameStartMask = Letter_Lowercase | Letter_Uppercase | Letter_Other | Letter_Titlecase | Number_Letter;
    if (!(Unicode::category(c) & nameStartMask))
        return false;

    // rule (c) above
    if (c >= 0xF900 && c < 0xFFFE)
        return false;

    // rule (d) above
    DecompositionType decompType = decompositionType(c);
    if (decompType == DecompositionFont || decompType == DecompositionCompat)
        return false;

    return true;
}

static inline bool isValidNamePart(UChar32 c)
{
    // rules (a), (e), and (i) above
    if (isValidNameStart(c))
        return true;

    // rules (g) and (h) above
    if (c == 0x00B7 || c == 0x0387)
        return true;

    // rule (j) above
    if (c == '-' || c == '.')
        return true;

    // rules (b) and (f) above
    const uint32_t otherNamePartMask = Mark_NonSpacing | Mark_Enclosing | Mark_SpacingCombining | Letter_Modifier | Number_DecimalDigit;
    if (!(Unicode::category(c) & otherNamePartMask))
        return false;

    // rule (c) above
    if (c >= 0xF900 && c < 0xFFFE)
        return false;

    // rule (d) above
    DecompositionType decompType = decompositionType(c);
    if (decompType == DecompositionFont || decompType == DecompositionCompat)
        return false;

    return true;
}

static bool shouldInheritSecurityOriginFromOwner(const KURL& url)
{
    // http://www.whatwg.org/specs/web-apps/current-work/#origin-0
    //
    // If a Document has the address "about:blank"
    //     The origin of the Document is the origin it was assigned when its browsing context was created.
    //
    // Note: We generalize this to all "blank" URLs and invalid URLs because we
    // treat all of these URLs as about:blank.
    //
    return url.isEmpty() || url.isBlankURL();
}

static Widget* widgetForElement(Element* focusedElement)
{
    if (!focusedElement)
        return 0;
    RenderObject* renderer = focusedElement->renderer();
    if (!renderer || !renderer->isWidget())
        return 0;
    return toRenderWidget(renderer)->widget();
}

static bool acceptsEditingFocus(Element* element)
{
    ASSERT(element);
    ASSERT(element->rendererIsEditable());

    Element* root = element->rootEditableElement();
    Frame* frame = element->document()->frame();
    if (!frame || !root)
        return false;

    return frame->editor()->shouldBeginEditing(rangeOfContents(root).get());
}

static bool canAccessAncestor(const SecurityOrigin* activeSecurityOrigin, Frame* targetFrame)
{
    // targetFrame can be 0 when we're trying to navigate a top-level frame
    // that has a 0 opener.
    if (!targetFrame)
        return false;

    const bool isLocalActiveOrigin = activeSecurityOrigin->isLocal();
    for (Frame* ancestorFrame = targetFrame; ancestorFrame; ancestorFrame = ancestorFrame->tree()->parent()) {
        Document* ancestorDocument = ancestorFrame->document();
        // FIXME: Should be an ASSERT? Frames should alway have documents.
        if (!ancestorDocument)
            return true;

        const SecurityOrigin* ancestorSecurityOrigin = ancestorDocument->securityOrigin();
        if (activeSecurityOrigin->canAccess(ancestorSecurityOrigin))
            return true;

        // Allow file URL descendant navigation even when allowFileAccessFromFileURLs is false.
        // FIXME: It's a bit strange to special-case local origins here. Should we be doing
        // something more general instead?
        if (isLocalActiveOrigin && ancestorSecurityOrigin->isLocal())
            return true;
    }

    return false;
}

static void printNavigationErrorMessage(Frame* frame, const KURL& activeURL, const char* reason)
{
    String message = "Unsafe JavaScript attempt to initiate navigation for frame with URL '" + frame->document()->url().string() + "' from frame with URL '" + activeURL.string() + "'. " + reason + "\n";

    // FIXME: should we print to the console of the document performing the navigation instead?
    frame->domWindow()->printErrorMessage(message);
}

uint64_t Document::s_globalTreeVersion = 0;

// This class should be passed only to Document::postTask.
class CheckFocusedElementTask FINAL : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<CheckFocusedElementTask> create()
    {
        return adoptPtr(new CheckFocusedElementTask());
    }
    virtual ~CheckFocusedElementTask() { }

private:
    CheckFocusedElementTask() { }
    virtual void performTask(ScriptExecutionContext* context) OVERRIDE
    {
        ASSERT(context->isDocument());
        Document* document = toDocument(context);
        document->didRunCheckFocusedElementTask();
        Element* element = document->focusedElement();
        if (!element)
            return;
        if (document->childNeedsStyleRecalc())
            return;
        if (element->renderer() && element->renderer()->needsLayout())
            return;
        if (!element->isFocusable())
            document->setFocusedElement(0);
    }
};

Document::Document(const DocumentInit& initializer, DocumentClassFlags documentClasses)
    : ContainerNode(0, CreateDocument)
    , TreeScope(this)
    , m_styleResolverThrowawayTimer(this, &Document::styleResolverThrowawayTimerFired)
    , m_lastStyleResolverAccessTime(0)
    , m_didCalculateStyleResolver(false)
    , m_ignorePendingStylesheets(false)
    , m_needsNotifyRemoveAllPendingStylesheet(false)
    , m_hasNodesWithPlaceholderStyle(false)
    , m_pendingSheetLayout(NoLayoutWithPendingSheets)
    , m_frame(initializer.frame())
    , m_domWindow(0)
    , m_import(initializer.import())
    , m_activeParserCount(0)
    , m_contextFeatures(ContextFeatures::defaultSwitch())
    , m_wellFormed(false)
    , m_printing(false)
    , m_paginatedForScreen(false)
    , m_ignoreAutofocus(false)
    , m_compatibilityMode(NoQuirksMode)
    , m_compatibilityModeLocked(false)
    , m_didPostCheckFocusedElementTask(false)
    , m_domTreeVersion(++s_globalTreeVersion)
    , m_listenerTypes(0)
    , m_mutationObserverTypes(0)
    , m_styleSheetCollection(DocumentStyleSheetCollection::create(this))
    , m_visitedLinkState(VisitedLinkState::create(this))
    , m_visuallyOrdered(false)
    , m_readyState(Complete)
    , m_bParsing(false)
    , m_styleRecalcTimer(this, &Document::styleRecalcTimerFired)
    , m_inStyleRecalc(false)
    , m_closeAfterStyleRecalc(false)
    , m_gotoAnchorNeededAfterStylesheetsLoad(false)
    , m_pendingStyleRecalcShouldForce(false)
    , m_containsValidityStyleRules(false)
    , m_updateFocusAppearanceRestoresSelection(false)
    , m_ignoreDestructiveWriteCount(0)
    , m_titleSetExplicitly(false)
    , m_markers(adoptPtr(new DocumentMarkerController))
    , m_updateFocusAppearanceTimer(this, &Document::updateFocusAppearanceTimerFired)
    , m_cssTarget(0)
    , m_processingLoadEvent(false)
    , m_loadEventFinished(false)
    , m_startTime(currentTime())
    , m_overMinimumLayoutThreshold(false)
    , m_scriptRunner(ScriptRunner::create(this))
    , m_xmlVersion("1.0")
    , m_xmlStandalone(StandaloneUnspecified)
    , m_hasXMLDeclaration(0)
    , m_designMode(inherit)
    , m_hasAnnotatedRegions(false)
    , m_annotatedRegionsDirty(false)
    , m_useSecureKeyboardEntryWhenActive(false)
    , m_documentClasses(documentClasses)
    , m_isViewSource(false)
    , m_sawElementsInKnownNamespaces(false)
    , m_isSrcdocDocument(false)
    , m_renderer(0)
    , m_eventQueue(DocumentEventQueue::create(this))
    , m_weakFactory(this)
    , m_idAttributeName(idAttr)
    , m_hasFullscreenController(false)
    , m_loadEventDelayCount(0)
    , m_loadEventDelayTimer(this, &Document::loadEventDelayTimerFired)
    , m_referrerPolicy(ReferrerPolicyDefault)
    , m_directionSetOnDocumentElement(false)
    , m_writingModeSetOnDocumentElement(false)
    , m_writeRecursionIsTooDeep(false)
    , m_writeRecursionDepth(0)
    , m_wheelEventHandlerCount(0)
    , m_lastHandledUserGestureTimestamp(0)
    , m_prerenderer(Prerenderer::create(this))
    , m_textAutosizer(TextAutosizer::create(this))
    , m_pendingTasksTimer(this, &Document::pendingTasksTimerFired)
    , m_scheduledTasksAreSuspended(false)
    , m_sharedObjectPoolClearTimer(this, &Document::sharedObjectPoolClearTimerFired)
#ifndef NDEBUG
    , m_didDispatchViewportPropertiesChanged(false)
#endif
    , m_timeline(DocumentTimeline::create(this))
    , m_templateDocumentHost(0)
    , m_fontloader(0)
    , m_didAssociateFormControlsTimer(this, &Document::didAssociateFormControlsTimerFired)
{
    ScriptWrappable::init(this);

    if (m_frame) {
        provideContextFeaturesToDocumentFrom(this, m_frame->page());

        m_cachedResourceLoader = m_frame->loader()->activeDocumentLoader()->cachedResourceLoader();
    }

    if (!m_cachedResourceLoader)
        m_cachedResourceLoader = CachedResourceLoader::create(0);
    m_cachedResourceLoader->setDocument(this);

    // We depend on the url getting immediately set in subframes, but we
    // also depend on the url NOT getting immediately set in opened windows.
    // See fast/dom/early-frame-url.html
    // and fast/dom/location-new-window-no-crash.html, respectively.
    // FIXME: Can/should we unify this behavior?
    if (initializer.shouldSetURL())
        setURL(initializer.url());

    initSecurityContext(initializer);
    initDNSPrefetch();

    for (unsigned i = 0; i < WTF_ARRAY_LENGTH(m_nodeListCounts); i++)
        m_nodeListCounts[i] = 0;

    InspectorCounters::incrementCounter(InspectorCounters::DocumentCounter);

    bool shouldProcessCustomElements =
        (isHTMLDocument() || isXHTMLDocument())
        && RuntimeEnabledFeatures::customDOMElementsEnabled();
    m_registrationContext = shouldProcessCustomElements
        ? CustomElementRegistrationContext::create()
        : CustomElementRegistrationContext::nullRegistrationContext();
}

static void histogramMutationEventUsage(const unsigned short& listenerTypes)
{
    HistogramSupport::histogramEnumeration("DOMAPI.PerDocumentMutationEventUsage.DOMSubtreeModified", static_cast<bool>(listenerTypes & Document::DOMSUBTREEMODIFIED_LISTENER), 2);
    HistogramSupport::histogramEnumeration("DOMAPI.PerDocumentMutationEventUsage.DOMNodeInserted", static_cast<bool>(listenerTypes & Document::DOMNODEINSERTED_LISTENER), 2);
    HistogramSupport::histogramEnumeration("DOMAPI.PerDocumentMutationEventUsage.DOMNodeRemoved", static_cast<bool>(listenerTypes & Document::DOMNODEREMOVED_LISTENER), 2);
    HistogramSupport::histogramEnumeration("DOMAPI.PerDocumentMutationEventUsage.DOMNodeRemovedFromDocument", static_cast<bool>(listenerTypes & Document::DOMNODEREMOVEDFROMDOCUMENT_LISTENER), 2);
    HistogramSupport::histogramEnumeration("DOMAPI.PerDocumentMutationEventUsage.DOMNodeInsertedIntoDocument", static_cast<bool>(listenerTypes & Document::DOMNODEINSERTEDINTODOCUMENT_LISTENER), 2);
    HistogramSupport::histogramEnumeration("DOMAPI.PerDocumentMutationEventUsage.DOMCharacterDataModified", static_cast<bool>(listenerTypes & Document::DOMCHARACTERDATAMODIFIED_LISTENER), 2);
}

static bool isAttributeOnAllOwners(const WebCore::QualifiedName& attribute, const WebCore::QualifiedName& prefixedAttribute, const HTMLFrameOwnerElement* owner)
{
    if (!owner)
        return true;
    do {
        if (!(owner->hasAttribute(attribute) || owner->hasAttribute(prefixedAttribute)))
            return false;
    } while ((owner = owner->document()->ownerElement()));
    return true;
}

Document::~Document()
{
    ASSERT(!renderer());
    ASSERT(m_ranges.isEmpty());
    ASSERT(!m_styleRecalcTimer.isActive());
    ASSERT(!m_parentTreeScope);
    ASSERT(!hasGuardRefCount());

    if (m_templateDocument)
        m_templateDocument->setTemplateDocumentHost(0); // balanced in templateDocument().

    if (Document* ownerDocument = this->ownerDocument())
        ownerDocument->didRemoveEventTargetNode(this);

    m_scriptRunner.clear();

    histogramMutationEventUsage(m_listenerTypes);

    removeAllEventListeners();

    // Currently we believe that Document can never outlive the parser.
    // Although the Document may be replaced synchronously, DocumentParsers
    // generally keep at least one reference to an Element which would in turn
    // has a reference to the Document.  If you hit this ASSERT, then that
    // assumption is wrong.  DocumentParser::detach() should ensure that even
    // if the DocumentParser outlives the Document it won't cause badness.
    ASSERT(!m_parser || m_parser->refCount() == 1);
    detachParser();

    m_renderArena.clear();

    if (this == topDocument())
        clearAXObjectCache();

    m_decoder = 0;

    if (m_styleSheetList)
        m_styleSheetList->detachFromDocument();

    if (m_import) {
        m_import->wasDetachedFromDocument();
        m_import = 0;
    }

    m_styleSheetCollection.clear();

    if (m_elemSheet)
        m_elemSheet->clearOwnerNode();

    clearStyleResolver(); // We need to destory CSSFontSelector before destroying m_cachedResourceLoader.

    // It's possible for multiple Documents to end up referencing the same CachedResourceLoader (e.g., SVGImages
    // load the initial empty document and the SVGDocument with the same DocumentLoader).
    if (m_cachedResourceLoader->document() == this)
        m_cachedResourceLoader->setDocument(0);
    m_cachedResourceLoader.clear();

    // We must call clearRareData() here since a Document class inherits TreeScope
    // as well as Node. See a comment on TreeScope.h for the reason.
    if (hasRareData())
        clearRareData();

    ASSERT(!m_listsInvalidatedAtDocument.size());

    for (unsigned i = 0; i < WTF_ARRAY_LENGTH(m_nodeListCounts); i++)
        ASSERT(!m_nodeListCounts[i]);

    clearDocumentScope();

    InspectorCounters::decrementCounter(InspectorCounters::DocumentCounter);
}

void Document::dispose()
{
    ASSERT(!m_deletionHasBegun);
    // We must make sure not to be retaining any of our children through
    // these extra pointers or we will create a reference cycle.
    m_docType = 0;
    m_focusedElement = 0;
    m_hoverNode = 0;
    m_activeElement = 0;
    m_titleElement = 0;
    m_documentElement = 0;
    m_contextFeatures = ContextFeatures::defaultSwitch();
    m_userActionElements.documentDidRemoveLastRef();

    detachParser();

    m_registrationContext.clear();

    if (m_import) {
        m_import->wasDetachedFromDocument();
        m_import = 0;
    }

    // removeDetachedChildren() doesn't always unregister IDs,
    // so tear down scope information upfront to avoid having stale references in the map.
    destroyTreeScopeData();
    removeDetachedChildren();
    // removeDetachedChildren() can access FormController.
    m_formController.clear();

    m_markers->detach();

    m_cssCanvasElements.clear();

    // FIXME: consider using ActiveDOMObject.
    if (m_scriptedAnimationController)
        m_scriptedAnimationController->clearDocumentPointer();
    m_scriptedAnimationController.clear();

    if (svgExtensions())
        accessSVGExtensions()->pauseAnimations();

    lifecycleNotifier()->notifyDocumentWasDisposed();
}

Element* Document::getElementById(const AtomicString& id) const
{
    return TreeScope::getElementById(id);
}

SelectorQueryCache* Document::selectorQueryCache()
{
    if (!m_selectorQueryCache)
        m_selectorQueryCache = adoptPtr(new SelectorQueryCache());
    return m_selectorQueryCache.get();
}

MediaQueryMatcher* Document::mediaQueryMatcher()
{
    if (!m_mediaQueryMatcher)
        m_mediaQueryMatcher = MediaQueryMatcher::create(this);
    return m_mediaQueryMatcher.get();
}

void Document::setCompatibilityMode(CompatibilityMode mode)
{
    if (m_compatibilityModeLocked || mode == m_compatibilityMode)
        return;
    bool wasInQuirksMode = inQuirksMode();
    m_compatibilityMode = mode;
    selectorQueryCache()->invalidate();
    if (inQuirksMode() != wasInQuirksMode) {
        // All user stylesheets have to reparse using the different mode.
        m_styleSheetCollection->clearPageUserSheet();
        m_styleSheetCollection->invalidateInjectedStyleSheetCache();
    }
}

String Document::compatMode() const
{
    return inQuirksMode() ? "BackCompat" : "CSS1Compat";
}

void Document::setDocType(PassRefPtr<DocumentType> docType)
{
    // This should never be called more than once.
    ASSERT(!m_docType || !docType);
    m_docType = docType;
    if (m_docType) {
        this->adoptIfNeeded(m_docType.get());
        if (m_docType->publicId().startsWith("-//wapforum//dtd xhtml mobile 1.", /* caseSensitive */ false))
            processViewport("width=device-width, height=device-height", ViewportArguments::XHTMLMobileProfile);
    }
    // Doctype affects the interpretation of the stylesheets.
    clearStyleResolver();
}

DOMImplementation* Document::implementation()
{
    if (!m_implementation)
        m_implementation = DOMImplementation::create(this);
    return m_implementation.get();
}

bool Document::hasManifest() const
{
    return documentElement() && isHTMLHtmlElement(documentElement()) && documentElement()->hasAttribute(manifestAttr);
}

void Document::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    ContainerNode::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);

    Element* newDocumentElement = ElementTraversal::firstWithin(this);
    if (newDocumentElement == m_documentElement)
        return;
    m_documentElement = newDocumentElement;
    // The root style used for media query matching depends on the document element.
    clearStyleResolver();
}

PassRefPtr<Element> Document::createElement(const AtomicString& name, ExceptionCode& ec)
{
    if (!isValidName(name)) {
        ec = InvalidCharacterError;
        return 0;
    }

    if (isXHTMLDocument() || isHTMLDocument())
        return HTMLElementFactory::createHTMLElement(QualifiedName(nullAtom, isHTMLDocument() ? name.lower() : name, xhtmlNamespaceURI), this, 0, false);

    return createElement(QualifiedName(nullAtom, name, nullAtom), false);
}

PassRefPtr<Element> Document::createElement(const AtomicString& localName, const AtomicString& typeExtension, ExceptionCode& ec)
{
    if (!isValidName(localName)) {
        ec = InvalidCharacterError;
        return 0;
    }

    RefPtr<Element> element;

    if (CustomElementRegistrationContext::isCustomTagName(localName))
        element = registrationContext()->createCustomTagElement(this, QualifiedName(nullAtom, localName, xhtmlNamespaceURI));
    else
        element = createElement(localName, ec);

    if (!typeExtension.isNull() && !typeExtension.isEmpty())
        CustomElementRegistrationContext::setIsAttributeAndTypeExtension(element.get(), typeExtension);

    return element;
}

PassRefPtr<Element> Document::createElementNS(const AtomicString& namespaceURI, const String& qualifiedName, const AtomicString& typeExtension, ExceptionCode& ec)
{
    String prefix, localName;
    if (!parseQualifiedName(qualifiedName, prefix, localName, ec))
        return 0;

    QualifiedName qName(prefix, localName, namespaceURI);
    if (!hasValidNamespaceForElements(qName)) {
        ec = NamespaceError;
        return 0;
    }

    RefPtr<Element> element;
    if (CustomElementRegistrationContext::isCustomTagName(qName.localName()))
        element = registrationContext()->createCustomTagElement(this, qName);
    else
        element = createElementNS(namespaceURI, qualifiedName, ec);

    if (!typeExtension.isNull() && !typeExtension.isEmpty())
        CustomElementRegistrationContext::setIsAttributeAndTypeExtension(element.get(), typeExtension);

    return element;
}

ScriptValue Document::registerElement(WebCore::ScriptState* state, const AtomicString& name, ExceptionCode& ec)
{
    return registerElement(state, name, Dictionary(), ec);
}

ScriptValue Document::registerElement(WebCore::ScriptState* state, const AtomicString& name, const Dictionary& options, ExceptionCode& ec)
{
    CustomElementConstructorBuilder constructorBuilder(state, &options);
    registrationContext()->registerElement(this, &constructorBuilder, name, ec);
    return constructorBuilder.bindingsReturnValue();
}

void Document::setImport(HTMLImport* import)
{
    ASSERT(!m_import || !import);
    m_import = import;
}

void Document::didLoadAllImports()
{
    executeScriptsWaitingForResourcesIfNeeded();
}

bool Document::haveImportsLoaded() const
{
    return !m_import || m_import->haveChildrenLoaded();
}

PassRefPtr<DocumentFragment> Document::createDocumentFragment()
{
    return DocumentFragment::create(document());
}

PassRefPtr<Text> Document::createTextNode(const String& data)
{
    return Text::create(this, data);
}

PassRefPtr<Comment> Document::createComment(const String& data)
{
    return Comment::create(this, data);
}

PassRefPtr<CDATASection> Document::createCDATASection(const String& data, ExceptionCode& ec)
{
    if (isHTMLDocument()) {
        ec = NotSupportedError;
        return 0;
    }
    return CDATASection::create(this, data);
}

PassRefPtr<ProcessingInstruction> Document::createProcessingInstruction(const String& target, const String& data, ExceptionCode& ec)
{
    if (!isValidName(target)) {
        ec = InvalidCharacterError;
        return 0;
    }
    if (isHTMLDocument()) {
        ec = NotSupportedError;
        return 0;
    }
    return ProcessingInstruction::create(this, target, data);
}

PassRefPtr<Text> Document::createEditingTextNode(const String& text)
{
    return Text::createEditingText(this, text);
}

PassRefPtr<CSSStyleDeclaration> Document::createCSSStyleDeclaration()
{
    return MutableStylePropertySet::create()->ensureCSSStyleDeclaration();
}

PassRefPtr<Node> Document::importNode(Node* importedNode, bool deep, ExceptionCode& ec)
{
    ec = 0;

    if (!importedNode) {
        ec = NotSupportedError;
        return 0;
    }

    switch (importedNode->nodeType()) {
    case TEXT_NODE:
        return createTextNode(importedNode->nodeValue());
    case CDATA_SECTION_NODE:
        return createCDATASection(importedNode->nodeValue(), ec);
    case PROCESSING_INSTRUCTION_NODE:
        return createProcessingInstruction(importedNode->nodeName(), importedNode->nodeValue(), ec);
    case COMMENT_NODE:
        return createComment(importedNode->nodeValue());
    case ELEMENT_NODE: {
        Element* oldElement = toElement(importedNode);
        // FIXME: The following check might be unnecessary. Is it possible that
        // oldElement has mismatched prefix/namespace?
        if (!hasValidNamespaceForElements(oldElement->tagQName())) {
            ec = NamespaceError;
            return 0;
        }
        RefPtr<Element> newElement = createElement(oldElement->tagQName(), false);

        newElement->cloneDataFromElement(*oldElement);

        if (deep) {
            for (Node* oldChild = oldElement->firstChild(); oldChild; oldChild = oldChild->nextSibling()) {
                RefPtr<Node> newChild = importNode(oldChild, true, ec);
                if (ec)
                    return 0;
                newElement->appendChild(newChild.release(), ec);
                if (ec)
                    return 0;
            }
        }

        return newElement.release();
    }
    case ATTRIBUTE_NODE:
        return Attr::create(this, QualifiedName(nullAtom, toAttr(importedNode)->name(), nullAtom), toAttr(importedNode)->value());
    case DOCUMENT_FRAGMENT_NODE: {
        if (importedNode->isShadowRoot()) {
            // ShadowRoot nodes should not be explicitly importable.
            // Either they are imported along with their host node, or created implicitly.
            break;
        }
        DocumentFragment* oldFragment = static_cast<DocumentFragment*>(importedNode);
        RefPtr<DocumentFragment> newFragment = createDocumentFragment();
        if (deep) {
            for (Node* oldChild = oldFragment->firstChild(); oldChild; oldChild = oldChild->nextSibling()) {
                RefPtr<Node> newChild = importNode(oldChild, true, ec);
                if (ec)
                    return 0;
                newFragment->appendChild(newChild.release(), ec);
                if (ec)
                    return 0;
            }
        }

        return newFragment.release();
    }
    case ENTITY_NODE:
    case NOTATION_NODE:
        // FIXME: It should be possible to import these node types, however in DOM3 the DocumentType is readonly, so there isn't much sense in doing that.
        // Ability to add these imported nodes to a DocumentType will be considered for addition to a future release of the DOM.
    case DOCUMENT_NODE:
    case DOCUMENT_TYPE_NODE:
    case XPATH_NAMESPACE_NODE:
        break;
    }
    ec = NotSupportedError;
    return 0;
}

PassRefPtr<Node> Document::adoptNode(PassRefPtr<Node> source, ExceptionCode& ec)
{
    if (!source) {
        ec = NotSupportedError;
        return 0;
    }

    EventQueueScope scope;

    switch (source->nodeType()) {
    case ENTITY_NODE:
    case NOTATION_NODE:
    case DOCUMENT_NODE:
    case DOCUMENT_TYPE_NODE:
    case XPATH_NAMESPACE_NODE:
        ec = NotSupportedError;
        return 0;
    case ATTRIBUTE_NODE: {
        Attr* attr = toAttr(source.get());
        if (attr->ownerElement())
            attr->ownerElement()->removeAttributeNode(attr, ec);
        attr->setSpecified(true);
        break;
    }
    default:
        if (source->isShadowRoot()) {
            // ShadowRoot cannot disconnect itself from the host node.
            ec = HierarchyRequestError;
            return 0;
        }

        if (source->isFrameOwnerElement()) {
            HTMLFrameOwnerElement* frameOwnerElement = toFrameOwnerElement(source.get());
            if (frame() && frame()->tree()->isDescendantOf(frameOwnerElement->contentFrame())) {
                ec = HierarchyRequestError;
                return 0;
            }
        }
        if (source->parentNode()) {
            source->parentNode()->removeChild(source.get(), ec);
            if (ec)
                return 0;
        }
    }

    this->adoptIfNeeded(source.get());

    return source;
}

bool Document::hasValidNamespaceForElements(const QualifiedName& qName)
{
    // These checks are from DOM Core Level 2, createElementNS
    // http://www.w3.org/TR/DOM-Level-2-Core/core.html#ID-DocCrElNS
    if (!qName.prefix().isEmpty() && qName.namespaceURI().isNull()) // createElementNS(null, "html:div")
        return false;
    if (qName.prefix() == xmlAtom && qName.namespaceURI() != XMLNames::xmlNamespaceURI) // createElementNS("http://www.example.com", "xml:lang")
        return false;

    // Required by DOM Level 3 Core and unspecified by DOM Level 2 Core:
    // http://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407/core.html#ID-DocCrElNS
    // createElementNS("http://www.w3.org/2000/xmlns/", "foo:bar"), createElementNS(null, "xmlns:bar")
    if ((qName.prefix() == xmlnsAtom && qName.namespaceURI() != XMLNSNames::xmlnsNamespaceURI) || (qName.prefix() != xmlnsAtom && qName.namespaceURI() == XMLNSNames::xmlnsNamespaceURI))
        return false;

    return true;
}

bool Document::hasValidNamespaceForAttributes(const QualifiedName& qName)
{
    // Spec: DOM Level 2 Core: http://www.w3.org/TR/DOM-Level-2-Core/core.html#ID-ElSetAttrNS
    if (qName.prefix().isEmpty() && qName.localName() == xmlnsAtom) {
        // Note: The case of an "xmlns" qualified name with a namespace of
        // xmlnsNamespaceURI is specifically allowed (See <http://www.w3.org/2000/xmlns/>).
        return qName.namespaceURI() == XMLNSNames::xmlnsNamespaceURI;
    }
    return hasValidNamespaceForElements(qName);
}

// FIXME: This should really be in a possible ElementFactory class
PassRefPtr<Element> Document::createElement(const QualifiedName& qName, bool createdByParser)
{
    RefPtr<Element> e;

    // FIXME: Use registered namespaces and look up in a hash to find the right factory.
    if (qName.namespaceURI() == xhtmlNamespaceURI)
        e = HTMLElementFactory::createHTMLElement(qName, this, 0, createdByParser);
    else if (qName.namespaceURI() == SVGNames::svgNamespaceURI)
        e = SVGElementFactory::createSVGElement(qName, this, createdByParser);

    if (e)
        m_sawElementsInKnownNamespaces = true;
    else
        e = Element::create(qName, document());

    // <image> uses imgTag so we need a special rule.
    ASSERT((qName.matches(imageTag) && e->tagQName().matches(imgTag) && e->tagQName().prefix() == qName.prefix()) || qName == e->tagQName());

    return e.release();
}

bool Document::regionBasedColumnsEnabled() const
{
    return settings() && settings()->regionBasedColumnsEnabled();
}

bool Document::cssStickyPositionEnabled() const
{
    return settings() && settings()->cssStickyPositionEnabled();
}

bool Document::cssCompositingEnabled() const
{
    return RuntimeEnabledFeatures::cssCompositingEnabled();
}

PassRefPtr<DOMNamedFlowCollection> Document::webkitGetNamedFlows()
{
    if (!RuntimeEnabledFeatures::cssRegionsEnabled() || !renderer())
        return 0;

    updateStyleIfNeeded();

    return namedFlows()->createCSSOMSnapshot();
}

NamedFlowCollection* Document::namedFlows()
{
    if (!m_namedFlows)
        m_namedFlows = NamedFlowCollection::create(this);

    return m_namedFlows.get();
}

PassRefPtr<Element> Document::createElementNS(const String& namespaceURI, const String& qualifiedName, ExceptionCode& ec)
{
    String prefix, localName;
    if (!parseQualifiedName(qualifiedName, prefix, localName, ec))
        return 0;

    QualifiedName qName(prefix, localName, namespaceURI);
    if (!hasValidNamespaceForElements(qName)) {
        ec = NamespaceError;
        return 0;
    }

    return createElement(qName, false);
}

String Document::readyState() const
{
    DEFINE_STATIC_LOCAL(const String, loading, (ASCIILiteral("loading")));
    DEFINE_STATIC_LOCAL(const String, interactive, (ASCIILiteral("interactive")));
    DEFINE_STATIC_LOCAL(const String, complete, (ASCIILiteral("complete")));

    switch (m_readyState) {
    case Loading:
        return loading;
    case Interactive:
        return interactive;
    case Complete:
        return complete;
    }

    ASSERT_NOT_REACHED();
    return String();
}

void Document::setReadyState(ReadyState readyState)
{
    if (readyState == m_readyState)
        return;

    switch (readyState) {
    case Loading:
        if (!m_documentTiming.domLoading)
            m_documentTiming.domLoading = monotonicallyIncreasingTime();
        break;
    case Interactive:
        if (!m_documentTiming.domInteractive)
            m_documentTiming.domInteractive = monotonicallyIncreasingTime();
        break;
    case Complete:
        if (!m_documentTiming.domComplete)
            m_documentTiming.domComplete = monotonicallyIncreasingTime();
        break;
    }

    m_readyState = readyState;
    dispatchEvent(Event::create(eventNames().readystatechangeEvent, false, false));
}

String Document::encoding() const
{
    // TextEncoding::domName() returns a char*, no need to allocate a new
    // String for it each time.
    // FIXME: We should fix TextEncoding to speak AtomicString anyway.
    if (TextResourceDecoder* d = decoder())
        return AtomicString(d->encoding().domName());
    return String();
}

String Document::defaultCharset() const
{
    if (Settings* settings = this->settings())
        return settings->defaultTextEncodingName();
    return String();
}

void Document::setCharset(const String& charset)
{
    if (!decoder())
        return;
    decoder()->setEncoding(charset, TextResourceDecoder::UserChosenEncoding);
}

void Document::setContentLanguage(const String& language)
{
    if (m_contentLanguage == language)
        return;
    m_contentLanguage = language;

    // Document's style depends on the content language.
    scheduleForcedStyleRecalc();
}

void Document::setXMLVersion(const String& version, ExceptionCode& ec)
{
    if (!implementation()->hasFeature("XML", String())) {
        ec = NotSupportedError;
        return;
    }

    if (!XMLDocumentParser::supportsXMLVersion(version)) {
        ec = NotSupportedError;
        return;
    }

    m_xmlVersion = version;
}

void Document::setXMLStandalone(bool standalone, ExceptionCode& ec)
{
    if (!implementation()->hasFeature("XML", String())) {
        ec = NotSupportedError;
        return;
    }

    m_xmlStandalone = standalone ? Standalone : NotStandalone;
}

KURL Document::baseURI() const
{
    return m_baseURL;
}

void Document::setContent(const String& content)
{
    open();
    // FIXME: This should probably use insert(), but that's (intentionally)
    // not implemented for the XML parser as it's normally synonymous with
    // document.write(). append() will end up yielding, but close() will
    // pump the tokenizer syncrhonously and finish the parse.
    m_parser->pinToMainThread();
    m_parser->append(content.impl());
    close();
}

String Document::suggestedMIMEType() const
{
    if (isXHTMLDocument())
        return "application/xhtml+xml";
    if (isSVGDocument())
        return "image/svg+xml";
    if (xmlStandalone())
        return "text/xml";
    if (isHTMLDocument())
        return "text/html";

    if (DocumentLoader* documentLoader = loader())
        return documentLoader->responseMIMEType();
    return String();
}

Element* Document::elementFromPoint(int x, int y) const
{
    if (!renderer())
        return 0;

    return TreeScope::elementFromPoint(x, y);
}

PassRefPtr<Range> Document::caretRangeFromPoint(int x, int y)
{
    if (!renderer())
        return 0;
    LayoutPoint localPoint;
    Node* node = nodeFromPoint(this, x, y, &localPoint);
    if (!node)
        return 0;

    Node* shadowAncestorNode = ancestorInThisScope(node);
    if (shadowAncestorNode != node) {
        unsigned offset = shadowAncestorNode->nodeIndex();
        ContainerNode* container = shadowAncestorNode->parentNode();
        return Range::create(this, container, offset, container, offset);
    }

    RenderObject* renderer = node->renderer();
    if (!renderer)
        return 0;
    VisiblePosition visiblePosition = renderer->positionForPoint(localPoint);
    if (visiblePosition.isNull())
        return 0;

    Position rangeCompliantPosition = visiblePosition.deepEquivalent().parentAnchoredEquivalent();
    return Range::create(this, rangeCompliantPosition, rangeCompliantPosition);
}

/*
 * Performs three operations:
 *  1. Convert control characters to spaces
 *  2. Trim leading and trailing spaces
 *  3. Collapse internal whitespace.
 */
template <typename CharacterType>
static inline StringWithDirection canonicalizedTitle(Document* document, const StringWithDirection& titleWithDirection)
{
    const String& title = titleWithDirection.string();
    const CharacterType* characters = title.getCharacters<CharacterType>();
    unsigned length = title.length();
    unsigned i;

    StringBuffer<CharacterType> buffer(length);
    unsigned builderIndex = 0;

    // Skip leading spaces and leading characters that would convert to spaces
    for (i = 0; i < length; ++i) {
        CharacterType c = characters[i];
        if (!(c <= 0x20 || c == 0x7F))
            break;
    }

    if (i == length)
        return StringWithDirection();

    // Replace control characters with spaces, and backslashes with currency symbols, and collapse whitespace.
    bool previousCharWasWS = false;
    for (; i < length; ++i) {
        CharacterType c = characters[i];
        if (c <= 0x20 || c == 0x7F || (WTF::Unicode::category(c) & (WTF::Unicode::Separator_Line | WTF::Unicode::Separator_Paragraph))) {
            if (previousCharWasWS)
                continue;
            buffer[builderIndex++] = ' ';
            previousCharWasWS = true;
        } else {
            buffer[builderIndex++] = c;
            previousCharWasWS = false;
        }
    }

    // Strip trailing spaces
    while (builderIndex > 0) {
        --builderIndex;
        if (buffer[builderIndex] != ' ')
            break;
    }

    if (!builderIndex && buffer[builderIndex] == ' ')
        return StringWithDirection();

    buffer.shrink(builderIndex + 1);

    // Replace the backslashes with currency symbols if the encoding requires it.
    document->displayBufferModifiedByEncoding(buffer.characters(), buffer.length());

    return StringWithDirection(String::adopt(buffer), titleWithDirection.direction());
}

void Document::updateTitle(const StringWithDirection& title)
{
    if (m_rawTitle == title)
        return;

    m_rawTitle = title;

    if (m_rawTitle.string().isEmpty())
        m_title = StringWithDirection();
    else {
        if (m_rawTitle.string().is8Bit())
            m_title = canonicalizedTitle<LChar>(this, m_rawTitle);
        else
            m_title = canonicalizedTitle<UChar>(this, m_rawTitle);
    }
    if (Frame* f = frame())
        f->loader()->setTitle(m_title);
}

void Document::setTitle(const String& title)
{
    // Title set by JavaScript -- overrides any title elements.
    m_titleSetExplicitly = true;
    if (!isHTMLDocument() && !isXHTMLDocument())
        m_titleElement = 0;
    else if (!m_titleElement) {
        if (HTMLElement* headElement = head()) {
            m_titleElement = createElement(titleTag, false);
            headElement->appendChild(m_titleElement, ASSERT_NO_EXCEPTION);
        }
    }

    // The DOM API has no method of specifying direction, so assume LTR.
    updateTitle(StringWithDirection(title, LTR));

    if (m_titleElement) {
        ASSERT(isHTMLTitleElement(m_titleElement.get()));
        if (isHTMLTitleElement(m_titleElement.get()))
            toHTMLTitleElement(m_titleElement.get())->setText(title);
    }
}

void Document::setTitleElement(const StringWithDirection& title, Element* titleElement)
{
    if (titleElement != m_titleElement) {
        if (m_titleElement || m_titleSetExplicitly)
            // Only allow the first title element to change the title -- others have no effect.
            return;
        m_titleElement = titleElement;
    }

    updateTitle(title);
}

void Document::removeTitle(Element* titleElement)
{
    if (m_titleElement != titleElement)
        return;

    m_titleElement = 0;
    m_titleSetExplicitly = false;

    // Update title based on first title element in the head, if one exists.
    if (HTMLElement* headElement = head()) {
        for (Node* e = headElement->firstChild(); e; e = e->nextSibling()) {
            if (isHTMLTitleElement(e)) {
                HTMLTitleElement* titleElement = toHTMLTitleElement(e);
                setTitleElement(titleElement->textWithDirection(), titleElement);
                break;
            }
        }
    }

    if (!m_titleElement)
        updateTitle(StringWithDirection());
}

PageVisibilityState Document::visibilityState() const
{
    // The visibility of the document is inherited from the visibility of the
    // page. If there is no page associated with the document, we will assume
    // that the page is hidden, as specified by the spec:
    // http://dvcs.w3.org/hg/webperf/raw-file/tip/specs/PageVisibility/Overview.html#dom-document-hidden
    if (!m_frame || !m_frame->page())
        return PageVisibilityStateHidden;
    return m_frame->page()->visibilityState();
}

String Document::webkitVisibilityState() const
{
    return pageVisibilityStateString(visibilityState());
}

bool Document::webkitHidden() const
{
    return visibilityState() != PageVisibilityStateVisible;
}

void Document::dispatchVisibilityStateChangeEvent()
{
    dispatchEvent(Event::create(eventNames().webkitvisibilitychangeEvent, false, false));
}

DOMSecurityPolicy* Document::securityPolicy()
{
    if (!m_domSecurityPolicy)
        m_domSecurityPolicy = DOMSecurityPolicy::create(this);
    return m_domSecurityPolicy.get();
}

String Document::nodeName() const
{
    return "#document";
}

Node::NodeType Document::nodeType() const
{
    return DOCUMENT_NODE;
}

FormController* Document::formController()
{
    if (!m_formController)
        m_formController = FormController::create();
    return m_formController.get();
}

Vector<String> Document::formElementsState() const
{
    if (!m_formController)
        return Vector<String>();
    return m_formController->formElementsState();
}

void Document::setStateForNewFormElements(const Vector<String>& stateVector)
{
    if (!stateVector.size() && !m_formController)
        return;
    formController()->setStateForNewFormElements(stateVector);
}

FrameView* Document::view() const
{
    return m_frame ? m_frame->view() : 0;
}

Page* Document::page() const
{
    return m_frame ? m_frame->page() : 0;
}

Settings* Document::settings() const
{
    return m_frame ? m_frame->settings() : 0;
}

PassRefPtr<Range> Document::createRange()
{
    return Range::create(this);
}

PassRefPtr<NodeIterator> Document::createNodeIterator(Node* root, ExceptionCode& ec)
{
    // FIXME: Probably this should be handled within the bindings layer and TypeError should be thrown.
    if (!root) {
        ec = NotSupportedError;
        return 0;
    }
    return NodeIterator::create(root, NodeFilter::SHOW_ALL, PassRefPtr<NodeFilter>());
}

PassRefPtr<NodeIterator> Document::createNodeIterator(Node* root, unsigned whatToShow, ExceptionCode& ec)
{
    if (!root) {
        ec = NotSupportedError;
        return 0;
    }
    // FIXME: It might be a good idea to emit a warning if |whatToShow| contains a bit that is not defined in
    // NodeFilter.
    return NodeIterator::create(root, whatToShow, PassRefPtr<NodeFilter>());
}

PassRefPtr<NodeIterator> Document::createNodeIterator(Node* root, unsigned whatToShow, PassRefPtr<NodeFilter> filter, ExceptionCode& ec)
{
    if (!root) {
        ec = NotSupportedError;
        return 0;
    }
    // FIXME: Ditto.
    return NodeIterator::create(root, whatToShow, filter);
}

PassRefPtr<NodeIterator> Document::createNodeIterator(Node* root, unsigned whatToShow, PassRefPtr<NodeFilter> filter, bool expandEntityReferences, ExceptionCode& ec)
{
    if (!root) {
        ec = NotSupportedError;
        return 0;
    }
    // FIXME: Warn if |expandEntityReferences| is specified. This optional argument is deprecated in DOM4.
    UNUSED_PARAM(expandEntityReferences);
    return NodeIterator::create(root, whatToShow, filter);
}

PassRefPtr<TreeWalker> Document::createTreeWalker(Node* root, ExceptionCode& ec)
{
    if (!root) {
        ec = NotSupportedError;
        return 0;
    }
    return TreeWalker::create(root, NodeFilter::SHOW_ALL, PassRefPtr<NodeFilter>());
}

PassRefPtr<TreeWalker> Document::createTreeWalker(Node* root, unsigned whatToShow, ExceptionCode& ec)
{
    if (!root) {
        ec = NotSupportedError;
        return 0;
    }
    return TreeWalker::create(root, whatToShow, PassRefPtr<NodeFilter>());
}

PassRefPtr<TreeWalker> Document::createTreeWalker(Node* root, unsigned whatToShow, PassRefPtr<NodeFilter> filter, ExceptionCode& ec)
{
    if (!root) {
        ec = NotSupportedError;
        return 0;
    }
    return TreeWalker::create(root, whatToShow, filter);
}

PassRefPtr<TreeWalker> Document::createTreeWalker(Node* root, unsigned whatToShow, PassRefPtr<NodeFilter> filter, bool expandEntityReferences, ExceptionCode& ec)
{
    UNUSED_PARAM(expandEntityReferences);
    if (!root) {
        ec = NotSupportedError;
        return 0;
    }
    return TreeWalker::create(root, whatToShow, filter);
}

void Document::scheduleForcedStyleRecalc()
{
    m_pendingStyleRecalcShouldForce = true;
    scheduleStyleRecalc();
}

void Document::scheduleStyleRecalc()
{
    if (shouldDisplaySeamlesslyWithParent()) {
        // When we're seamless, our parent document manages our style recalcs.
        ownerElement()->setNeedsStyleRecalc();
        ownerElement()->document()->scheduleStyleRecalc();
        return;
    }

    if (m_styleRecalcTimer.isActive())
        return;

    ASSERT(childNeedsStyleRecalc() || m_pendingStyleRecalcShouldForce);

    m_styleRecalcTimer.startOneShot(0);

    InspectorInstrumentation::didScheduleStyleRecalculation(this);
}

void Document::unscheduleStyleRecalc()
{
    ASSERT(!childNeedsStyleRecalc());

    m_styleRecalcTimer.stop();
    m_pendingStyleRecalcShouldForce = false;
}

bool Document::hasPendingStyleRecalc() const
{
    return m_styleRecalcTimer.isActive() && !m_inStyleRecalc;
}

bool Document::hasPendingForcedStyleRecalc() const
{
    return m_styleRecalcTimer.isActive() && m_pendingStyleRecalcShouldForce;
}

void Document::styleRecalcTimerFired(Timer<Document>*)
{
    updateStyleIfNeeded();
}

bool Document::childNeedsAndNotInStyleRecalc()
{
    return childNeedsStyleRecalc() && !m_inStyleRecalc;
}

void Document::recalcStyle(StyleChange change)
{
    // we should not enter style recalc while painting
    ASSERT(!view() || !view()->isPainting());
    if (view() && view()->isPainting())
        return;

    if (m_inStyleRecalc)
        return; // Guard against re-entrancy. -dwh

    TRACE_EVENT0("webkit", "Document::recalcStyle");
    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "RecalcStyle");

    // FIXME: We should update style on our ancestor chain before proceeding (especially for seamless),
    // however doing so currently causes several tests to crash, as Frame::setDocument calls Document::attach
    // before setting the DOMWindow on the Frame, or the SecurityOrigin on the document. The attach, in turn
    // resolves style (here) and then when we resolve style on the parent chain, we may end up
    // re-attaching our containing iframe, which when asked HTMLFrameElementBase::isURLAllowed
    // hits a null-dereference due to security code always assuming the document has a SecurityOrigin.

    if (m_styleSheetCollection->needsUpdateActiveStylesheetsOnStyleRecalc())
        m_styleSheetCollection->updateActiveStyleSheets(FullStyleUpdate);

    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willRecalculateStyle(this);

    if (m_elemSheet && m_elemSheet->contents()->usesRemUnits())
        m_styleSheetCollection->setUsesRemUnit(true);

    m_inStyleRecalc = true;
    {
        PostAttachCallbackDisabler disabler(this);
        WidgetHierarchyUpdatesSuspensionScope suspendWidgetHierarchyUpdates;

        RefPtr<FrameView> frameView = view();
        if (frameView) {
            frameView->pauseScheduledEvents();
            frameView->beginDeferredRepaints();
        }

        ASSERT(!renderer() || renderArena());
        if (!renderer() || !renderArena())
            goto bailOut;

        if (m_pendingStyleRecalcShouldForce)
            change = Force;

        // Recalculating the root style (on the document) is not needed in the common case.
        if ((change == Force) || (shouldDisplaySeamlesslyWithParent() && (change >= Inherit))) {
            // style selector may set this again during recalc
            m_hasNodesWithPlaceholderStyle = false;

            RefPtr<RenderStyle> documentStyle = StyleResolver::styleForDocument(this, m_styleResolver ? m_styleResolver->fontSelector() : 0);
            StyleChange ch = Node::diff(documentStyle.get(), renderer()->style(), this);
            if (ch != NoChange)
                renderer()->setStyle(documentStyle.release());
        }

        for (Node* n = firstChild(); n; n = n->nextSibling()) {
            if (!n->isElementNode())
                continue;
            Element* element = toElement(n);
            if (shouldRecalcStyle(change, element))
                element->recalcStyle(change);
        }

        if (view())
            view()->updateCompositingLayersAfterStyleChange();

    bailOut:
        clearNeedsStyleRecalc();
        clearChildNeedsStyleRecalc();
        unscheduleStyleRecalc();

        m_inStyleRecalc = false;

        // Pseudo element removal and similar may only work with these flags still set. Reset them after the style recalc.
        if (m_styleResolver)
            m_styleSheetCollection->resetCSSFeatureFlags(m_styleResolver->ruleFeatureSet());

        if (frameView) {
            frameView->resumeScheduledEvents();
            frameView->endDeferredRepaints();
        }
    }

    // If we wanted to call implicitClose() during recalcStyle, do so now that we're finished.
    if (m_closeAfterStyleRecalc) {
        m_closeAfterStyleRecalc = false;
        implicitClose();
    }

    STYLE_STATS_PRINT();
    STYLE_STATS_CLEAR();

    InspectorInstrumentation::didRecalculateStyle(cookie);

    // As a result of the style recalculation, the currently hovered element might have been
    // detached (for example, by setting display:none in the :hover style), schedule another mouseMove event
    // to check if any other elements ended up under the mouse pointer due to re-layout.
    if (hoverNode() && !hoverNode()->renderer() && frame())
        frame()->eventHandler()->dispatchFakeMouseMoveEventSoon();
}

void Document::updateStyleIfNeeded()
{
    ASSERT(isMainThread());
    ASSERT(!view() || (!view()->isInLayout() && !view()->isPainting()));

    if (!m_pendingStyleRecalcShouldForce && !childNeedsStyleRecalc())
        return;

    AnimationUpdateBlock animationUpdateBlock(m_frame ? m_frame->animation() : 0);
    recalcStyle(NoChange);
}

void Document::updateStyleForNodeIfNeeded(Node* node)
{
    if (!hasPendingForcedStyleRecalc() && !childNeedsStyleRecalc() && !needsStyleRecalc())
        return;

    bool needsStyleRecalc = hasPendingForcedStyleRecalc();
    for (Node* ancestor = node; ancestor && !needsStyleRecalc; ancestor = ancestor->parentOrShadowHostNode())
        needsStyleRecalc = ancestor->needsStyleRecalc();
    if (needsStyleRecalc)
        updateStyleIfNeeded();
}

void Document::updateLayout()
{
    ASSERT(isMainThread());

    FrameView* frameView = view();
    if (frameView && frameView->isInLayout()) {
        // View layout should not be re-entrant.
        ASSERT_NOT_REACHED();
        return;
    }

    if (Element* oe = ownerElement())
        oe->document()->updateLayout();

    updateStyleIfNeeded();

    StackStats::LayoutCheckPoint layoutCheckPoint;

    // Only do a layout if changes have occurred that make it necessary.
    if (frameView && renderer() && (frameView->layoutPending() || renderer()->needsLayout()))
        frameView->layout();

    // FIXME: Using a Task doesn't look a good idea.
    if (m_focusedElement && !m_didPostCheckFocusedElementTask) {
        postTask(CheckFocusedElementTask::create());
        m_didPostCheckFocusedElementTask = true;
    }
}

// FIXME: This is a bad idea and needs to be removed eventually.
// Other browsers load stylesheets before they continue parsing the web page.
// Since we don't, we can run JavaScript code that needs answers before the
// stylesheets are loaded. Doing a layout ignoring the pending stylesheets
// lets us get reasonable answers. The long term solution to this problem is
// to instead suspend JavaScript execution.
void Document::updateLayoutIgnorePendingStylesheets()
{
    bool oldIgnore = m_ignorePendingStylesheets;

    if (!haveStylesheetsLoaded()) {
        m_ignorePendingStylesheets = true;
        // FIXME: We are willing to attempt to suppress painting with outdated style info only once.  Our assumption is that it would be
        // dangerous to try to stop it a second time, after page content has already been loaded and displayed
        // with accurate style information.  (Our suppression involves blanking the whole page at the
        // moment.  If it were more refined, we might be able to do something better.)
        // It's worth noting though that this entire method is a hack, since what we really want to do is
        // suspend JS instead of doing a layout with inaccurate information.
        HTMLElement* bodyElement = body();
        if (bodyElement && !bodyElement->renderer() && m_pendingSheetLayout == NoLayoutWithPendingSheets) {
            m_pendingSheetLayout = DidLayoutWithPendingSheets;
            styleResolverChanged(RecalcStyleImmediately);
        } else if (m_hasNodesWithPlaceholderStyle)
            // If new nodes have been added or style recalc has been done with style sheets still pending, some nodes
            // may not have had their real style calculated yet. Normally this gets cleaned when style sheets arrive
            // but here we need up-to-date style immediately.
            recalcStyle(Force);
    }

    updateLayout();

    m_ignorePendingStylesheets = oldIgnore;
}

PassRefPtr<RenderStyle> Document::styleForElementIgnoringPendingStylesheets(Element* element)
{
    ASSERT_ARG(element, element->document() == this);
    TemporaryChange<bool> ignoreStyleSheets(m_ignorePendingStylesheets, true);
    return styleResolver()->styleForElement(element, element->parentNode() ? element->parentNode()->computedStyle() : 0);
}

PassRefPtr<RenderStyle> Document::styleForPage(int pageIndex)
{
    return styleResolver()->styleForPage(pageIndex);
}

bool Document::isPageBoxVisible(int pageIndex)
{
    return styleForPage(pageIndex)->visibility() != HIDDEN; // display property doesn't apply to @page.
}

void Document::pageSizeAndMarginsInPixels(int pageIndex, IntSize& pageSize, int& marginTop, int& marginRight, int& marginBottom, int& marginLeft)
{
    RefPtr<RenderStyle> style = styleForPage(pageIndex);
    RenderView* view = renderView();

    int width = pageSize.width();
    int height = pageSize.height();
    switch (style->pageSizeType()) {
    case PAGE_SIZE_AUTO:
        break;
    case PAGE_SIZE_AUTO_LANDSCAPE:
        if (width < height)
            std::swap(width, height);
        break;
    case PAGE_SIZE_AUTO_PORTRAIT:
        if (width > height)
            std::swap(width, height);
        break;
    case PAGE_SIZE_RESOLVED: {
        LengthSize size = style->pageSize();
        ASSERT(size.width().isFixed());
        ASSERT(size.height().isFixed());
        width = valueForLength(size.width(), 0, view);
        height = valueForLength(size.height(), 0, view);
        break;
    }
    default:
        ASSERT_NOT_REACHED();
    }
    pageSize = IntSize(width, height);

    // The percentage is calculated with respect to the width even for margin top and bottom.
    // http://www.w3.org/TR/CSS2/box.html#margin-properties
    marginTop = style->marginTop().isAuto() ? marginTop : intValueForLength(style->marginTop(), width, view);
    marginRight = style->marginRight().isAuto() ? marginRight : intValueForLength(style->marginRight(), width, view);
    marginBottom = style->marginBottom().isAuto() ? marginBottom : intValueForLength(style->marginBottom(), width, view);
    marginLeft = style->marginLeft().isAuto() ? marginLeft : intValueForLength(style->marginLeft(), width, view);
}

void Document::setIsViewSource(bool isViewSource)
{
    m_isViewSource = isViewSource;
    if (!m_isViewSource)
        return;

    setSecurityOrigin(SecurityOrigin::createUnique());
    didUpdateSecurityOrigin();
}

void Document::createStyleResolver()
{
    bool matchAuthorAndUserStyles = true;
    if (Settings* docSettings = settings())
        matchAuthorAndUserStyles = docSettings->authorAndUserStylesEnabled();
    m_styleResolver = adoptPtr(new StyleResolver(this, matchAuthorAndUserStyles));
    m_styleSheetCollection->combineCSSFeatureFlags(m_styleResolver->ruleFeatureSet());
}

void Document::clearStyleResolver()
{
    m_styleResolver.clear();
}

void Document::attach(const AttachContext& context)
{
    ASSERT(!attached());
    ASSERT(!m_axObjectCache || this != topDocument());

    if (!m_renderArena)
        m_renderArena = RenderArena::create();

    // Create the rendering tree
    setRenderer(new (m_renderArena.get()) RenderView(this));
    renderView()->setIsInWindow(true);

    recalcStyle(Force);

    ContainerNode::attach(context);
}

void Document::detach(const AttachContext& context)
{
    ASSERT(attached());

    if (page()) {
        page()->pointerLockController()->documentDetached(this);
        if (ValidationMessageClient* client = page()->validationMessageClient())
            client->documentDetached(*this);
    }

    if (this == topDocument())
        clearAXObjectCache();

    stopActiveDOMObjects();
    m_eventQueue->close();

    // FIXME: consider using ActiveDOMObject.
    if (m_scriptedAnimationController)
        m_scriptedAnimationController->clearDocumentPointer();
    m_scriptedAnimationController.clear();

    if (svgExtensions())
        accessSVGExtensions()->pauseAnimations();

    RenderObject* render = renderer();

    documentWillBecomeInactive();

    SharedWorkerRepository::documentDetached(this);

    if (m_frame) {
        FrameView* view = m_frame->view();
        if (view)
            view->detachCustomScrollbars();
    }

    // indicate destruction mode,  i.e. attached() but renderer == 0
    setRenderer(0);

    m_hoverNode = 0;
    m_focusedElement = 0;
    m_activeElement = 0;

    ContainerNode::detach(context);

    unscheduleStyleRecalc();

    if (render)
        render->destroy();

    if (m_touchEventTargets && m_touchEventTargets->size() && parentDocument())
        parentDocument()->didRemoveEventTargetNode(this);

    // This is required, as our Frame might delete itself as soon as it detaches
    // us. However, this violates Node::detach() semantics, as it's never
    // possible to re-attach. Eventually Document::detach() should be renamed,
    // or this setting of the frame to 0 could be made explicit in each of the
    // callers of Document::detach().
    m_frame = 0;
    m_renderArena.clear();

    if (m_mediaQueryMatcher)
        m_mediaQueryMatcher->documentDestroyed();

    lifecycleNotifier()->notifyDocumentWasDetached();
}

void Document::prepareForDestruction()
{
    disconnectDescendantFrames();
    if (DOMWindow* window = this->domWindow())
        window->willDetachDocumentFromFrame();
    detach();
}

void Document::removeAllEventListeners()
{
    EventTarget::removeAllEventListeners();

    if (DOMWindow* domWindow = this->domWindow())
        domWindow->removeAllEventListeners();
    for (Node* node = firstChild(); node; node = NodeTraversal::next(node))
        node->removeAllEventListeners();
}

void Document::suspendActiveDOMObjects(ActiveDOMObject::ReasonForSuspension why)
{
    ScriptExecutionContext::suspendActiveDOMObjects(why);
}

void Document::resumeActiveDOMObjects()
{
    ScriptExecutionContext::resumeActiveDOMObjects();
}

void Document::clearAXObjectCache()
{
    ASSERT(topDocument() == this);
    // Clear the cache member variable before calling delete because attempts
    // are made to access it during destruction.
    m_axObjectCache.clear();
}

AXObjectCache* Document::existingAXObjectCache() const
{
    if (!AXObjectCache::accessibilityEnabled())
        return 0;

    // If the renderer is gone then we are in the process of destruction.
    // This method will be called before m_frame = 0.
    if (!topDocument()->renderer())
        return 0;

    return topDocument()->m_axObjectCache.get();
}

AXObjectCache* Document::axObjectCache() const
{
    if (!AXObjectCache::accessibilityEnabled())
        return 0;

    // The only document that actually has a AXObjectCache is the top-level
    // document.  This is because we need to be able to get from any WebCoreAXObject
    // to any other WebCoreAXObject on the same page.  Using a single cache allows
    // lookups across nested webareas (i.e. multiple documents).
    Document* topDocument = this->topDocument();

    // If the document has already been detached, do not make a new axObjectCache.
    if (!topDocument->renderer())
        return 0;

    ASSERT(topDocument == this || !m_axObjectCache);
    if (!topDocument->m_axObjectCache)
        topDocument->m_axObjectCache = adoptPtr(new AXObjectCache(topDocument));
    return topDocument->m_axObjectCache.get();
}

void Document::setVisuallyOrdered()
{
    m_visuallyOrdered = true;
    // FIXME: How is possible to not have a renderer here?
    if (renderer())
        renderer()->style()->setRTLOrdering(VisualOrder);
    scheduleForcedStyleRecalc();
}

PassRefPtr<DocumentParser> Document::createParser()
{
    if (isHTMLDocument() || (RuntimeEnabledFeatures::parseSVGAsHTMLEnabled() && isSVGDocument())) {
        bool reportErrors = InspectorInstrumentation::collectingHTMLParseErrors(this->page());
        return HTMLDocumentParser::create(this, reportErrors);
    }
    // FIXME: this should probably pass the frame instead
    return XMLDocumentParser::create(this, view());
}

bool Document::isFrameSet() const
{
    if (!isHTMLDocument())
        return false;
    HTMLElement* bodyElement = body();
    return bodyElement && bodyElement->hasTagName(framesetTag);
}

ScriptableDocumentParser* Document::scriptableDocumentParser() const
{
    return parser() ? parser()->asScriptableDocumentParser() : 0;
}

void Document::open(Document* ownerDocument)
{
    if (ownerDocument) {
        setURL(ownerDocument->url());
        m_cookieURL = ownerDocument->cookieURL();
        setSecurityOrigin(ownerDocument->securityOrigin());
    }

    if (m_frame) {
        if (ScriptableDocumentParser* parser = scriptableDocumentParser()) {
            if (parser->isParsing()) {
                // FIXME: HTML5 doesn't tell us to check this, it might not be correct.
                if (parser->isExecutingScript())
                    return;

                if (!parser->wasCreatedByScript() && parser->hasInsertionPoint())
                    return;
            }
        }

        if (m_frame->loader()->state() == FrameStateProvisional)
            m_frame->loader()->stopAllLoaders();
    }

    removeAllEventListeners();
    implicitOpen();
    if (ScriptableDocumentParser* parser = scriptableDocumentParser())
        parser->setWasCreatedByScript(true);

    if (m_frame)
        m_frame->loader()->didExplicitOpen();
}

void Document::detachParser()
{
    if (!m_parser)
        return;
    m_parser->detach();
    m_parser.clear();
}

void Document::cancelParsing()
{
    if (!m_parser)
        return;

    // We have to clear the parser to avoid possibly triggering
    // the onload handler when closing as a side effect of a cancel-style
    // change, such as opening a new document or closing the window while
    // still parsing
    detachParser();
    explicitClose();
}

PassRefPtr<DocumentParser> Document::implicitOpen()
{
    cancelParsing();

    removeChildren();
    ASSERT(!m_focusedElement);

    setCompatibilityMode(NoQuirksMode);

    // Documents rendered seamlessly should start out requiring a stylesheet
    // collection update in order to ensure they inherit all the relevant data
    // from their parent.
    if (shouldDisplaySeamlesslyWithParent())
        styleResolverChanged(DeferRecalcStyle);

    m_parser = createParser();
    setParsing(true);
    setReadyState(Loading);

    return m_parser;
}

HTMLElement* Document::body() const
{
    if (!documentElement())
        return 0;

    for (Node* child = documentElement()->firstChild(); child; child = child->nextSibling()) {
        if (child->hasTagName(framesetTag) || child->hasTagName(bodyTag))
            return toHTMLElement(child);
    }

    return 0;
}

void Document::setBody(PassRefPtr<HTMLElement> prpNewBody, ExceptionCode& ec)
{
    RefPtr<HTMLElement> newBody = prpNewBody;

    if (!newBody || !documentElement()) {
        ec = HierarchyRequestError;
        return;
    }

    if (!newBody->hasTagName(bodyTag) && !newBody->hasTagName(framesetTag)) {
        ec = HierarchyRequestError;
        return;
    }

    HTMLElement* oldBody = body();
    if (oldBody == newBody)
        return;

    if (oldBody)
        documentElement()->replaceChild(newBody.release(), oldBody, ec, AttachLazily);
    else
        documentElement()->appendChild(newBody.release(), ec, AttachLazily);
}

HTMLHeadElement* Document::head()
{
    Node* de = documentElement();
    if (!de)
        return 0;

    for (Node* e = de->firstChild(); e; e = e->nextSibling())
        if (e->hasTagName(headTag))
            return static_cast<HTMLHeadElement*>(e);

    return 0;
}

void Document::close()
{
    // FIXME: We should follow the specification more closely:
    //        http://www.whatwg.org/specs/web-apps/current-work/#dom-document-close

    if (!scriptableDocumentParser() || !scriptableDocumentParser()->wasCreatedByScript() || !scriptableDocumentParser()->isParsing())
        return;

    explicitClose();
}

void Document::explicitClose()
{
    if (RefPtr<DocumentParser> parser = m_parser)
        parser->finish();

    if (!m_frame) {
        // Because we have no frame, we don't know if all loading has completed,
        // so we just call implicitClose() immediately. FIXME: This might fire
        // the load event prematurely <http://bugs.webkit.org/show_bug.cgi?id=14568>.
        implicitClose();
        return;
    }

    m_frame->loader()->checkCompleted();
}

void Document::implicitClose()
{
    // If we're in the middle of recalcStyle, we need to defer the close until the style information is accurate and all elements are re-attached.
    if (m_inStyleRecalc) {
        m_closeAfterStyleRecalc = true;
        return;
    }

    bool wasLocationChangePending = frame() && frame()->navigationScheduler()->locationChangePending();
    bool doload = !parsing() && m_parser && !m_processingLoadEvent && !wasLocationChangePending;

    if (!doload)
        return;

    // The call to dispatchWindowLoadEvent can detach the DOMWindow and cause it (and its
    // attached Document) to be destroyed.
    RefPtr<DOMWindow> protect(this->domWindow());

    m_processingLoadEvent = true;

    ScriptableDocumentParser* parser = scriptableDocumentParser();
    m_wellFormed = parser && parser->wellFormed();

    // We have to clear the parser, in case someone document.write()s from the
    // onLoad event handler, as in Radar 3206524.
    detachParser();

    Frame* f = frame();
    if (f && !RuntimeEnabledFeatures::webAnimationsCSSEnabled())
        f->animation()->resumeAnimationsForDocument(this);

    if (f && f->script()->canExecuteScripts(NotAboutToExecuteScript)) {
        ImageLoader::dispatchPendingBeforeLoadEvents();
        ImageLoader::dispatchPendingLoadEvents();
        ImageLoader::dispatchPendingErrorEvents();

        HTMLLinkElement::dispatchPendingLoadEvents();
        HTMLStyleElement::dispatchPendingLoadEvents();
    }

    // To align the HTML load event and the SVGLoad event for the outermost <svg> element, fire it from
    // here, instead of doing it from SVGElement::finishedParsingChildren (if externalResourcesRequired="false",
    // which is the default, for ='true' its fired at a later time, once all external resources finished loading).
    if (svgExtensions())
        accessSVGExtensions()->dispatchSVGLoadEventToOutermostSVGElements();

    dispatchWindowLoadEvent();
    enqueuePageshowEvent(PageshowEventNotPersisted);
    enqueuePopstateEvent(m_pendingStateObject ? m_pendingStateObject.release() : SerializedScriptValue::nullValue());

    if (f)
        f->loader()->handledOnloadEvents();

    // An event handler may have removed the frame
    if (!frame()) {
        m_processingLoadEvent = false;
        return;
    }

    // Make sure both the initial layout and reflow happen after the onload
    // fires. This will improve onload scores, and other browsers do it.
    // If they wanna cheat, we can too. -dwh

    if (frame()->navigationScheduler()->locationChangePending() && elapsedTime() < cLayoutScheduleThreshold) {
        // Just bail out. Before or during the onload we were shifted to another page.
        // The old i-Bench suite does this. When this happens don't bother painting or laying out.
        m_processingLoadEvent = false;
        view()->unscheduleRelayout();
        return;
    }

    frame()->loader()->checkCallImplicitClose();
    RenderObject* renderObject = renderer();

    // We used to force a synchronous display and flush here.  This really isn't
    // necessary and can in fact be actively harmful if pages are loading at a rate of > 60fps
    // (if your platform is syncing flushes and limiting them to 60fps).
    m_overMinimumLayoutThreshold = true;
    if (!ownerElement() || (ownerElement()->renderer() && !ownerElement()->renderer()->needsLayout())) {
        updateStyleIfNeeded();

        // Always do a layout after loading if needed.
        if (view() && renderObject && (!renderObject->firstChild() || renderObject->needsLayout()))
            view()->layout();
    }

    m_processingLoadEvent = false;

    if (f && renderObject && AXObjectCache::accessibilityEnabled()) {
        // The AX cache may have been cleared at this point, but we need to make sure it contains an
        // AX object to send the notification to. getOrCreate will make sure that an valid AX object
        // exists in the cache (we ignore the return value because we don't need it here). This is
        // only safe to call when a layout is not in progress, so it can not be used in postNotification.
        if (AXObjectCache* cache = axObjectCache()) {
            cache->getOrCreate(renderObject);
            if (this == topDocument()) {
                cache->postNotification(renderObject, AXObjectCache::AXLoadComplete, true);
            } else {
                // AXLoadComplete can only be posted on the top document, so if it's a document
                // in an iframe that just finished loading, post AXLayoutComplete instead.
                cache->postNotification(renderObject, AXObjectCache::AXLayoutComplete, true);
            }
        }
    }

    if (svgExtensions())
        accessSVGExtensions()->startAnimations();
}

void Document::setParsing(bool b)
{
    m_bParsing = b;

    if (m_bParsing && !m_sharedObjectPool)
        m_sharedObjectPool = DocumentSharedObjectPool::create();

    if (!m_bParsing && view())
        view()->scheduleRelayout();
}

bool Document::shouldScheduleLayout()
{
    // This function will only be called when FrameView thinks a layout is needed.
    // This enforces a couple extra rules.
    //
    //    (a) Only schedule a layout once the stylesheets are loaded.
    //    (b) Only schedule layout once we have a body element.

    return (haveStylesheetsLoaded() && body())
        || (documentElement() && !isHTMLHtmlElement(documentElement()));
}

bool Document::isLayoutTimerActive()
{
    return view() && view()->layoutPending() && !minimumLayoutDelay();
}

int Document::minimumLayoutDelay()
{
    if (m_overMinimumLayoutThreshold)
        return 0;

    int elapsed = elapsedTime();
    m_overMinimumLayoutThreshold = elapsed > cLayoutScheduleThreshold;

    // We'll want to schedule the timer to fire at the minimum layout threshold.
    return max(0, cLayoutScheduleThreshold - elapsed);
}

int Document::elapsedTime() const
{
    return static_cast<int>((currentTime() - m_startTime) * 1000);
}

void Document::write(const SegmentedString& text, Document* ownerDocument)
{
    NestingLevelIncrementer nestingLevelIncrementer(m_writeRecursionDepth);

    m_writeRecursionIsTooDeep = (m_writeRecursionDepth > 1) && m_writeRecursionIsTooDeep;
    m_writeRecursionIsTooDeep = (m_writeRecursionDepth > cMaxWriteRecursionDepth) || m_writeRecursionIsTooDeep;

    if (m_writeRecursionIsTooDeep)
       return;

    bool hasInsertionPoint = m_parser && m_parser->hasInsertionPoint();
    if (!hasInsertionPoint && m_ignoreDestructiveWriteCount)
        return;

    if (!hasInsertionPoint)
        open(ownerDocument);

    ASSERT(m_parser);
    m_parser->insert(text);
}

void Document::write(const String& text, Document* ownerDocument)
{
    write(SegmentedString(text), ownerDocument);
}

void Document::writeln(const String& text, Document* ownerDocument)
{
    write(text, ownerDocument);
    write("\n", ownerDocument);
}

const KURL& Document::virtualURL() const
{
    return m_url;
}

KURL Document::virtualCompleteURL(const String& url) const
{
    return completeURL(url);
}

double Document::timerAlignmentInterval() const
{
    Page* p = page();
    if (!p)
        return ScriptExecutionContext::timerAlignmentInterval();
    return p->timerAlignmentInterval();
}

EventTarget* Document::errorEventTarget()
{
    return domWindow();
}

void Document::logExceptionToConsole(const String& errorMessage, const String& sourceURL, int lineNumber, PassRefPtr<ScriptCallStack> callStack)
{
    addMessage(JSMessageSource, ErrorMessageLevel, errorMessage, sourceURL, lineNumber, callStack);
}

void Document::setURL(const KURL& url)
{
    const KURL& newURL = url.isEmpty() ? blankURL() : url;
    if (newURL == m_url)
        return;

    m_url = newURL;
    m_documentURI = m_url.string();
    updateBaseURL();
    contextFeatures()->urlDidChange(this);
}

void Document::updateBaseURL()
{
    KURL oldBaseURL = m_baseURL;
    // DOM 3 Core: When the Document supports the feature "HTML" [DOM Level 2 HTML], the base URI is computed using
    // first the value of the href attribute of the HTML BASE element if any, and the value of the documentURI attribute
    // from the Document interface otherwise.
    if (!m_baseElementURL.isEmpty())
        m_baseURL = m_baseElementURL;
    else if (!m_baseURLOverride.isEmpty())
        m_baseURL = m_baseURLOverride;
    else {
        // The documentURI attribute is read-only from JavaScript, but writable from Objective C, so we need to retain
        // this fallback behavior. We use a null base URL, since the documentURI attribute is an arbitrary string
        // and DOM 3 Core does not specify how it should be resolved.
        // FIXME: Now that we don't support Objective-C this can probably be removed.
        m_baseURL = KURL(ParsedURLString, documentURI());
    }
    selectorQueryCache()->invalidate();

    if (!m_baseURL.isValid())
        m_baseURL = KURL();

    if (m_elemSheet) {
        // Element sheet is silly. It never contains anything.
        ASSERT(!m_elemSheet->contents()->ruleCount());
        bool usesRemUnits = m_elemSheet->contents()->usesRemUnits();
        m_elemSheet = CSSStyleSheet::createInline(this, m_baseURL);
        // FIXME: So we are not really the parser. The right fix is to eliminate the element sheet completely.
        m_elemSheet->contents()->parserSetUsesRemUnits(usesRemUnits);
    }

    if (!equalIgnoringFragmentIdentifier(oldBaseURL, m_baseURL)) {
        // Base URL change changes any relative visited links.
        // FIXME: There are other URLs in the tree that would need to be re-evaluated on dynamic base URL change. Style should be invalidated too.
        for (Element* element = ElementTraversal::firstWithin(this); element; element = ElementTraversal::next(element)) {
            if (isHTMLAnchorElement(element))
                toHTMLAnchorElement(element)->invalidateCachedVisitedLinkHash();
        }
    }
}

void Document::setBaseURLOverride(const KURL& url)
{
    m_baseURLOverride = url;
    updateBaseURL();
}

void Document::processBaseElement()
{
    // Find the first href attribute in a base element and the first target attribute in a base element.
    const AtomicString* href = 0;
    const AtomicString* target = 0;
    for (Element* element = ElementTraversal::firstWithin(this); element && (!href || !target); element = ElementTraversal::next(element)) {
        if (element->hasTagName(baseTag)) {
            if (!href) {
                const AtomicString& value = element->fastGetAttribute(hrefAttr);
                if (!value.isNull())
                    href = &value;
            }
            if (!target) {
                const AtomicString& value = element->fastGetAttribute(targetAttr);
                if (!value.isNull())
                    target = &value;
            }
            if (contentSecurityPolicy()->isActive())
                UseCounter::count(this, UseCounter::ContentSecurityPolicyWithBaseElement);
        }
    }

    // FIXME: Since this doesn't share code with completeURL it may not handle encodings correctly.
    KURL baseElementURL;
    if (href) {
        String strippedHref = stripLeadingAndTrailingHTMLSpaces(*href);
        if (!strippedHref.isEmpty())
            baseElementURL = KURL(url(), strippedHref);
    }
    if (m_baseElementURL != baseElementURL && contentSecurityPolicy()->allowBaseURI(baseElementURL)) {
        m_baseElementURL = baseElementURL;
        updateBaseURL();
    }

    m_baseTarget = target ? *target : nullAtom;
}

String Document::userAgent(const KURL& url) const
{
    return frame() ? frame()->loader()->userAgent(url) : String();
}

void Document::disableEval(const String& errorMessage)
{
    if (!frame())
        return;

    frame()->script()->disableEval(errorMessage);
}

bool Document::canNavigate(Frame* targetFrame)
{
    if (!m_frame)
        return false;

    // FIXME: We shouldn't call this function without a target frame, but
    // fast/forms/submit-to-blank-multiple-times.html depends on this function
    // returning true when supplied with a 0 targetFrame.
    if (!targetFrame)
        return true;

    // Frame-busting is generally allowed, but blocked for sandboxed frames lacking the 'allow-top-navigation' flag.
    if (!isSandboxed(SandboxTopNavigation) && targetFrame == m_frame->tree()->top())
        return true;

    if (isSandboxed(SandboxNavigation)) {
        if (targetFrame->tree()->isDescendantOf(m_frame))
            return true;

        const char* reason = "The frame attempting navigation is sandboxed, and is therefore disallowed from navigating its ancestors.";
        if (isSandboxed(SandboxTopNavigation) && targetFrame == m_frame->tree()->top())
            reason = "The frame attempting navigation of the top-level window is sandboxed, but the 'allow-top-navigation' flag is not set.";

        printNavigationErrorMessage(targetFrame, url(), reason);
        return false;
    }

    // This is the normal case. A document can navigate its decendant frames,
    // or, more generally, a document can navigate a frame if the document is
    // in the same origin as any of that frame's ancestors (in the frame
    // hierarchy).
    //
    // See http://www.adambarth.com/papers/2008/barth-jackson-mitchell.pdf for
    // historical information about this security check.
    if (canAccessAncestor(securityOrigin(), targetFrame))
        return true;

    // Top-level frames are easier to navigate than other frames because they
    // display their URLs in the address bar (in most browsers). However, there
    // are still some restrictions on navigation to avoid nuisance attacks.
    // Specifically, a document can navigate a top-level frame if that frame
    // opened the document or if the document is the same-origin with any of
    // the top-level frame's opener's ancestors (in the frame hierarchy).
    //
    // In both of these cases, the document performing the navigation is in
    // some way related to the frame being navigate (e.g., by the "opener"
    // and/or "parent" relation). Requiring some sort of relation prevents a
    // document from navigating arbitrary, unrelated top-level frames.
    if (!targetFrame->tree()->parent()) {
        if (targetFrame == m_frame->loader()->opener())
            return true;

        if (canAccessAncestor(securityOrigin(), targetFrame->loader()->opener()))
            return true;
    }

    printNavigationErrorMessage(targetFrame, url(), "The frame attempting navigation is neither same-origin with the target, nor is it the target's parent or opener.");
    return false;
}

Frame* Document::findUnsafeParentScrollPropagationBoundary()
{
    Frame* currentFrame = m_frame;
    Frame* ancestorFrame = currentFrame->tree()->parent();

    while (ancestorFrame) {
        if (!ancestorFrame->document()->securityOrigin()->canAccess(securityOrigin()))
            return currentFrame;
        currentFrame = ancestorFrame;
        ancestorFrame = ancestorFrame->tree()->parent();
    }
    return 0;
}


void Document::seamlessParentUpdatedStylesheets()
{
    styleResolverChanged(RecalcStyleImmediately);
}

void Document::didRemoveAllPendingStylesheet()
{
    m_needsNotifyRemoveAllPendingStylesheet = false;

    styleResolverChanged(RecalcStyleImmediately, AnalyzedStyleUpdate);
    executeScriptsWaitingForResourcesIfNeeded();

    if (m_gotoAnchorNeededAfterStylesheetsLoad && view())
        view()->scrollToFragment(m_url);
}

void Document::executeScriptsWaitingForResourcesIfNeeded()
{
    if (!haveStylesheetsAndImportsLoaded())
        return;
    if (ScriptableDocumentParser* parser = scriptableDocumentParser())
        parser->executeScriptsWaitingForResources();
}


CSSStyleSheet* Document::elementSheet()
{
    if (!m_elemSheet)
        m_elemSheet = CSSStyleSheet::createInline(this, m_baseURL);
    return m_elemSheet.get();
}

void Document::processHttpEquiv(const String& equiv, const String& content)
{
    ASSERT(!equiv.isNull() && !content.isNull());

    if (equalIgnoringCase(equiv, "default-style"))
        processHttpEquivDefaultStyle(content);
    else if (equalIgnoringCase(equiv, "refresh"))
        processHttpEquivRefresh(content);
    else if (equalIgnoringCase(equiv, "set-cookie"))
        processHttpEquivSetCookie(content);
    else if (equalIgnoringCase(equiv, "content-language"))
        setContentLanguage(content);
    else if (equalIgnoringCase(equiv, "x-dns-prefetch-control"))
        parseDNSPrefetchControlHeader(content);
    else if (equalIgnoringCase(equiv, "x-frame-options"))
        processHttpEquivXFrameOptions(content);
    else if (equalIgnoringCase(equiv, "content-security-policy"))
        contentSecurityPolicy()->didReceiveHeader(content, ContentSecurityPolicy::Enforce);
    else if (equalIgnoringCase(equiv, "content-security-policy-report-only"))
        contentSecurityPolicy()->didReceiveHeader(content, ContentSecurityPolicy::Report);
    else if (equalIgnoringCase(equiv, "x-webkit-csp"))
        contentSecurityPolicy()->didReceiveHeader(content, ContentSecurityPolicy::PrefixedEnforce);
    else if (equalIgnoringCase(equiv, "x-webkit-csp-report-only"))
        contentSecurityPolicy()->didReceiveHeader(content, ContentSecurityPolicy::PrefixedReport);
}

void Document::processHttpEquivDefaultStyle(const String& content)
{
    // The preferred style set has been overridden as per section
    // 14.3.2 of the HTML4.0 specification. We need to update the
    // sheet used variable and then update our style selector.
    // For more info, see the test at:
    // http://www.hixie.ch/tests/evil/css/import/main/preferred.html
    // -dwh
    m_styleSheetCollection->setSelectedStylesheetSetName(content);
    m_styleSheetCollection->setPreferredStylesheetSetName(content);
    styleResolverChanged(DeferRecalcStyle);
}

void Document::processHttpEquivRefresh(const String& content)
{
    Frame* frame = this->frame();
    if (!frame)
        return;

    double delay;
    String refreshUrl;
    if (parseHTTPRefresh(content, true, delay, refreshUrl)) {
        if (refreshUrl.isEmpty())
            refreshUrl = m_url.string();
        else
            refreshUrl = completeURL(refreshUrl).string();

        if (protocolIsJavaScript(refreshUrl)) {
            String message = "Refused to refresh " + m_url.elidedString() + " to a javascript: URL";
            addConsoleMessage(SecurityMessageSource, ErrorMessageLevel, message);
            return;
        }

        if (isSandboxed(SandboxAutomaticFeatures)) {
            String message = "Refused to execute the redirect specified via '<meta http-equiv='refresh' content='...'>'. The document is sandboxed, and the 'allow-scripts' keyword is not set.";
            addConsoleMessage(SecurityMessageSource, ErrorMessageLevel, message);
            return;
        }

        frame->navigationScheduler()->scheduleRedirect(delay, refreshUrl);
    }
}

void Document::processHttpEquivSetCookie(const String& content)
{
    // FIXME: make setCookie work on XML documents too; e.g. in case of <html:meta .....>
    if (!isHTMLDocument())
        return;

    // Exception (for sandboxed documents) ignored.
    toHTMLDocument(this)->setCookie(content, IGNORE_EXCEPTION);
}

void Document::processHttpEquivXFrameOptions(const String& content)
{
    Frame* frame = this->frame();
    if (!frame)
        return;

    FrameLoader* frameLoader = frame->loader();
    unsigned long requestIdentifier = loader()->mainResourceIdentifier();
    if (frameLoader->shouldInterruptLoadForXFrameOptions(content, url(), requestIdentifier)) {
        String message = "Refused to display '" + url().elidedString() + "' in a frame because it set 'X-Frame-Options' to '" + content + "'.";
        frameLoader->stopAllLoaders();
        // Stopping the loader isn't enough, as we're already parsing the document; to honor the header's
        // intent, we must navigate away from the possibly partially-rendered document to a location that
        // doesn't inherit the parent's SecurityOrigin.
        frame->navigationScheduler()->scheduleLocationChange(securityOrigin(), SecurityOrigin::urlWithUniqueSecurityOrigin(), String());
        addConsoleMessage(SecurityMessageSource, ErrorMessageLevel, message, requestIdentifier);
    }
}

// Though isspace() considers \t and \v to be whitespace, Win IE doesn't.
static bool isSeparator(UChar c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '=' || c == ',' || c == '\0';
}

void Document::processArguments(const String& features, void* data, ArgumentsCallback callback)
{
    // Tread lightly in this code -- it was specifically designed to mimic Win IE's parsing behavior.
    int keyBegin, keyEnd;
    int valueBegin, valueEnd;

    int i = 0;
    int length = features.length();
    String buffer = features.lower();
    while (i < length) {
        // skip to first non-separator, but don't skip past the end of the string
        while (isSeparator(buffer[i])) {
            if (i >= length)
                break;
            i++;
        }
        keyBegin = i;

        // skip to first separator
        while (!isSeparator(buffer[i]))
            i++;
        keyEnd = i;

        // skip to first '=', but don't skip past a ',' or the end of the string
        while (buffer[i] != '=') {
            if (buffer[i] == ',' || i >= length)
                break;
            i++;
        }

        // skip to first non-separator, but don't skip past a ',' or the end of the string
        while (isSeparator(buffer[i])) {
            if (buffer[i] == ',' || i >= length)
                break;
            i++;
        }
        valueBegin = i;

        // skip to first separator
        while (!isSeparator(buffer[i]))
            i++;
        valueEnd = i;

        ASSERT_WITH_SECURITY_IMPLICATION(i <= length);

        String keyString = buffer.substring(keyBegin, keyEnd - keyBegin);
        String valueString = buffer.substring(valueBegin, valueEnd - valueBegin);
        callback(keyString, valueString, this, data);
    }
}

void Document::processViewport(const String& features, ViewportArguments::Type origin)
{
    ASSERT(!features.isNull());

    if (origin < m_viewportArguments.type)
        return;

    m_viewportArguments = ViewportArguments(origin);
    processArguments(features, (void*)&m_viewportArguments, &setViewportFeature);

    updateViewportArguments();
}

void Document::updateViewportArguments()
{
    if (page() && page()->mainFrame() == frame()) {
#ifndef NDEBUG
        m_didDispatchViewportPropertiesChanged = true;
#endif
        page()->chrome().dispatchViewportPropertiesDidChange(m_viewportArguments);
    }
}

void Document::processReferrerPolicy(const String& policy)
{
    ASSERT(!policy.isNull());

    m_referrerPolicy = ReferrerPolicyDefault;

    if (equalIgnoringCase(policy, "never"))
        m_referrerPolicy = ReferrerPolicyNever;
    else if (equalIgnoringCase(policy, "always"))
        m_referrerPolicy = ReferrerPolicyAlways;
    else if (equalIgnoringCase(policy, "origin"))
        m_referrerPolicy = ReferrerPolicyOrigin;
}

MouseEventWithHitTestResults Document::prepareMouseEvent(const HitTestRequest& request, const LayoutPoint& documentPoint, const PlatformMouseEvent& event)
{
    ASSERT(!renderer() || renderer()->isRenderView());

    // RenderView::hitTest causes a layout, and we don't want to hit that until the first
    // layout because until then, there is nothing shown on the screen - the user can't
    // have intentionally clicked on something belonging to this page. Furthermore,
    // mousemove events before the first layout should not lead to a premature layout()
    // happening, which could show a flash of white.
    // See also the similar code in EventHandler::hitTestResultAtPoint.
    if (!renderer() || !view() || !view()->didFirstLayout())
        return MouseEventWithHitTestResults(event, HitTestResult(LayoutPoint()));

    HitTestResult result(documentPoint);
    renderView()->hitTest(request, result);

    if (!request.readOnly())
        updateHoverActiveState(request, result.innerElement(), &event);

    return MouseEventWithHitTestResults(event, result);
}

// DOM Section 1.1.1
bool Document::childTypeAllowed(NodeType type) const
{
    switch (type) {
    case ATTRIBUTE_NODE:
    case CDATA_SECTION_NODE:
    case DOCUMENT_FRAGMENT_NODE:
    case DOCUMENT_NODE:
    case ENTITY_NODE:
    case NOTATION_NODE:
    case TEXT_NODE:
    case XPATH_NAMESPACE_NODE:
        return false;
    case COMMENT_NODE:
    case PROCESSING_INSTRUCTION_NODE:
        return true;
    case DOCUMENT_TYPE_NODE:
    case ELEMENT_NODE:
        // Documents may contain no more than one of each of these.
        // (One Element and one DocumentType.)
        for (Node* c = firstChild(); c; c = c->nextSibling())
            if (c->nodeType() == type)
                return false;
        return true;
    }
    return false;
}

bool Document::canReplaceChild(Node* newChild, Node* oldChild)
{
    if (!oldChild)
        // ContainerNode::replaceChild will raise a NotFoundError.
        return true;

    if (oldChild->nodeType() == newChild->nodeType())
        return true;

    int numDoctypes = 0;
    int numElements = 0;

    // First, check how many doctypes and elements we have, not counting
    // the child we're about to remove.
    for (Node* c = firstChild(); c; c = c->nextSibling()) {
        if (c == oldChild)
            continue;

        switch (c->nodeType()) {
        case DOCUMENT_TYPE_NODE:
            numDoctypes++;
            break;
        case ELEMENT_NODE:
            numElements++;
            break;
        default:
            break;
        }
    }

    // Then, see how many doctypes and elements might be added by the new child.
    if (newChild->nodeType() == DOCUMENT_FRAGMENT_NODE) {
        for (Node* c = newChild->firstChild(); c; c = c->nextSibling()) {
            switch (c->nodeType()) {
            case ATTRIBUTE_NODE:
            case CDATA_SECTION_NODE:
            case DOCUMENT_FRAGMENT_NODE:
            case DOCUMENT_NODE:
            case ENTITY_NODE:
            case NOTATION_NODE:
            case TEXT_NODE:
            case XPATH_NAMESPACE_NODE:
                return false;
            case COMMENT_NODE:
            case PROCESSING_INSTRUCTION_NODE:
                break;
            case DOCUMENT_TYPE_NODE:
                numDoctypes++;
                break;
            case ELEMENT_NODE:
                numElements++;
                break;
            }
        }
    } else {
        switch (newChild->nodeType()) {
        case ATTRIBUTE_NODE:
        case CDATA_SECTION_NODE:
        case DOCUMENT_FRAGMENT_NODE:
        case DOCUMENT_NODE:
        case ENTITY_NODE:
        case NOTATION_NODE:
        case TEXT_NODE:
        case XPATH_NAMESPACE_NODE:
            return false;
        case COMMENT_NODE:
        case PROCESSING_INSTRUCTION_NODE:
            return true;
        case DOCUMENT_TYPE_NODE:
            numDoctypes++;
            break;
        case ELEMENT_NODE:
            numElements++;
            break;
        }
    }

    if (numElements > 1 || numDoctypes > 1)
        return false;

    return true;
}

PassRefPtr<Node> Document::cloneNode(bool /*deep*/)
{
    // Spec says cloning Document nodes is "implementation dependent"
    // so we do not support it...
    return 0;
}

StyleSheetList* Document::styleSheets()
{
    if (!m_styleSheetList)
        m_styleSheetList = StyleSheetList::create(this);
    return m_styleSheetList.get();
}

String Document::preferredStylesheetSet() const
{
    return m_styleSheetCollection->preferredStylesheetSetName();
}

String Document::selectedStylesheetSet() const
{
    return m_styleSheetCollection->selectedStylesheetSetName();
}

void Document::setSelectedStylesheetSet(const String& aString)
{
    m_styleSheetCollection->setSelectedStylesheetSetName(aString);
    styleResolverChanged(DeferRecalcStyle);
}

void Document::evaluateMediaQueryList()
{
    if (m_mediaQueryMatcher)
        m_mediaQueryMatcher->styleResolverChanged();
}

void Document::styleResolverChanged(StyleResolverUpdateType updateType, StyleResolverUpdateMode updateMode)
{
    // Don't bother updating, since we haven't loaded all our style info yet
    // and haven't calculated the style selector for the first time.
    if (!attached() || (!m_didCalculateStyleResolver && !haveStylesheetsLoaded())) {
        m_styleResolver.clear();
        return;
    }
    m_didCalculateStyleResolver = true;

    bool needsRecalc = m_styleSheetCollection->updateActiveStyleSheets(updateMode);

    if (updateType >= DeferRecalcStyle) {
        scheduleForcedStyleRecalc();
        return;
    }

    if (didLayoutWithPendingStylesheets() && !m_styleSheetCollection->hasPendingSheets()) {
        m_pendingSheetLayout = IgnoreLayoutWithPendingSheets;
        if (renderer())
            renderView()->repaintViewAndCompositedLayers();
    }

    if (!needsRecalc)
        return;

    // This recalcStyle initiates a new recalc cycle. We need to bracket it to
    // make sure animations get the correct update time
    {
        AnimationUpdateBlock animationUpdateBlock(m_frame ? m_frame->animation() : 0);
        recalcStyle(Force);
    }

    if (renderer()) {
        renderer()->setNeedsLayoutAndPrefWidthsRecalc();
        if (view())
            view()->scheduleRelayout();
    }

    evaluateMediaQueryList();
}

void Document::notifySeamlessChildDocumentsOfStylesheetUpdate() const
{
    // If we're not in a frame yet any potential child documents won't have a StyleResolver to update.
    if (!frame())
        return;

    // Seamless child frames are expected to notify their seamless children recursively, so we only do direct children.
    for (Frame* child = frame()->tree()->firstChild(); child; child = child->tree()->nextSibling()) {
        Document* childDocument = child->document();
        if (childDocument->shouldDisplaySeamlesslyWithParent()) {
            ASSERT(childDocument->seamlessParentIFrame()->document() == this);
            childDocument->seamlessParentUpdatedStylesheets();
        }
    }
}

void Document::setHoverNode(PassRefPtr<Node> newHoverNode)
{
    m_hoverNode = newHoverNode;
}

void Document::setActiveElement(PassRefPtr<Element> newActiveElement)
{
    if (!newActiveElement) {
        m_activeElement.clear();
        return;
    }

    m_activeElement = newActiveElement;
}

void Document::removeFocusedElementOfSubtree(Node* node, bool amongChildrenOnly)
{
    if (!m_focusedElement)
        return;

    Element* focusedElement = node->treeScope()->adjustedFocusedElement();
    if (!focusedElement)
        return;

    bool nodeInSubtree = false;
    if (amongChildrenOnly)
        nodeInSubtree = focusedElement->isDescendantOf(node);
    else
        nodeInSubtree = (focusedElement == node) || focusedElement->isDescendantOf(node);

    if (nodeInSubtree)
        setFocusedElement(0);
}

void Document::hoveredNodeDetached(Node* node)
{
    if (!m_hoverNode)
        return;

    NodeRenderingTraversal::ParentDetails details;
    if (node != m_hoverNode && (!m_hoverNode->isTextNode() || node != NodeRenderingTraversal::parent(m_hoverNode.get(), &details)))
        return;

    for (m_hoverNode = NodeRenderingTraversal::parent(node, &details); m_hoverNode && !m_hoverNode->renderer(); m_hoverNode = NodeRenderingTraversal::parent(m_hoverNode.get(), &details)) { }

    // If the mouse cursor is not visible, do not clear existing
    // hover effects on the ancestors of |node| and do not invoke
    // new hover effects on any other element.
    if (!page()->isCursorVisible())
        return;

    if (frame())
        frame()->eventHandler()->scheduleHoverStateUpdate();
}

void Document::activeChainNodeDetached(Node* node)
{
    if (!m_activeElement)
        return;

    NodeRenderingTraversal::ParentDetails details;
    if (node != m_activeElement && (!m_activeElement->isTextNode() || node != NodeRenderingTraversal::parent(m_activeElement.get(), &details)))
        return;

    Node* activeNode = NodeRenderingTraversal::parent(node, &details);
    for (; activeNode && activeNode->isElementNode() && !activeNode->renderer(); activeNode = NodeRenderingTraversal::parent(activeNode, &details)) { }

    m_activeElement = activeNode && activeNode->isElementNode() ? toElement(activeNode) : 0;
}

const Vector<AnnotatedRegionValue>& Document::annotatedRegions() const
{
    return m_annotatedRegions;
}

void Document::setAnnotatedRegions(const Vector<AnnotatedRegionValue>& regions)
{
    m_annotatedRegions = regions;
    setAnnotatedRegionsDirty(false);
}

bool Document::setFocusedElement(PassRefPtr<Element> prpNewFocusedElement, FocusDirection direction)
{
    RefPtr<Element> newFocusedElement = prpNewFocusedElement;

    // Make sure newFocusedNode is actually in this document
    if (newFocusedElement && (newFocusedElement->document() != this))
        return true;

    if (m_focusedElement == newFocusedElement)
        return true;

    bool focusChangeBlocked = false;
    RefPtr<Element> oldFocusedElement = m_focusedElement;
    m_focusedElement = 0;

    // Remove focus from the existing focus node (if any)
    if (oldFocusedElement) {
        ASSERT(!oldFocusedElement->inDetach());

        if (oldFocusedElement->active())
            oldFocusedElement->setActive(false);

        oldFocusedElement->setFocus(false);

        // Dispatch a change event for text fields or textareas that have been edited
        if (oldFocusedElement->wasChangedSinceLastFormControlChangeEvent())
            oldFocusedElement->dispatchFormControlChangeEvent();

        // Dispatch the blur event and let the node do any other blur related activities (important for text fields)
        oldFocusedElement->dispatchBlurEvent(newFocusedElement);

        if (m_focusedElement) {
            // handler shifted focus
            focusChangeBlocked = true;
            newFocusedElement = 0;
        }

        oldFocusedElement->dispatchFocusOutEvent(eventNames().focusoutEvent, newFocusedElement); // DOM level 3 name for the bubbling blur event.
        // FIXME: We should remove firing DOMFocusOutEvent event when we are sure no content depends
        // on it, probably when <rdar://problem/8503958> is resolved.
        oldFocusedElement->dispatchFocusOutEvent(eventNames().DOMFocusOutEvent, newFocusedElement); // DOM level 2 name for compatibility.

        if (m_focusedElement) {
            // handler shifted focus
            focusChangeBlocked = true;
            newFocusedElement = 0;
        }

        if (oldFocusedElement->isRootEditableElement())
            frame()->editor()->didEndEditing();

        if (view()) {
            Widget* oldWidget = widgetForElement(oldFocusedElement.get());
            if (oldWidget)
                oldWidget->setFocus(false);
            else
                view()->setFocus(false);
        }
    }

    if (newFocusedElement && newFocusedElement->isFocusable()) {
        if (newFocusedElement->isRootEditableElement() && !acceptsEditingFocus(newFocusedElement.get())) {
            // delegate blocks focus change
            focusChangeBlocked = true;
            goto SetFocusedElementDone;
        }
        // Set focus on the new node
        m_focusedElement = newFocusedElement;

        // Dispatch the focus event and let the node do any other focus related activities (important for text fields)
        m_focusedElement->dispatchFocusEvent(oldFocusedElement, direction);

        if (m_focusedElement != newFocusedElement) {
            // handler shifted focus
            focusChangeBlocked = true;
            goto SetFocusedElementDone;
        }

        m_focusedElement->dispatchFocusInEvent(eventNames().focusinEvent, oldFocusedElement); // DOM level 3 bubbling focus event.

        if (m_focusedElement != newFocusedElement) {
            // handler shifted focus
            focusChangeBlocked = true;
            goto SetFocusedElementDone;
        }

        // FIXME: We should remove firing DOMFocusInEvent event when we are sure no content depends
        // on it, probably when <rdar://problem/8503958> is m.
        m_focusedElement->dispatchFocusInEvent(eventNames().DOMFocusInEvent, oldFocusedElement); // DOM level 2 for compatibility.

        if (m_focusedElement != newFocusedElement) {
            // handler shifted focus
            focusChangeBlocked = true;
            goto SetFocusedElementDone;
        }
        m_focusedElement->setFocus(true);

        if (m_focusedElement->isRootEditableElement())
            frame()->editor()->didBeginEditing();

        // eww, I suck. set the qt focus correctly
        // ### find a better place in the code for this
        if (view()) {
            Widget* focusWidget = widgetForElement(m_focusedElement.get());
            if (focusWidget) {
                // Make sure a widget has the right size before giving it focus.
                // Otherwise, we are testing edge cases of the Widget code.
                // Specifically, in WebCore this does not work well for text fields.
                updateLayout();
                // Re-get the widget in case updating the layout changed things.
                focusWidget = widgetForElement(m_focusedElement.get());
            }
            if (focusWidget)
                focusWidget->setFocus(true);
            else
                view()->setFocus(true);
        }
    }

    if (!focusChangeBlocked && m_focusedElement) {
        // Create the AXObject cache in a focus change because Chromium relies on it.
        if (AXObjectCache* cache = axObjectCache())
            cache->handleFocusedUIElementChanged(oldFocusedElement.get(), newFocusedElement.get());
    }

    if (!focusChangeBlocked)
        page()->chrome().focusedNodeChanged(m_focusedElement.get());

SetFocusedElementDone:
    updateStyleIfNeeded();
    if (Frame* frame = this->frame())
        frame->selection()->didChangeFocus();
    return !focusChangeBlocked;
}

void Document::setCSSTarget(Element* n)
{
    if (m_cssTarget)
        m_cssTarget->didAffectSelector(AffectedSelectorTarget);
    m_cssTarget = n;
    if (n)
        n->didAffectSelector(AffectedSelectorTarget);
}

void Document::registerNodeList(LiveNodeListBase* list)
{
    if (list->hasIdNameCache())
        m_nodeListCounts[InvalidateOnIdNameAttrChange]++;
    m_nodeListCounts[list->invalidationType()]++;
    if (list->isRootedAtDocument())
        m_listsInvalidatedAtDocument.add(list);
}

void Document::unregisterNodeList(LiveNodeListBase* list)
{
    if (list->hasIdNameCache())
        m_nodeListCounts[InvalidateOnIdNameAttrChange]--;
    m_nodeListCounts[list->invalidationType()]--;
    if (list->isRootedAtDocument()) {
        ASSERT(m_listsInvalidatedAtDocument.contains(list));
        m_listsInvalidatedAtDocument.remove(list);
    }
}

void Document::attachNodeIterator(NodeIterator* ni)
{
    m_nodeIterators.add(ni);
}

void Document::detachNodeIterator(NodeIterator* ni)
{
    // The node iterator can be detached without having been attached if its root node didn't have a document
    // when the iterator was created, but has it now.
    m_nodeIterators.remove(ni);
}

void Document::moveNodeIteratorsToNewDocument(Node* node, Document* newDocument)
{
    HashSet<NodeIterator*> nodeIteratorsList = m_nodeIterators;
    HashSet<NodeIterator*>::const_iterator nodeIteratorsEnd = nodeIteratorsList.end();
    for (HashSet<NodeIterator*>::const_iterator it = nodeIteratorsList.begin(); it != nodeIteratorsEnd; ++it) {
        if ((*it)->root() == node) {
            detachNodeIterator(*it);
            newDocument->attachNodeIterator(*it);
        }
    }
}

void Document::updateRangesAfterChildrenChanged(ContainerNode* container)
{
    if (!m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->nodeChildrenChanged(container);
    }
}

void Document::nodeChildrenWillBeRemoved(ContainerNode* container)
{
    if (!m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->nodeChildrenWillBeRemoved(container);
    }

    HashSet<NodeIterator*>::const_iterator nodeIteratorsEnd = m_nodeIterators.end();
    for (HashSet<NodeIterator*>::const_iterator it = m_nodeIterators.begin(); it != nodeIteratorsEnd; ++it) {
        for (Node* n = container->firstChild(); n; n = n->nextSibling())
            (*it)->nodeWillBeRemoved(n);
    }

    if (Frame* frame = this->frame()) {
        for (Node* n = container->firstChild(); n; n = n->nextSibling()) {
            frame->eventHandler()->nodeWillBeRemoved(n);
            frame->selection()->nodeWillBeRemoved(n);
            frame->page()->dragCaretController()->nodeWillBeRemoved(n);
        }
    }
}

void Document::nodeWillBeRemoved(Node* n)
{
    HashSet<NodeIterator*>::const_iterator nodeIteratorsEnd = m_nodeIterators.end();
    for (HashSet<NodeIterator*>::const_iterator it = m_nodeIterators.begin(); it != nodeIteratorsEnd; ++it)
        (*it)->nodeWillBeRemoved(n);

    if (!m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator rangesEnd = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != rangesEnd; ++it)
            (*it)->nodeWillBeRemoved(n);
    }

    if (Frame* frame = this->frame()) {
        frame->eventHandler()->nodeWillBeRemoved(n);
        frame->selection()->nodeWillBeRemoved(n);
        frame->page()->dragCaretController()->nodeWillBeRemoved(n);
    }
}

void Document::textInserted(Node* text, unsigned offset, unsigned length)
{
    if (!m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->textInserted(text, offset, length);
    }

    // Update the markers for spelling and grammar checking.
    m_markers->shiftMarkers(text, offset, length);
}

void Document::textRemoved(Node* text, unsigned offset, unsigned length)
{
    if (!m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->textRemoved(text, offset, length);
    }

    // Update the markers for spelling and grammar checking.
    m_markers->removeMarkers(text, offset, length);
    m_markers->shiftMarkers(text, offset + length, 0 - length);
}

void Document::textNodesMerged(Text* oldNode, unsigned offset)
{
    if (!m_ranges.isEmpty()) {
        NodeWithIndex oldNodeWithIndex(oldNode);
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->textNodesMerged(oldNodeWithIndex, offset);
    }

    // FIXME: This should update markers for spelling and grammar checking.
}

void Document::textNodeSplit(Text* oldNode)
{
    if (!m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->textNodeSplit(oldNode);
    }

    // FIXME: This should update markers for spelling and grammar checking.
}

void Document::setWindowAttributeEventListener(const AtomicString& eventType, PassRefPtr<EventListener> listener, DOMWrapperWorld* isolatedWorld)
{
    DOMWindow* domWindow = this->domWindow();
    if (!domWindow)
        return;
    domWindow->setAttributeEventListener(eventType, listener, isolatedWorld);
}

EventListener* Document::getWindowAttributeEventListener(const AtomicString& eventType, DOMWrapperWorld* isolatedWorld)
{
    DOMWindow* domWindow = this->domWindow();
    if (!domWindow)
        return 0;
    return domWindow->getAttributeEventListener(eventType, isolatedWorld);
}

void Document::dispatchWindowEvent(PassRefPtr<Event> event,  PassRefPtr<EventTarget> target)
{
    ASSERT(!NoEventDispatchAssertion::isEventDispatchForbidden());
    DOMWindow* domWindow = this->domWindow();
    if (!domWindow)
        return;
    domWindow->dispatchEvent(event, target);
}

void Document::enqueueWindowEvent(PassRefPtr<Event> event)
{
    event->setTarget(domWindow());
    m_eventQueue->enqueueEvent(event);
}

void Document::enqueueDocumentEvent(PassRefPtr<Event> event)
{
    event->setTarget(this);
    m_eventQueue->enqueueEvent(event);
}

PassRefPtr<Event> Document::createEvent(const String& eventType, ExceptionCode& ec)
{
    RefPtr<Event> event = EventFactory::create(eventType);
    if (event)
        return event.release();

    ec = NotSupportedError;
    return 0;
}

void Document::dispatchWindowLoadEvent()
{
    ASSERT(!NoEventDispatchAssertion::isEventDispatchForbidden());
    DOMWindow* domWindow = this->domWindow();
    if (!domWindow)
        return;
    domWindow->dispatchLoadEvent();
    m_loadEventFinished = true;
}

void Document::addMutationEventListenerTypeIfEnabled(ListenerType listenerType)
{
    if (ContextFeatures::mutationEventsEnabled(this))
        addListenerType(listenerType);
}

void Document::addListenerTypeIfNeeded(const AtomicString& eventType)
{
    if (eventType == eventNames().DOMSubtreeModifiedEvent)
        addMutationEventListenerTypeIfEnabled(DOMSUBTREEMODIFIED_LISTENER);
    else if (eventType == eventNames().DOMNodeInsertedEvent)
        addMutationEventListenerTypeIfEnabled(DOMNODEINSERTED_LISTENER);
    else if (eventType == eventNames().DOMNodeRemovedEvent)
        addMutationEventListenerTypeIfEnabled(DOMNODEREMOVED_LISTENER);
    else if (eventType == eventNames().DOMNodeRemovedFromDocumentEvent)
        addMutationEventListenerTypeIfEnabled(DOMNODEREMOVEDFROMDOCUMENT_LISTENER);
    else if (eventType == eventNames().DOMNodeInsertedIntoDocumentEvent)
        addMutationEventListenerTypeIfEnabled(DOMNODEINSERTEDINTODOCUMENT_LISTENER);
    else if (eventType == eventNames().DOMCharacterDataModifiedEvent)
        addMutationEventListenerTypeIfEnabled(DOMCHARACTERDATAMODIFIED_LISTENER);
    else if (eventType == eventNames().overflowchangedEvent)
        addListenerType(OVERFLOWCHANGED_LISTENER);
    else if (eventType == eventNames().webkitAnimationStartEvent)
        addListenerType(ANIMATIONSTART_LISTENER);
    else if (eventType == eventNames().webkitAnimationEndEvent)
        addListenerType(ANIMATIONEND_LISTENER);
    else if (eventType == eventNames().webkitAnimationIterationEvent)
        addListenerType(ANIMATIONITERATION_LISTENER);
    else if (eventType == eventNames().webkitTransitionEndEvent || eventType == eventNames().transitionendEvent)
        addListenerType(TRANSITIONEND_LISTENER);
    else if (eventType == eventNames().beforeloadEvent)
        addListenerType(BEFORELOAD_LISTENER);
    else if (eventType == eventNames().scrollEvent)
        addListenerType(SCROLL_LISTENER);
}

CSSStyleDeclaration* Document::getOverrideStyle(Element*, const String&)
{
    return 0;
}

HTMLFrameOwnerElement* Document::ownerElement() const
{
    if (!frame())
        return 0;
    return frame()->ownerElement();
}

String Document::cookie(ExceptionCode& ec) const
{
    if (page() && !page()->settings()->cookieEnabled())
        return String();

    // FIXME: The HTML5 DOM spec states that this attribute can raise an
    // InvalidStateError exception on getting if the Document has no
    // browsing context.

    if (!securityOrigin()->canAccessCookies()) {
        ec = SecurityError;
        return String();
    }

    KURL cookieURL = this->cookieURL();
    if (cookieURL.isEmpty())
        return String();

    return cookies(this, cookieURL);
}

void Document::setCookie(const String& value, ExceptionCode& ec)
{
    if (page() && !page()->settings()->cookieEnabled())
        return;

    // FIXME: The HTML5 DOM spec states that this attribute can raise an
    // InvalidStateError exception on setting if the Document has no
    // browsing context.

    if (!securityOrigin()->canAccessCookies()) {
        ec = SecurityError;
        return;
    }

    KURL cookieURL = this->cookieURL();
    if (cookieURL.isEmpty())
        return;

    setCookies(this, cookieURL, value);
}

String Document::referrer() const
{
    if (frame())
        return frame()->loader()->referrer();
    return String();
}

String Document::domain() const
{
    return securityOrigin()->domain();
}

void Document::setDomain(const String& newDomain, ExceptionCode& ec)
{
    if (SchemeRegistry::isDomainRelaxationForbiddenForURLScheme(securityOrigin()->protocol())) {
        ec = SecurityError;
        return;
    }

    // Both NS and IE specify that changing the domain is only allowed when
    // the new domain is a suffix of the old domain.

    // FIXME: We should add logging indicating why a domain was not allowed.

    // If the new domain is the same as the old domain, still call
    // securityOrigin()->setDomainForDOM. This will change the
    // security check behavior. For example, if a page loaded on port 8000
    // assigns its current domain using document.domain, the page will
    // allow other pages loaded on different ports in the same domain that
    // have also assigned to access this page.
    if (equalIgnoringCase(domain(), newDomain)) {
        securityOrigin()->setDomainFromDOM(newDomain);
        if (m_frame)
            m_frame->script()->updateSecurityOrigin();
        return;
    }

    int oldLength = domain().length();
    int newLength = newDomain.length();
    // e.g. newDomain = webkit.org (10) and domain() = www.webkit.org (14)
    if (newLength >= oldLength) {
        ec = SecurityError;
        return;
    }

    String test = domain();
    // Check that it's a subdomain, not e.g. "ebkit.org"
    if (test[oldLength - newLength - 1] != '.') {
        ec = SecurityError;
        return;
    }

    // Now test is "webkit.org" from domain()
    // and we check that it's the same thing as newDomain
    test.remove(0, oldLength - newLength);
    if (test != newDomain) {
        ec = SecurityError;
        return;
    }

    securityOrigin()->setDomainFromDOM(newDomain);
    if (m_frame)
        m_frame->script()->updateSecurityOrigin();
}

// http://www.whatwg.org/specs/web-apps/current-work/#dom-document-lastmodified
String Document::lastModified() const
{
    DateComponents date;
    bool foundDate = false;
    if (m_frame) {
        String httpLastModified;
        if (DocumentLoader* documentLoader = loader())
            httpLastModified = documentLoader->response().httpHeaderField("Last-Modified");
        if (!httpLastModified.isEmpty()) {
            date.setMillisecondsSinceEpochForDateTime(parseDate(httpLastModified));
            foundDate = true;
        }
    }
    // FIXME: If this document came from the file system, the HTML5
    // specificiation tells us to read the last modification date from the file
    // system.
    if (!foundDate)
        date.setMillisecondsSinceEpochForDateTime(currentTimeMS());
    return String::format("%02d/%02d/%04d %02d:%02d:%02d", date.month() + 1, date.monthDay(), date.fullYear(), date.hour(), date.minute(), date.second());
}

const KURL& Document::firstPartyForCookies() const
{
    return topDocument()->url();
}

static bool isValidNameNonASCII(const LChar* characters, unsigned length)
{
    if (!isValidNameStart(characters[0]))
        return false;

    for (unsigned i = 1; i < length; ++i) {
        if (!isValidNamePart(characters[i]))
            return false;
    }

    return true;
}

static bool isValidNameNonASCII(const UChar* characters, unsigned length)
{
    unsigned i = 0;

    UChar32 c;
    U16_NEXT(characters, i, length, c)
    if (!isValidNameStart(c))
        return false;

    while (i < length) {
        U16_NEXT(characters, i, length, c)
        if (!isValidNamePart(c))
            return false;
    }

    return true;
}

template<typename CharType>
static inline bool isValidNameASCII(const CharType* characters, unsigned length)
{
    CharType c = characters[0];
    if (!(isASCIIAlpha(c) || c == ':' || c == '_'))
        return false;

    for (unsigned i = 1; i < length; ++i) {
        c = characters[i];
        if (!(isASCIIAlphanumeric(c) || c == ':' || c == '_' || c == '-' || c == '.'))
            return false;
    }

    return true;
}

bool Document::isValidName(const String& name)
{
    unsigned length = name.length();
    if (!length)
        return false;

    if (name.is8Bit()) {
        const LChar* characters = name.characters8();

        if (isValidNameASCII(characters, length))
            return true;

        return isValidNameNonASCII(characters, length);
    }

    const UChar* characters = name.characters16();

    if (isValidNameASCII(characters, length))
        return true;

    return isValidNameNonASCII(characters, length);
}

template<typename CharType>
static bool parseQualifiedNameInternal(const String& qualifiedName, const CharType* characters, unsigned length, String& prefix, String& localName, ExceptionCode& ec)
{
    bool nameStart = true;
    bool sawColon = false;
    int colonPos = 0;

    for (unsigned i = 0; i < length;) {
        UChar32 c;
        U16_NEXT(characters, i, length, c)
        if (c == ':') {
            if (sawColon) {
                ec = NamespaceError;
                return false; // multiple colons: not allowed
            }
            nameStart = true;
            sawColon = true;
            colonPos = i - 1;
        } else if (nameStart) {
            if (!isValidNameStart(c)) {
                ec = InvalidCharacterError;
                return false;
            }
            nameStart = false;
        } else {
            if (!isValidNamePart(c)) {
                ec = InvalidCharacterError;
                return false;
            }
        }
    }

    if (!sawColon) {
        prefix = String();
        localName = qualifiedName;
    } else {
        prefix = qualifiedName.substring(0, colonPos);
        if (prefix.isEmpty()) {
            ec = NamespaceError;
            return false;
        }
        localName = qualifiedName.substring(colonPos + 1);
    }

    if (localName.isEmpty()) {
        ec = NamespaceError;
        return false;
    }

    return true;
}

bool Document::parseQualifiedName(const String& qualifiedName, String& prefix, String& localName, ExceptionCode& ec)
{
    unsigned length = qualifiedName.length();

    if (!length) {
        ec = InvalidCharacterError;
        return false;
    }

    if (qualifiedName.is8Bit())
        return parseQualifiedNameInternal(qualifiedName, qualifiedName.characters8(), length, prefix, localName, ec);
    return parseQualifiedNameInternal(qualifiedName, qualifiedName.characters16(), length, prefix, localName, ec);
}

void Document::setDecoder(PassRefPtr<TextResourceDecoder> decoder)
{
    m_decoder = decoder;
}

KURL Document::completeURL(const String& url, const KURL& baseURLOverride) const
{
    // Always return a null URL when passed a null string.
    // FIXME: Should we change the KURL constructor to have this behavior?
    // See also [CSS]StyleSheet::completeURL(const String&)
    if (url.isNull())
        return KURL();
    const KURL& baseURL = ((baseURLOverride.isEmpty() || baseURLOverride == blankURL()) && parentDocument()) ? parentDocument()->baseURL() : baseURLOverride;
    if (!m_decoder)
        return KURL(baseURL, url);
    return KURL(baseURL, url, m_decoder->encoding());
}

KURL Document::completeURL(const String& url) const
{
    return completeURL(url, m_baseURL);
}

void Document::documentWillBecomeInactive()
{
    if (renderer())
        renderView()->setIsInWindow(false);
}

// Support for Javascript execCommand, and related methods

static Editor::Command command(Document* document, const String& commandName, bool userInterface = false)
{
    Frame* frame = document->frame();
    if (!frame || frame->document() != document)
        return Editor::Command();

    document->updateStyleIfNeeded();

    return frame->editor()->command(commandName,
        userInterface ? CommandFromDOMWithUserInterface : CommandFromDOM);
}

bool Document::execCommand(const String& commandName, bool userInterface, const String& value)
{
    return command(this, commandName, userInterface).execute(value);
}

bool Document::queryCommandEnabled(const String& commandName)
{
    return command(this, commandName).isEnabled();
}

bool Document::queryCommandIndeterm(const String& commandName)
{
    return command(this, commandName).state() == MixedTriState;
}

bool Document::queryCommandState(const String& commandName)
{
    return command(this, commandName).state() == TrueTriState;
}

bool Document::queryCommandSupported(const String& commandName)
{
    return command(this, commandName).isSupported();
}

String Document::queryCommandValue(const String& commandName)
{
    return command(this, commandName).value();
}

KURL Document::openSearchDescriptionURL()
{
    static const char* const openSearchMIMEType = "application/opensearchdescription+xml";
    static const char* const openSearchRelation = "search";

    // FIXME: Why do only top-level frames have openSearchDescriptionURLs?
    if (!frame() || frame()->tree()->parent())
        return KURL();

    // FIXME: Why do we need to wait for FrameStateComplete?
    if (frame()->loader()->state() != FrameStateComplete)
        return KURL();

    if (!head())
        return KURL();

    RefPtr<HTMLCollection> children = head()->children();
    for (unsigned i = 0; Node* child = children->item(i); i++) {
        if (!child->hasTagName(linkTag))
            continue;
        HTMLLinkElement* linkElement = toHTMLLinkElement(child);
        if (!equalIgnoringCase(linkElement->type(), openSearchMIMEType) || !equalIgnoringCase(linkElement->rel(), openSearchRelation))
            continue;
        if (linkElement->href().isEmpty())
            continue;
        return linkElement->href();
    }

    return KURL();
}

void Document::pushCurrentScript(PassRefPtr<HTMLScriptElement> newCurrentScript)
{
    ASSERT(newCurrentScript);
    m_currentScriptStack.append(newCurrentScript);
}

void Document::popCurrentScript()
{
    ASSERT(!m_currentScriptStack.isEmpty());
    m_currentScriptStack.removeLast();
}

void Document::applyXSLTransform(ProcessingInstruction* pi)
{
    UseCounter::count(this, UseCounter::XSLProcessingInstruction);
    RefPtr<XSLTProcessor> processor = XSLTProcessor::create();
    processor->setXSLStyleSheet(static_cast<XSLStyleSheet*>(pi->sheet()));
    String resultMIMEType;
    String newSource;
    String resultEncoding;
    if (!processor->transformToString(this, resultMIMEType, newSource, resultEncoding))
        return;
    // FIXME: If the transform failed we should probably report an error (like Mozilla does).
    Frame* ownerFrame = frame();
    processor->createDocumentFromSource(newSource, resultEncoding, resultMIMEType, this, ownerFrame);
    InspectorInstrumentation::frameDocumentUpdated(ownerFrame);
}

void Document::setTransformSource(PassOwnPtr<TransformSource> source)
{
    m_transformSource = source;
}

void Document::setDesignMode(InheritedBool value)
{
    m_designMode = value;
    for (Frame* frame = m_frame; frame && frame->document(); frame = frame->tree()->traverseNext(m_frame))
        frame->document()->scheduleForcedStyleRecalc();
}

Document::InheritedBool Document::getDesignMode() const
{
    return m_designMode;
}

bool Document::inDesignMode() const
{
    for (const Document* d = this; d; d = d->parentDocument()) {
        if (d->m_designMode != inherit)
            return d->m_designMode;
    }
    return false;
}

Document* Document::parentDocument() const
{
    if (!m_frame)
        return 0;
    Frame* parent = m_frame->tree()->parent();
    if (!parent)
        return 0;
    return parent->document();
}

Document* Document::topDocument() const
{
    Document* doc = const_cast<Document *>(this);
    Element* element;
    while ((element = doc->ownerElement()))
        doc = element->document();

    return doc;
}

PassRefPtr<Attr> Document::createAttribute(const String& name, ExceptionCode& ec)
{
    return createAttributeNS(String(), name, ec, true);
}

PassRefPtr<Attr> Document::createAttributeNS(const String& namespaceURI, const String& qualifiedName, ExceptionCode& ec, bool shouldIgnoreNamespaceChecks)
{
    String prefix, localName;
    if (!parseQualifiedName(qualifiedName, prefix, localName, ec))
        return 0;

    QualifiedName qName(prefix, localName, namespaceURI);

    if (!shouldIgnoreNamespaceChecks && !hasValidNamespaceForAttributes(qName)) {
        ec = NamespaceError;
        return 0;
    }

    return Attr::create(this, qName, emptyString());
}

const SVGDocumentExtensions* Document::svgExtensions()
{
    return m_svgExtensions.get();
}

SVGDocumentExtensions* Document::accessSVGExtensions()
{
    if (!m_svgExtensions)
        m_svgExtensions = adoptPtr(new SVGDocumentExtensions(this));
    return m_svgExtensions.get();
}

bool Document::hasSVGRootNode() const
{
    return documentElement() && documentElement()->hasTagName(SVGNames::svgTag);
}

PassRefPtr<HTMLCollection> Document::ensureCachedCollection(CollectionType type)
{
    return ensureRareData()->ensureNodeLists()->addCacheWithAtomicName<HTMLCollection>(this, type);
}

PassRefPtr<HTMLCollection> Document::images()
{
    return ensureCachedCollection(DocImages);
}

PassRefPtr<HTMLCollection> Document::applets()
{
    return ensureCachedCollection(DocApplets);
}

PassRefPtr<HTMLCollection> Document::embeds()
{
    return ensureCachedCollection(DocEmbeds);
}

PassRefPtr<HTMLCollection> Document::plugins()
{
    // This is an alias for embeds() required for the JS DOM bindings.
    return ensureCachedCollection(DocEmbeds);
}

PassRefPtr<HTMLCollection> Document::scripts()
{
    return ensureCachedCollection(DocScripts);
}

PassRefPtr<HTMLCollection> Document::links()
{
    return ensureCachedCollection(DocLinks);
}

PassRefPtr<HTMLCollection> Document::forms()
{
    return ensureCachedCollection(DocForms);
}

PassRefPtr<HTMLCollection> Document::anchors()
{
    return ensureCachedCollection(DocAnchors);
}

PassRefPtr<HTMLCollection> Document::allForBinding()
{
    UseCounter::count(this, UseCounter::DocumentAll);
    return all();
}

PassRefPtr<HTMLCollection> Document::all()
{
    return ensureRareData()->ensureNodeLists()->addCacheWithAtomicName<HTMLAllCollection>(this, DocAll);
}

PassRefPtr<HTMLCollection> Document::windowNamedItems(const AtomicString& name)
{
    return ensureRareData()->ensureNodeLists()->addCacheWithAtomicName<HTMLNameCollection>(this, WindowNamedItems, name);
}

PassRefPtr<HTMLCollection> Document::documentNamedItems(const AtomicString& name)
{
    return ensureRareData()->ensureNodeLists()->addCacheWithAtomicName<HTMLNameCollection>(this, DocumentNamedItems, name);
}

void Document::finishedParsing()
{
    ASSERT(!scriptableDocumentParser() || !m_parser->isParsing());
    ASSERT(!scriptableDocumentParser() || m_readyState != Loading);
    setParsing(false);
    if (!m_documentTiming.domContentLoadedEventStart)
        m_documentTiming.domContentLoadedEventStart = monotonicallyIncreasingTime();
    dispatchEvent(Event::create(eventNames().DOMContentLoadedEvent, true, false));
    if (!m_documentTiming.domContentLoadedEventEnd)
        m_documentTiming.domContentLoadedEventEnd = monotonicallyIncreasingTime();

    if (RefPtr<Frame> f = frame()) {
        // FrameLoader::finishedParsing() might end up calling Document::implicitClose() if all
        // resource loads are complete. HTMLObjectElements can start loading their resources from
        // post attach callbacks triggered by recalcStyle().  This means if we parse out an <object>
        // tag and then reach the end of the document without updating styles, we might not have yet
        // started the resource load and might fire the window load event too early.  To avoid this
        // we force the styles to be up to date before calling FrameLoader::finishedParsing().
        // See https://bugs.webkit.org/show_bug.cgi?id=36864 starting around comment 35.
        updateStyleIfNeeded();

        f->loader()->finishedParsing();

        InspectorInstrumentation::domContentLoadedEventFired(f.get());
    }

    // Schedule dropping of the DocumentSharedObjectPool. We keep it alive for a while after parsing finishes
    // so that dynamically inserted content can also benefit from sharing optimizations.
    // Note that we don't refresh the timer on pool access since that could lead to huge caches being kept
    // alive indefinitely by something innocuous like JS setting .innerHTML repeatedly on a timer.
    static const int timeToKeepSharedObjectPoolAliveAfterParsingFinishedInSeconds = 10;
    m_sharedObjectPoolClearTimer.startOneShot(timeToKeepSharedObjectPoolAliveAfterParsingFinishedInSeconds);

    // Parser should have picked up all preloads by now
    m_cachedResourceLoader->clearPreloads();
}

void Document::sharedObjectPoolClearTimerFired(Timer<Document>*)
{
    m_sharedObjectPool.clear();
}

void Document::didAccessStyleResolver()
{
    static const int timeBeforeThrowingAwayStyleResolverAfterLastUseInSeconds = 60;
    static const int holdOffTimeBeforeReschedulingTimerInSeconds = 5;

    double currentTime = WTF::currentTime();

    if (currentTime > m_lastStyleResolverAccessTime + holdOffTimeBeforeReschedulingTimerInSeconds) {
        m_styleResolverThrowawayTimer.startOneShot(timeBeforeThrowingAwayStyleResolverAfterLastUseInSeconds);
        m_lastStyleResolverAccessTime = currentTime;
    }
}

void Document::styleResolverThrowawayTimerFired(Timer<Document>*)
{
    ASSERT(!m_inStyleRecalc);
    clearStyleResolver();
}

const Vector<IconURL>& Document::shortcutIconURLs()
{
    // Include any icons where type = link, rel = "shortcut icon".
    return iconURLs(Favicon);
}

const Vector<IconURL>& Document::iconURLs(int iconTypesMask)
{
    m_iconURLs.clear();

    if (!head() || !(head()->children()))
        return m_iconURLs;

    RefPtr<HTMLCollection> children = head()->children();
    unsigned int length = children->length();
    for (unsigned int i = 0; i < length; ++i) {
        Node* child = children->item(i);
        if (!child->hasTagName(linkTag))
            continue;
        HTMLLinkElement* linkElement = toHTMLLinkElement(child);
        if (!(linkElement->iconType() & iconTypesMask))
            continue;
        if (linkElement->href().isEmpty())
            continue;

        // Put it at the front to ensure that icons seen later take precedence as required by the spec.
        IconURL newURL(linkElement->href(), linkElement->iconSizes(), linkElement->type(), linkElement->iconType());
        m_iconURLs.prepend(newURL);
    }

    return m_iconURLs;
}

void Document::setUseSecureKeyboardEntryWhenActive(bool usesSecureKeyboard)
{
    if (m_useSecureKeyboardEntryWhenActive == usesSecureKeyboard)
        return;

    m_useSecureKeyboardEntryWhenActive = usesSecureKeyboard;
    m_frame->selection()->updateSecureKeyboardEntryIfActive();
}

bool Document::useSecureKeyboardEntryWhenActive() const
{
    return m_useSecureKeyboardEntryWhenActive;
}

static bool isEligibleForSeamless(Document* parent, Document* child)
{
    // It should not matter what we return for the top-most document.
    if (!parent)
        return false;
    if (parent->isSandboxed(SandboxSeamlessIframes))
        return false;
    if (child->isSrcdocDocument())
        return true;
    if (parent->securityOrigin()->canAccess(child->securityOrigin()))
        return true;
    return parent->securityOrigin()->canRequest(child->url());
}

void Document::initSecurityContext()
{
    initSecurityContext(DocumentInit(m_url, m_frame, m_import));
}

void Document::initSecurityContext(const DocumentInit& initializer)
{
    if (haveInitializedSecurityOrigin()) {
        ASSERT(securityOrigin());
        return;
    }

    if (!initializer.frame()) {
        // No source for a security context.
        // This can occur via document.implementation.createDocument().
        m_cookieURL = KURL(ParsedURLString, emptyString());
        setSecurityOrigin(SecurityOrigin::createUnique());
        setContentSecurityPolicy(ContentSecurityPolicy::create(this));
        return;
    }

    // In the common case, create the security context from the currently
    // loading URL with a fresh content security policy.
    m_cookieURL = m_url;
    enforceSandboxFlags(initializer.sandboxFlags());
    setSecurityOrigin(isSandboxed(SandboxOrigin) ? SecurityOrigin::createUnique() : SecurityOrigin::create(m_url));
    setContentSecurityPolicy(ContentSecurityPolicy::create(this));

    if (Settings* settings = initializer.settings()) {
        if (!settings->webSecurityEnabled()) {
            // Web security is turned off. We should let this document access every other document. This is used primary by testing
            // harnesses for web sites.
            securityOrigin()->grantUniversalAccess();
        } else if (securityOrigin()->isLocal()) {
            if (settings->allowUniversalAccessFromFileURLs()) {
                // Some clients want local URLs to have universal access, but that setting is dangerous for other clients.
                securityOrigin()->grantUniversalAccess();
            } else if (!settings->allowFileAccessFromFileURLs()) {
                // Some clients want local URLs to have even tighter restrictions by default, and not be able to access other local files.
                // FIXME 81578: The naming of this is confusing. Files with restricted access to other local files
                // still can have other privileges that can be remembered, thereby not making them unique origins.
                securityOrigin()->enforceFilePathSeparation();
            }
        }
    }

    Document* parentDocument = ownerElement() ? ownerElement()->document() : 0;
    if (parentDocument && initializer.shouldTreatURLAsSrcdocDocument()) {
        m_isSrcdocDocument = true;
        setBaseURLOverride(parentDocument->baseURL());
    }

    // FIXME: What happens if we inherit the security origin? This check may need to be later.
    // <iframe seamless src="about:blank"> likely won't work as-is.
    m_mayDisplaySeamlesslyWithParent = isEligibleForSeamless(parentDocument, this);

    if (!shouldInheritSecurityOriginFromOwner(m_url))
        return;

    // If we do not obtain a meaningful origin from the URL, then we try to
    // find one via the frame hierarchy.

    Frame* ownerFrame = initializer.ownerFrame();
    if (!ownerFrame) {
        didFailToInitializeSecurityOrigin();
        return;
    }

    if (isSandboxed(SandboxOrigin)) {
        // If we're supposed to inherit our security origin from our owner,
        // but we're also sandboxed, the only thing we inherit is the ability
        // to load local resources. This lets about:blank iframes in file://
        // URL documents load images and other resources from the file system.
        if (ownerFrame->document()->securityOrigin()->canLoadLocalResources())
            securityOrigin()->grantLoadLocalResources();
        return;
    }

    m_cookieURL = ownerFrame->document()->cookieURL();
    // We alias the SecurityOrigins to match Firefox, see Bug 15313
    // https://bugs.webkit.org/show_bug.cgi?id=15313
    setSecurityOrigin(ownerFrame->document()->securityOrigin());
}

void Document::initContentSecurityPolicy()
{
    if (!m_frame->tree()->parent() || (!shouldInheritSecurityOriginFromOwner(m_url) && !isPluginDocument()))
        return;

    contentSecurityPolicy()->copyStateFrom(m_frame->tree()->parent()->document()->contentSecurityPolicy());
}

void Document::didUpdateSecurityOrigin()
{
    if (!m_frame)
        return;
    m_frame->script()->updateSecurityOrigin();
}

bool Document::isContextThread() const
{
    return isMainThread();
}

void Document::updateURLForPushOrReplaceState(const KURL& url)
{
    Frame* f = frame();
    if (!f)
        return;

    setURL(url);
    f->loader()->setOutgoingReferrer(url);

    if (DocumentLoader* documentLoader = loader())
        documentLoader->replaceRequestURLForSameDocumentNavigation(url);
}

void Document::statePopped(PassRefPtr<SerializedScriptValue> stateObject)
{
    if (!frame())
        return;

    // Per step 11 of section 6.5.9 (history traversal) of the HTML5 spec, we
    // defer firing of popstate until we're in the complete state.
    if (m_readyState == Complete)
        enqueuePopstateEvent(stateObject);
    else
        m_pendingStateObject = stateObject;
}

void Document::updateFocusAppearanceSoon(bool restorePreviousSelection)
{
    m_updateFocusAppearanceRestoresSelection = restorePreviousSelection;
    if (!m_updateFocusAppearanceTimer.isActive())
        m_updateFocusAppearanceTimer.startOneShot(0);
}

void Document::cancelFocusAppearanceUpdate()
{
    m_updateFocusAppearanceTimer.stop();
}

void Document::updateFocusAppearanceTimerFired(Timer<Document>*)
{
    Element* element = focusedElement();
    if (!element)
        return;
    updateLayout();
    if (element->isFocusable())
        element->updateFocusAppearance(m_updateFocusAppearanceRestoresSelection);
}

void Document::attachRange(Range* range)
{
    ASSERT(!m_ranges.contains(range));
    m_ranges.add(range);
}

void Document::detachRange(Range* range)
{
    // We don't ASSERT m_ranges.contains(range) to allow us to call this
    // unconditionally to fix: https://bugs.webkit.org/show_bug.cgi?id=26044
    m_ranges.remove(range);
}

CanvasRenderingContext* Document::getCSSCanvasContext(const String& type, const String& name, int width, int height)
{
    HTMLCanvasElement* element = getCSSCanvasElement(name);
    if (!element)
        return 0;
    element->setSize(IntSize(width, height));
    return element->getContext(type);
}

HTMLCanvasElement* Document::getCSSCanvasElement(const String& name)
{
    RefPtr<HTMLCanvasElement>& element = m_cssCanvasElements.add(name, 0).iterator->value;
    if (!element) {
        element = HTMLCanvasElement::create(this);
        element->setAccelerationDisabled(true);
    }
    return element.get();
}

void Document::initDNSPrefetch()
{
    Settings* settings = this->settings();

    m_haveExplicitlyDisabledDNSPrefetch = false;
    m_isDNSPrefetchEnabled = settings && settings->dnsPrefetchingEnabled() && securityOrigin()->protocol() == "http";

    // Inherit DNS prefetch opt-out from parent frame
    if (Document* parent = parentDocument()) {
        if (!parent->isDNSPrefetchEnabled())
            m_isDNSPrefetchEnabled = false;
    }
}

void Document::parseDNSPrefetchControlHeader(const String& dnsPrefetchControl)
{
    if (equalIgnoringCase(dnsPrefetchControl, "on") && !m_haveExplicitlyDisabledDNSPrefetch) {
        m_isDNSPrefetchEnabled = true;
        return;
    }

    m_isDNSPrefetchEnabled = false;
    m_haveExplicitlyDisabledDNSPrefetch = true;
}

void Document::addConsoleMessage(MessageSource source, MessageLevel level, const String& message, unsigned long requestIdentifier)
{
    if (!isContextThread()) {
        postTask(AddConsoleMessageTask::create(source, level, message));
        return;
    }

    if (Page* page = this->page())
        page->console()->addMessage(source, level, message, requestIdentifier, this);
}

void Document::addMessage(MessageSource source, MessageLevel level, const String& message, const String& sourceURL, unsigned lineNumber, PassRefPtr<ScriptCallStack> callStack, ScriptState* state, unsigned long requestIdentifier)
{
    if (!isContextThread()) {
        postTask(AddConsoleMessageTask::create(source, level, message));
        return;
    }

    if (Page* page = this->page())
        page->console()->addMessage(source, level, message, sourceURL, lineNumber, 0, callStack, state, requestIdentifier);
}

const SecurityOrigin* Document::topOrigin() const
{
    return topDocument()->securityOrigin();
}

struct PerformTaskContext {
    WTF_MAKE_NONCOPYABLE(PerformTaskContext); WTF_MAKE_FAST_ALLOCATED;
public:
    PerformTaskContext(WeakPtr<Document> document, PassOwnPtr<ScriptExecutionContext::Task> task)
        : documentReference(document)
        , task(task)
    {
    }

    WeakPtr<Document> documentReference;
    OwnPtr<ScriptExecutionContext::Task> task;
};

void Document::didReceiveTask(void* untypedContext)
{
    ASSERT(isMainThread());

    OwnPtr<PerformTaskContext> context = adoptPtr(static_cast<PerformTaskContext*>(untypedContext));
    ASSERT(context);

    Document* document = context->documentReference.get();
    if (!document)
        return;

    Page* page = document->page();
    if ((page && page->defersLoading()) || !document->m_pendingTasks.isEmpty()) {
        document->m_pendingTasks.append(context->task.release());
        return;
    }

    context->task->performTask(document);
}

void Document::postTask(PassOwnPtr<Task> task)
{
    callOnMainThread(didReceiveTask, new PerformTaskContext(m_weakFactory.createWeakPtr(), task));
}

void Document::pendingTasksTimerFired(Timer<Document>*)
{
    while (!m_pendingTasks.isEmpty()) {
        OwnPtr<Task> task = m_pendingTasks[0].release();
        m_pendingTasks.remove(0);
        task->performTask(this);
    }
}

void Document::suspendScheduledTasks(ActiveDOMObject::ReasonForSuspension reason)
{
    ASSERT(!m_scheduledTasksAreSuspended);

    suspendScriptedAnimationControllerCallbacks();
    suspendActiveDOMObjects(reason);
    scriptRunner()->suspend();
    m_pendingTasksTimer.stop();
    if (m_parser)
        m_parser->suspendScheduledTasks();

    m_scheduledTasksAreSuspended = true;
}

void Document::resumeScheduledTasks()
{
    ASSERT(m_scheduledTasksAreSuspended);

    if (m_parser)
        m_parser->resumeScheduledTasks();
    if (!m_pendingTasks.isEmpty())
        m_pendingTasksTimer.startOneShot(0);
    scriptRunner()->resume();
    resumeActiveDOMObjects();
    resumeScriptedAnimationControllerCallbacks();

    m_scheduledTasksAreSuspended = false;
}

void Document::suspendScriptedAnimationControllerCallbacks()
{
    if (m_scriptedAnimationController)
        m_scriptedAnimationController->suspend();
}

void Document::resumeScriptedAnimationControllerCallbacks()
{
    if (m_scriptedAnimationController)
        m_scriptedAnimationController->resume();
}

String Document::displayStringModifiedByEncoding(const String& str) const
{
    if (m_decoder)
        return m_decoder->encoding().displayString(str.impl());
    return str;
}

PassRefPtr<StringImpl> Document::displayStringModifiedByEncoding(PassRefPtr<StringImpl> str) const
{
    if (m_decoder)
        return m_decoder->encoding().displayString(str);
    return str;
}

template <typename CharacterType>
void Document::displayBufferModifiedByEncodingInternal(CharacterType* buffer, unsigned len) const
{
    if (m_decoder)
        m_decoder->encoding().displayBuffer(buffer, len);
}

// Generate definitions for both character types
template void Document::displayBufferModifiedByEncodingInternal<LChar>(LChar*, unsigned) const;
template void Document::displayBufferModifiedByEncodingInternal<UChar>(UChar*, unsigned) const;

void Document::enqueuePageshowEvent(PageshowEventPersistence persisted)
{
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=36334 Pageshow event needs to fire asynchronously.
    dispatchWindowEvent(PageTransitionEvent::create(eventNames().pageshowEvent, persisted), this);
}

void Document::enqueueHashchangeEvent(const String& oldURL, const String& newURL)
{
    enqueueWindowEvent(HashChangeEvent::create(oldURL, newURL));
}

void Document::enqueuePopstateEvent(PassRefPtr<SerializedScriptValue> stateObject)
{
    if (!ContextFeatures::pushStateEnabled(this))
        return;

    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=36202 Popstate event needs to fire asynchronously
    dispatchWindowEvent(PopStateEvent::create(stateObject, domWindow() ? domWindow()->history() : 0));
}

void Document::addToTopLayer(Element* element, const Element* before)
{
    if (element->isInTopLayer())
        return;

    ASSERT(!m_topLayerElements.contains(element));
    ASSERT(!before || m_topLayerElements.contains(before));
    if (before) {
        size_t beforePosition = m_topLayerElements.find(before);
        m_topLayerElements.insert(beforePosition, element);
    } else {
        m_topLayerElements.append(element);
    }
    element->setIsInTopLayer(true);
}

void Document::removeFromTopLayer(Element* element)
{
    if (!element->isInTopLayer())
        return;
    size_t position = m_topLayerElements.find(element);
    ASSERT(position != notFound);
    m_topLayerElements.remove(position);
    element->setIsInTopLayer(false);
}

void Document::webkitExitPointerLock()
{
    if (!page())
        return;
    if (Element* target = page()->pointerLockController()->element()) {
        if (target->document() != this)
            return;
    }
    page()->pointerLockController()->requestPointerUnlock();
}

Element* Document::webkitPointerLockElement() const
{
    if (!page() || page()->pointerLockController()->lockPending())
        return 0;
    if (Element* element = page()->pointerLockController()->element()) {
        if (element->document() == this)
            return element;
    }
    return 0;
}

void Document::decrementLoadEventDelayCount()
{
    ASSERT(m_loadEventDelayCount);
    --m_loadEventDelayCount;

    if (frame() && !m_loadEventDelayCount && !m_loadEventDelayTimer.isActive())
        m_loadEventDelayTimer.startOneShot(0);
}

void Document::loadEventDelayTimerFired(Timer<Document>*)
{
    if (frame())
        frame()->loader()->checkCompleted();
}

int Document::requestAnimationFrame(PassRefPtr<RequestAnimationFrameCallback> callback)
{
    if (!m_scriptedAnimationController) {
        m_scriptedAnimationController = ScriptedAnimationController::create(this);
        // We need to make sure that we don't start up the animation controller on a background tab, for example.
        if (!page())
            m_scriptedAnimationController->suspend();
    }

    return m_scriptedAnimationController->registerCallback(callback);
}

void Document::cancelAnimationFrame(int id)
{
    if (!m_scriptedAnimationController)
        return;
    m_scriptedAnimationController->cancelCallback(id);
}

void Document::serviceScriptedAnimations(double monotonicAnimationStartTime)
{
    if (!m_scriptedAnimationController)
        return;
    m_scriptedAnimationController->serviceScriptedAnimations(monotonicAnimationStartTime);
}

PassRefPtr<Touch> Document::createTouch(DOMWindow* window, EventTarget* target, int identifier, int pageX, int pageY, int screenX, int screenY, int radiusX, int radiusY, float rotationAngle, float force) const
{
    // FIXME: It's not clear from the documentation at
    // http://developer.apple.com/library/safari/#documentation/UserExperience/Reference/DocumentAdditionsReference/DocumentAdditions/DocumentAdditions.html
    // when this method should throw and nor is it by inspection of iOS behavior. It would be nice to verify any cases where it throws under iOS
    // and implement them here. See https://bugs.webkit.org/show_bug.cgi?id=47819
    Frame* frame = window ? window->frame() : this->frame();
    return Touch::create(frame, target, identifier, screenX, screenY, pageX, pageY, radiusX, radiusY, rotationAngle, force);
}

static void wheelEventHandlerCountChanged(Document* document)
{
    Page* page = document->page();
    if (!page)
        return;

    ScrollingCoordinator* scrollingCoordinator = page->scrollingCoordinator();
    if (!scrollingCoordinator)
        return;

    FrameView* frameView = document->view();
    if (!frameView)
        return;

    scrollingCoordinator->frameViewWheelEventHandlerCountChanged(frameView);
}

void Document::didAddWheelEventHandler()
{
    ++m_wheelEventHandlerCount;
    Frame* mainFrame = page() ? page()->mainFrame() : 0;
    if (mainFrame)
        mainFrame->notifyChromeClientWheelEventHandlerCountChanged();

    wheelEventHandlerCountChanged(this);
}

void Document::didRemoveWheelEventHandler()
{
    ASSERT(m_wheelEventHandlerCount > 0);
    --m_wheelEventHandlerCount;
    Frame* mainFrame = page() ? page()->mainFrame() : 0;
    if (mainFrame)
        mainFrame->notifyChromeClientWheelEventHandlerCountChanged();

    wheelEventHandlerCountChanged(this);
}

void Document::didAddTouchEventHandler(Node* handler)
{
    if (!m_touchEventTargets.get())
        m_touchEventTargets = adoptPtr(new TouchEventTargetSet);
    m_touchEventTargets->add(handler);
    if (Document* parent = parentDocument()) {
        parent->didAddTouchEventHandler(this);
        return;
    }
    if (Page* page = this->page()) {
        if (ScrollingCoordinator* scrollingCoordinator = page->scrollingCoordinator())
            scrollingCoordinator->touchEventTargetRectsDidChange(this);
        if (m_touchEventTargets->size() == 1)
            page->chrome().client()->needTouchEvents(true);
    }
}

void Document::didRemoveTouchEventHandler(Node* handler)
{
    if (!m_touchEventTargets.get())
        return;
    ASSERT(m_touchEventTargets->contains(handler));
    m_touchEventTargets->remove(handler);
    if (Document* parent = parentDocument()) {
        parent->didRemoveTouchEventHandler(this);
        return;
    }

    Page* page = this->page();
    if (!page)
        return;
    if (ScrollingCoordinator* scrollingCoordinator = page->scrollingCoordinator())
        scrollingCoordinator->touchEventTargetRectsDidChange(this);
    if (m_touchEventTargets->size())
        return;
    for (const Frame* frame = page->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (frame->document() && frame->document()->hasTouchEventHandlers())
            return;
    }
    page->chrome().client()->needTouchEvents(false);
}

void Document::didRemoveEventTargetNode(Node* handler)
{
    if (m_touchEventTargets) {
        m_touchEventTargets->removeAll(handler);
        if ((handler == this || m_touchEventTargets->isEmpty()) && parentDocument())
            parentDocument()->didRemoveEventTargetNode(this);
    }
}

void Document::resetLastHandledUserGestureTimestamp()
{
    m_lastHandledUserGestureTimestamp = currentTime();
}

HTMLIFrameElement* Document::seamlessParentIFrame() const
{
    if (!shouldDisplaySeamlesslyWithParent())
        return 0;

    return toHTMLIFrameElement(this->ownerElement());
}

bool Document::shouldDisplaySeamlesslyWithParent() const
{
    if (!RuntimeEnabledFeatures::seamlessIFramesEnabled())
        return false;
    HTMLFrameOwnerElement* ownerElement = this->ownerElement();
    if (!ownerElement)
        return false;
    return m_mayDisplaySeamlesslyWithParent && ownerElement->hasTagName(iframeTag) && ownerElement->fastHasAttribute(seamlessAttr);
}

DocumentLoader* Document::loader() const
{
    if (!m_frame)
        return 0;

    DocumentLoader* loader = m_frame->loader()->documentLoader();
    if (!loader)
        return 0;

    if (m_frame->document() != this)
        return 0;

    return loader;
}

IntSize Document::viewportSize() const
{
    if (!view())
        return IntSize();
    return view()->visibleContentRect(ScrollableArea::IncludeScrollbars).size();
}

IntSize Document::initialViewportSize() const
{
    if (!view())
        return IntSize();
    return view()->unscaledVisibleContentSize(ScrollableArea::IncludeScrollbars);
}

Node* eventTargetNodeForDocument(Document* doc)
{
    if (!doc)
        return 0;
    Node* node = doc->focusedElement();
    if (!node && doc->isPluginDocument()) {
        PluginDocument* pluginDocument = toPluginDocument(doc);
        node =  pluginDocument->pluginNode();
    }
    if (!node && doc->isHTMLDocument())
        node = doc->body();
    if (!node)
        node = doc->documentElement();
    return node;
}

void Document::adjustFloatQuadsForScrollAndAbsoluteZoom(Vector<FloatQuad>& quads, RenderObject* renderer)
{
    if (!view())
        return;

    LayoutRect visibleContentRect = view()->visibleContentRect();
    for (size_t i = 0; i < quads.size(); ++i) {
        quads[i].move(-visibleContentRect.x(), -visibleContentRect.y());
        adjustFloatQuadForAbsoluteZoom(quads[i], renderer);
    }
}

void Document::adjustFloatRectForScrollAndAbsoluteZoom(FloatRect& rect, RenderObject* renderer)
{
    if (!view())
        return;

    LayoutRect visibleContentRect = view()->visibleContentRect();
    rect.move(-visibleContentRect.x(), -visibleContentRect.y());
    adjustFloatRectForAbsoluteZoom(rect, renderer);
}

bool Document::hasActiveParser()
{
    return m_activeParserCount || (m_parser && m_parser->processingData());
}

void Document::decrementActiveParserCount()
{
    --m_activeParserCount;
    if (!frame())
        return;
    // FIXME: This should always be enabled, but it seems to cause
    // http/tests/security/feed-urls-from-remote.html to timeout on Mac WK1
    // see http://webkit.org/b/110554 and http://webkit.org/b/110401
    loader()->checkLoadComplete();
    frame()->loader()->checkLoadComplete();
}

void Document::setContextFeatures(PassRefPtr<ContextFeatures> features)
{
    m_contextFeatures = features;
}

static RenderObject* nearestCommonHoverAncestor(RenderObject* obj1, RenderObject* obj2)
{
    if (!obj1 || !obj2)
        return 0;

    for (RenderObject* currObj1 = obj1; currObj1; currObj1 = currObj1->hoverAncestor()) {
        for (RenderObject* currObj2 = obj2; currObj2; currObj2 = currObj2->hoverAncestor()) {
            if (currObj1 == currObj2)
                return currObj1;
        }
    }

    return 0;
}

void Document::updateHoverActiveState(const HitTestRequest& request, Element* innerElement, const PlatformMouseEvent* event)
{
    ASSERT(!request.readOnly());

    Element* innerElementInDocument = innerElement;
    while (innerElementInDocument && innerElementInDocument->document() != this) {
        innerElementInDocument->document()->updateHoverActiveState(request, innerElementInDocument, event);
        innerElementInDocument = innerElementInDocument->document()->ownerElement();
    }

    Element* oldActiveElement = activeElement();
    if (oldActiveElement && !request.active()) {
        // We are clearing the :active chain because the mouse has been released.
        for (RenderObject* curr = oldActiveElement->renderer(); curr; curr = curr->parent()) {
            if (curr->node()) {
                ASSERT(!curr->node()->isTextNode());
                curr->node()->setActive(false);
                m_userActionElements.setInActiveChain(curr->node(), false);
            }
        }
        setActiveElement(0);
    } else {
        Element* newActiveElement = innerElementInDocument;
        if (!oldActiveElement && newActiveElement && request.active() && !request.touchMove()) {
            // We are setting the :active chain and freezing it. If future moves happen, they
            // will need to reference this chain.
            for (RenderObject* curr = newActiveElement->renderer(); curr; curr = curr->parent()) {
                if (curr->node() && !curr->isText())
                    m_userActionElements.setInActiveChain(curr->node(), true);
            }

            setActiveElement(newActiveElement);
        }
    }
    // If the mouse has just been pressed, set :active on the chain. Those (and only those)
    // nodes should remain :active until the mouse is released.
    bool allowActiveChanges = !oldActiveElement && activeElement();

    // If the mouse is down and if this is a mouse move event, we want to restrict changes in
    // :hover/:active to only apply to elements that are in the :active chain that we froze
    // at the time the mouse went down.
    bool mustBeInActiveChain = request.active() && request.move();

    RefPtr<Node> oldHoverNode = hoverNode();

    // A touch release does not set a new hover target; setting the element we're working with to 0
    // will clear the chain of hovered elements all the way to the top of the tree.
    if (request.touchRelease())
        innerElementInDocument = 0;

    // Check to see if the hovered node has changed.
    // If it hasn't, we do not need to do anything.
    Node* newHoverNode = innerElementInDocument;
    while (newHoverNode && !newHoverNode->renderer())
        newHoverNode = newHoverNode->parentOrShadowHostNode();

    // Update our current hover node.
    setHoverNode(newHoverNode);

    // We have two different objects. Fetch their renderers.
    RenderObject* oldHoverObj = oldHoverNode ? oldHoverNode->renderer() : 0;
    RenderObject* newHoverObj = newHoverNode ? newHoverNode->renderer() : 0;

    // Locate the common ancestor render object for the two renderers.
    RenderObject* ancestor = nearestCommonHoverAncestor(oldHoverObj, newHoverObj);

    Vector<RefPtr<Node>, 32> nodesToRemoveFromChain;
    Vector<RefPtr<Node>, 32> nodesToAddToChain;

    if (oldHoverObj != newHoverObj) {
        // If the old hovered node is not nil but it's renderer is, it was probably detached as part of the :hover style
        // (for instance by setting display:none in the :hover pseudo-class). In this case, the old hovered element (and its ancestors)
        // must be updated, to ensure it's normal style is re-applied.
        if (oldHoverNode && !oldHoverObj) {
            for (Node* node = oldHoverNode.get(); node; node = node->parentNode()) {
                if (!mustBeInActiveChain || (node->isElementNode() && toElement(node)->inActiveChain()))
                    nodesToRemoveFromChain.append(node);
            }

        }

        // The old hover path only needs to be cleared up to (and not including) the common ancestor;
        for (RenderObject* curr = oldHoverObj; curr && curr != ancestor; curr = curr->hoverAncestor()) {
            if (curr->node() && !curr->isText() && (!mustBeInActiveChain || curr->node()->inActiveChain()))
                nodesToRemoveFromChain.append(curr->node());
        }
    }

    // Now set the hover state for our new object up to the root.
    for (RenderObject* curr = newHoverObj; curr; curr = curr->hoverAncestor()) {
        if (curr->node() && !curr->isText() && (!mustBeInActiveChain || curr->node()->inActiveChain()))
            nodesToAddToChain.append(curr->node());
    }

    // mouseenter and mouseleave events do not bubble, so they are dispatched iff there is a capturing
    // event handler on an ancestor or a normal event handler on the element itself. This special
    // handling is necessary to avoid O(n^2) capturing event handler checks. We'll check the previously
    // hovered node's ancestor tree for 'mouseleave' handlers here, then check the newly hovered node's
    // ancestor tree for 'mouseenter' handlers after dispatching the 'mouseleave' events (as the handler
    // for 'mouseleave' might set a capturing 'mouseenter' handler, odd as that might be).
    bool ancestorHasCapturingMouseleaveListener = false;
    if (event && newHoverNode != oldHoverNode.get()) {
        for (Node* node = oldHoverNode.get(); node; node = node->parentOrShadowHostNode()) {
            if (node->hasCapturingEventListeners(eventNames().mouseleaveEvent)) {
                ancestorHasCapturingMouseleaveListener = true;
                break;
            }
        }
    }

    size_t removeCount = nodesToRemoveFromChain.size();
    for (size_t i = 0; i < removeCount; ++i) {
        nodesToRemoveFromChain[i]->setHovered(false);
        if (event && (ancestorHasCapturingMouseleaveListener || nodesToRemoveFromChain[i]->hasEventListeners(eventNames().mouseleaveEvent)))
            nodesToRemoveFromChain[i]->dispatchMouseEvent(*event, eventNames().mouseleaveEvent, 0, newHoverNode);
    }

    bool ancestorHasCapturingMouseenterListener = false;
    if (event && newHoverNode != oldHoverNode.get()) {
        for (Node* node = newHoverNode; node; node = node->parentOrShadowHostNode()) {
            if (node->hasCapturingEventListeners(eventNames().mouseenterEvent)) {
                ancestorHasCapturingMouseenterListener = true;
                break;
            }
        }
    }

    bool sawCommonAncestor = false;
    size_t addCount = nodesToAddToChain.size();
    for (size_t i = 0; i < addCount; ++i) {
        // Elements past the common ancestor do not change hover state, but might change active state.
        if (ancestor && nodesToAddToChain[i] == ancestor->node())
            sawCommonAncestor = true;
        if (allowActiveChanges)
            nodesToAddToChain[i]->setActive(true);
        if (!sawCommonAncestor) {
            nodesToAddToChain[i]->setHovered(true);
            if (event && (ancestorHasCapturingMouseenterListener || nodesToAddToChain[i]->hasEventListeners(eventNames().mouseenterEvent)))
                nodesToAddToChain[i]->dispatchMouseEvent(*event, eventNames().mouseenterEvent, 0, oldHoverNode.get());
        }
    }

    updateStyleIfNeeded();
}

bool Document::haveStylesheetsLoaded() const
{
    return !m_styleSheetCollection->hasPendingSheets() || m_ignorePendingStylesheets;
}

Locale& Document::getCachedLocale(const AtomicString& locale)
{
    AtomicString localeKey = locale;
    if (locale.isEmpty() || !RuntimeEnabledFeatures::langAttributeAwareFormControlUIEnabled())
        localeKey = defaultLanguage();
    LocaleIdentifierToLocaleMap::AddResult result = m_localeCache.add(localeKey, nullptr);
    if (result.isNewEntry)
        result.iterator->value = Locale::create(localeKey);
    return *(result.iterator->value);
}

Document* Document::ensureTemplateDocument()
{
    if (const Document* document = templateDocument())
        return const_cast<Document*>(document);

    if (isHTMLDocument())
        m_templateDocument = HTMLDocument::create(DocumentInit(blankURL()));
    else
        m_templateDocument = Document::create(DocumentInit(blankURL()));

    m_templateDocument->setTemplateDocumentHost(this); // balanced in dtor.

    return m_templateDocument.get();
}

PassRefPtr<FontLoader> Document::fontloader()
{
    if (!m_fontloader)
        m_fontloader = FontLoader::create(this);
    return m_fontloader;
}

void Document::didAssociateFormControl(Element* element)
{
    if (!frame() || !frame()->page())
        return;
    m_associatedFormControls.add(element);
    if (!m_didAssociateFormControlsTimer.isActive())
        m_didAssociateFormControlsTimer.startOneShot(0);
}

void Document::didAssociateFormControlsTimerFired(Timer<Document>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_didAssociateFormControlsTimer);
    if (!frame() || !frame()->page())
        return;

    Vector<RefPtr<Element> > associatedFormControls;
    copyToVector(m_associatedFormControls, associatedFormControls);

    frame()->page()->chrome().client()->didAssociateFormControls(associatedFormControls);
    m_associatedFormControls.clear();
}

PassOwnPtr<LifecycleNotifier> Document::createLifecycleNotifier()
{
    return DocumentLifecycleNotifier::create(this);
}

DocumentLifecycleNotifier* Document::lifecycleNotifier()
{
    return static_cast<DocumentLifecycleNotifier*>(ScriptExecutionContext::lifecycleNotifier());
}

} // namespace WebCore
