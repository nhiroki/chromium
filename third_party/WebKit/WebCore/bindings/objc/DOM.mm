/*
 * Copyright (C) 2004-2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 James G. Speth (speth@end.com)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "DOM.h"

#import "CDATASection.h"
#import "CSSStyleSheet.h"
#import "Comment.h"
#import "DOMImplementationFront.h"
#import "DOMInternal.h"
#import "DOMPrivate.h"
#import "DeprecatedValueList.h"
#import "Document.h"
#import "DocumentFragment.h"
#import "DocumentType.h"
#import "EntityReference.h"
#import "Event.h"
#import "EventListener.h"
#import "FontData.h"
#import "FoundationExtras.h"
#import "FrameMac.h"
#import "HTMLDocument.h"
#import "HTMLNames.h"
#import "HTMLPlugInElement.h"
#import "IntRect.h"
#import "NodeFilter.h"
#import "NodeFilterCondition.h"
#import "NodeIterator.h"
#import "NodeList.h"
#import "ProcessingInstruction.h"
#import "QualifiedName.h"
#import "Range.h"
#import "RenderImage.h"
#import "Text.h"
#import "TreeWalker.h"
#import "WebScriptObjectPrivate.h"
#import "csshelper.h"
#import <objc/objc-class.h>
#import <wtf/HashMap.h>

using namespace WebCore::HTMLNames;

class ObjCEventListener : public WebCore::EventListener {
public:
    static ObjCEventListener* find(id <DOMEventListener>);
    static ObjCEventListener* create(id <DOMEventListener>);

private:
    ObjCEventListener(id <DOMEventListener>);
    virtual ~ObjCEventListener();

    virtual void handleEvent(WebCore::Event*, bool isWindowEvent);

    id <DOMEventListener> m_listener;
};

typedef HashMap<id, ObjCEventListener*> ListenerMap;
typedef HashMap<WebCore::AtomicStringImpl*, Class> ObjCClassMap;

static ObjCClassMap* elementClassMap;
static ListenerMap* listenerMap;


//------------------------------------------------------------------------------------------
// DOMNode

static void addElementClass(const WebCore::QualifiedName& tag, Class objCClass)
{
    elementClassMap->set(tag.localName().impl(), objCClass);
}

static void createHTMLElementClassMap()
{
    // Create the table.
    elementClassMap = new ObjCClassMap;
    
    // Populate it with HTML element classes.
    addElementClass(aTag, [DOMHTMLAnchorElement class]);
    addElementClass(appletTag, [DOMHTMLAppletElement class]);
    addElementClass(areaTag, [DOMHTMLAreaElement class]);
    addElementClass(baseTag, [DOMHTMLBaseElement class]);
    addElementClass(basefontTag, [DOMHTMLBaseFontElement class]);
    addElementClass(bodyTag, [DOMHTMLBodyElement class]);
    addElementClass(brTag, [DOMHTMLBRElement class]);
    addElementClass(buttonTag, [DOMHTMLButtonElement class]);
    addElementClass(canvasTag, [DOMHTMLImageElement class]);
    addElementClass(captionTag, [DOMHTMLTableCaptionElement class]);
    addElementClass(colTag, [DOMHTMLTableColElement class]);
    addElementClass(colgroupTag, [DOMHTMLTableColElement class]);
    addElementClass(dirTag, [DOMHTMLDirectoryElement class]);
    addElementClass(divTag, [DOMHTMLDivElement class]);
    addElementClass(dlTag, [DOMHTMLDListElement class]);
    addElementClass(fieldsetTag, [DOMHTMLFieldSetElement class]);
    addElementClass(fontTag, [DOMHTMLFontElement class]);
    addElementClass(formTag, [DOMHTMLFormElement class]);
    addElementClass(frameTag, [DOMHTMLFrameElement class]);
    addElementClass(framesetTag, [DOMHTMLFrameSetElement class]);
    addElementClass(h1Tag, [DOMHTMLHeadingElement class]);
    addElementClass(h2Tag, [DOMHTMLHeadingElement class]);
    addElementClass(h3Tag, [DOMHTMLHeadingElement class]);
    addElementClass(h4Tag, [DOMHTMLHeadingElement class]);
    addElementClass(h5Tag, [DOMHTMLHeadingElement class]);
    addElementClass(h6Tag, [DOMHTMLHeadingElement class]);
    addElementClass(headTag, [DOMHTMLHeadElement class]);
    addElementClass(hrTag, [DOMHTMLHRElement class]);
    addElementClass(htmlTag, [DOMHTMLHtmlElement class]);
    addElementClass(iframeTag, [DOMHTMLIFrameElement class]);
    addElementClass(imgTag, [DOMHTMLImageElement class]);
    addElementClass(inputTag, [DOMHTMLInputElement class]);
    addElementClass(isindexTag, [DOMHTMLIsIndexElement class]);
    addElementClass(labelTag, [DOMHTMLLabelElement class]);
    addElementClass(legendTag, [DOMHTMLLegendElement class]);
    addElementClass(liTag, [DOMHTMLLIElement class]);
    addElementClass(linkTag, [DOMHTMLLinkElement class]);
    addElementClass(listingTag, [DOMHTMLPreElement class]);
    addElementClass(mapTag, [DOMHTMLMapElement class]);
    addElementClass(menuTag, [DOMHTMLMenuElement class]);
    addElementClass(metaTag, [DOMHTMLMetaElement class]);
    addElementClass(objectTag, [DOMHTMLObjectElement class]);
    addElementClass(olTag, [DOMHTMLOListElement class]);
    addElementClass(optgroupTag, [DOMHTMLOptGroupElement class]);
    addElementClass(optionTag, [DOMHTMLOptionElement class]);
    addElementClass(pTag, [DOMHTMLParagraphElement class]);
    addElementClass(paramTag, [DOMHTMLParamElement class]);
    addElementClass(preTag, [DOMHTMLPreElement class]);
    addElementClass(qTag, [DOMHTMLQuoteElement class]);
    addElementClass(scriptTag, [DOMHTMLScriptElement class]);
    addElementClass(selectTag, [DOMHTMLSelectElement class]);
    addElementClass(styleTag, [DOMHTMLStyleElement class]);
    addElementClass(tableTag, [DOMHTMLTableElement class]);
    addElementClass(tbodyTag, [DOMHTMLTableSectionElement class]);
    addElementClass(tdTag, [DOMHTMLTableCellElement class]);
    addElementClass(textareaTag, [DOMHTMLTextAreaElement class]);
    addElementClass(tfootTag, [DOMHTMLTableSectionElement class]);
    addElementClass(theadTag, [DOMHTMLTableSectionElement class]);
    addElementClass(titleTag, [DOMHTMLTitleElement class]);
    addElementClass(trTag, [DOMHTMLTableRowElement class]);
    addElementClass(ulTag, [DOMHTMLUListElement class]);

    // FIXME: Reflect marquee once the API has been determined.
}

static Class elementClass(const WebCore::AtomicString& tagName)
{
    if (!elementClassMap)
        createHTMLElementClassMap();
    Class objcClass = elementClassMap->get(tagName.impl());
    if (!objcClass)
        objcClass = [DOMHTMLElement class];
    return objcClass;
}

@implementation DOMNode (WebCoreInternal)

// FIXME: should this go in the main implementation?
- (NSString *)description
{
    if (!_internal)
        return [NSString stringWithFormat:@"<%@: null>", [[self class] description], self];
    
    NSString *value = [self nodeValue];
    if (value)
        return [NSString stringWithFormat:@"<%@ [%@]: %p '%@'>",
            [[self class] description], [self nodeName], _internal, value];

    return [NSString stringWithFormat:@"<%@ [%@]: %p>", [[self class] description], [self nodeName], _internal];
}

- (id)_initWithNode:(WebCore::Node *)impl
{
    ASSERT(impl);

    [super _init];
    _internal = reinterpret_cast<DOMObjectInternal*>(impl);
    impl->ref();
    WebCore::addDOMWrapper(self, impl);
    return self;
}

+ (DOMNode *)_nodeWith:(WebCore::Node *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = WebCore::getDOMWrapper(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    Class wrapperClass = nil;
    switch (impl->nodeType()) {
        case WebCore::Node::ELEMENT_NODE:
            if (impl->isHTMLElement())
                wrapperClass = elementClass(static_cast<WebCore::HTMLElement*>(impl)->localName());
            else
                wrapperClass = [DOMElement class];
            break;
        case WebCore::Node::ATTRIBUTE_NODE:
            wrapperClass = [DOMAttr class];
            break;
        case WebCore::Node::TEXT_NODE:
            wrapperClass = [DOMText class];
            break;
        case WebCore::Node::CDATA_SECTION_NODE:
            wrapperClass = [DOMCDATASection class];
            break;
        case WebCore::Node::ENTITY_REFERENCE_NODE:
            wrapperClass = [DOMEntityReference class];
            break;
        case WebCore::Node::ENTITY_NODE:
            wrapperClass = [DOMEntity class];
            break;
        case WebCore::Node::PROCESSING_INSTRUCTION_NODE:
            wrapperClass = [DOMProcessingInstruction class];
            break;
        case WebCore::Node::COMMENT_NODE:
            wrapperClass = [DOMComment class];
            break;
        case WebCore::Node::DOCUMENT_NODE:
            if (static_cast<WebCore::Document*>(impl)->isHTMLDocument())
                wrapperClass = [DOMHTMLDocument class];
            else
                wrapperClass = [DOMDocument class];
            break;
        case WebCore::Node::DOCUMENT_TYPE_NODE:
            wrapperClass = [DOMDocumentType class];
            break;
        case WebCore::Node::DOCUMENT_FRAGMENT_NODE:
            wrapperClass = [DOMDocumentFragment class];
            break;
        case WebCore::Node::NOTATION_NODE:
            wrapperClass = [DOMNotation class];
            break;
        case WebCore::Node::XPATH_NAMESPACE_NODE:
            // FIXME: Create an XPath objective C wrapper
            // See http://bugzilla.opendarwin.org/show_bug.cgi?id=8755
            return nil;
    }
    return [[[wrapperClass alloc] _initWithNode:impl] autorelease];
}

- (WebCore::Node *)_node
{
    return reinterpret_cast<WebCore::Node*>(_internal);
}

- (const KJS::Bindings::RootObject *)_executionContext
{
    if (WebCore::Node *n = [self _node]) {
        if (WebCore::FrameMac *f = Mac(n->document()->frame()))
            return f->executionContextForDOM();
    }
    return 0;
}

@end

@implementation DOMNode (DOMNodeExtensions)

// FIXME: this should be implemented in the implementation
- (NSRect)boundingBox
{
    WebCore::RenderObject *renderer = [self _node]->renderer();
    if (renderer)
        return renderer->absoluteBoundingBoxRect();
    return NSZeroRect;
}

// FIXME: this should be implemented in the implementation
- (NSArray *)lineBoxRects
{
    WebCore::RenderObject *renderer = [self _node]->renderer();
    if (renderer) {
        Vector<WebCore::IntRect> rects;
        renderer->lineBoxRects(rects);
        size_t size = rects.size();
        NSMutableArray *results = [NSMutableArray arrayWithCapacity:size];
        for (size_t i = 0; i < size; ++i)
            [results addObject:[NSValue valueWithRect:rects[i]]];
        return results;
    }
    return nil;
}

@end

// FIXME: this should be auto-generated
@implementation DOMNode (DOMEventTarget)

- (void)addEventListener:(NSString *)type listener:(id <DOMEventListener>)listener useCapture:(BOOL)useCapture
{
    if (![self _node]->isEventTargetNode())
        WebCore::raiseDOMException(DOM_NOT_SUPPORTED_ERR);
    
    WebCore::EventListener *wrapper = ObjCEventListener::create(listener);
    WebCore::EventTargetNodeCast([self _node])->addEventListener(type, wrapper, useCapture);
    wrapper->deref();
}

- (void)addEventListener:(NSString *)type :(id <DOMEventListener>)listener :(BOOL)useCapture
{
    // FIXME: this method can be removed once Mail changes to use the new method <rdar://problem/4746649>
    [self addEventListener:type listener:listener useCapture:useCapture];
}

- (void)removeEventListener:(NSString *)type listener:(id <DOMEventListener>)listener useCapture:(BOOL)useCapture
{
    if (![self _node]->isEventTargetNode())
        WebCore::raiseDOMException(DOM_NOT_SUPPORTED_ERR);

    if (WebCore::EventListener *wrapper = ObjCEventListener::find(listener))
        WebCore::EventTargetNodeCast([self _node])->removeEventListener(type, wrapper, useCapture);
}

- (void)removeEventListener:(NSString *)type :(id <DOMEventListener>)listener :(BOOL)useCapture
{
    // FIXME: this method can be removed once Mail changes to use the new method <rdar://problem/4746649>
    [self removeEventListener:type listener:listener useCapture:useCapture];
}

- (BOOL)dispatchEvent:(DOMEvent *)event
{
    if (![self _node]->isEventTargetNode())
        WebCore::raiseDOMException(DOM_NOT_SUPPORTED_ERR);

    WebCore::ExceptionCode ec = 0;
    BOOL result = WebCore::EventTargetNodeCast([self _node])->dispatchEvent([event _event], ec);
    WebCore::raiseOnDOMError(ec);
    return result;
}

@end

//------------------------------------------------------------------------------------------
// DOMElement

// FIXME: this should be auto-generated in DOMElement.mm
@implementation DOMElement (DOMElementAppKitExtensions)

// FIXME: this should be implemented in the implementation
- (NSImage*)image
{
    WebCore::RenderObject* renderer = [self _element]->renderer();
    if (renderer && renderer->isImage()) {
        WebCore::RenderImage* img = static_cast<WebCore::RenderImage*>(renderer);
        if (img->cachedImage() && !img->cachedImage()->isErrorImage())
            return img->cachedImage()->image()->getNSImage();
    }
    return nil;
}

@end

@implementation DOMElement (WebPrivate)

// FIXME: this should be implemented in the implementation
- (NSFont *)_font
{
    WebCore::RenderObject* renderer = [self _element]->renderer();
    if (renderer)
        return renderer->style()->font().primaryFont()->getNSFont();
    return nil;
}

// FIXME: this should be implemented in the implementation
- (NSData *)_imageTIFFRepresentation
{
    WebCore::RenderObject* renderer = [self _element]->renderer();
    if (renderer && renderer->isImage()) {
        WebCore::RenderImage* img = static_cast<WebCore::RenderImage*>(renderer);
        if (img->cachedImage() && !img->cachedImage()->isErrorImage())
            return (NSData*)(img->cachedImage()->image()->getTIFFRepresentation());
    }
    return nil;
}

// FIXME: this should be implemented in the implementation
- (NSURL *)_getURLAttribute:(NSString *)name
{
    ASSERT(name);
    WebCore::Element* element = [self _element];
    ASSERT(element);
    return WebCore::KURL(element->document()->completeURL(parseURL(element->getAttribute(name)).deprecatedString())).getNSURL();
}

// FIXME: this should be implemented in the implementation
- (void *)_NPObject
{
    WebCore::Element* element = [self _element];
    if (element->hasTagName(appletTag) || element->hasTagName(embedTag) || element->hasTagName(objectTag))
        return static_cast<WebCore::HTMLPlugInElement*>(element)->getNPObject();
    return 0;
}

// FIXME: this should be implemented in the implementation
- (BOOL)isFocused
{
    WebCore::Element* impl = [self _element];
    if (impl->document()->focusNode() == impl)
        return YES;
    return NO;
}

@end


//------------------------------------------------------------------------------------------
// DOMRange

@implementation DOMRange (WebPrivate)

- (NSString *)description
{
    if (!_internal)
        return @"<DOMRange: null>";
    return [NSString stringWithFormat:@"<DOMRange: %@ %d %@ %d>",
               [self startContainer], [self startOffset], [self endContainer], [self endOffset]];
}

// FIXME: this should be removed as soon as all internal Apple uses of it have been replaced with
// calls to the public method - (NSString *)text.
- (NSString *)_text
{
    return [self text];
}

@end


//------------------------------------------------------------------------------------------
// DOMNodeFilter

// FIXME: This implementation should be in it's own file.

@implementation DOMNodeFilter

- (id)_initWithNodeFilter:(WebCore::NodeFilter *)impl
{
    ASSERT(impl);

    [super _init];
    _internal = reinterpret_cast<DOMObjectInternal*>(impl);
    impl->ref();
    WebCore::addDOMWrapper(self, impl);
    return self;
}

+ (DOMNodeFilter *)_nodeFilterWith:(WebCore::NodeFilter *)impl
{
    if (!impl)
        return nil;
    
    id cachedInstance;
    cachedInstance = WebCore::getDOMWrapper(impl);
    if (cachedInstance)
        return [[cachedInstance retain] autorelease];
    
    return [[[self alloc] _initWithNodeFilter:impl] autorelease];
}

- (WebCore::NodeFilter *)_nodeFilter
{
    return reinterpret_cast<WebCore::NodeFilter*>(_internal);
}

- (void)dealloc
{
    if (_internal)
        reinterpret_cast<WebCore::NodeFilter*>(_internal)->deref();
    [super dealloc];
}

- (void)finalize
{
    if (_internal)
        reinterpret_cast<WebCore::NodeFilter*>(_internal)->deref();
    [super finalize];
}

- (short)acceptNode:(DOMNode *)node
{
    return [self _nodeFilter]->acceptNode([node _node]);
}

@end


//------------------------------------------------------------------------------------------
// ObjCNodeFilterCondition

class ObjCNodeFilterCondition : public WebCore::NodeFilterCondition {
public:
    ObjCNodeFilterCondition(id <DOMNodeFilter>);
    virtual ~ObjCNodeFilterCondition();
    virtual short acceptNode(WebCore::Node*) const;

private:
    ObjCNodeFilterCondition(const ObjCNodeFilterCondition&);
    ObjCNodeFilterCondition &operator=(const ObjCNodeFilterCondition&);

    id <DOMNodeFilter> m_filter;
};

ObjCNodeFilterCondition::ObjCNodeFilterCondition(id <DOMNodeFilter> filter)
    : m_filter(filter)
{
    ASSERT(m_filter);
    CFRetain(m_filter);
}

ObjCNodeFilterCondition::~ObjCNodeFilterCondition()
{
    CFRelease(m_filter);
}

short ObjCNodeFilterCondition::acceptNode(WebCore::Node* node) const
{
    if (!node)
        return WebCore::NodeFilter::FILTER_REJECT;
    return [m_filter acceptNode:[DOMNode _nodeWith:node]];
}


//------------------------------------------------------------------------------------------
// DOMDocument (DOMDocumentTraversal)

// FIXME: this should be auto-generated in DOMDocument.mm
@implementation DOMDocument (DOMDocumentTraversal)

- (DOMNodeIterator *)createNodeIterator:(DOMNode *)root whatToShow:(unsigned)whatToShow filter:(id <DOMNodeFilter>)filter expandEntityReferences:(BOOL)expandEntityReferences
{
    RefPtr<WebCore::NodeFilter> cppFilter;
    if (filter)
        cppFilter = new WebCore::NodeFilter(new ObjCNodeFilterCondition(filter));
    WebCore::ExceptionCode ec = 0;
    RefPtr<WebCore::NodeIterator> impl = [self _document]->createNodeIterator([root _node], whatToShow, cppFilter, expandEntityReferences, ec);
    WebCore::raiseOnDOMError(ec);
    return [DOMNodeIterator _nodeIteratorWith:impl.get() filter:filter];
}

- (DOMTreeWalker *)createTreeWalker:(DOMNode *)root whatToShow:(unsigned)whatToShow filter:(id <DOMNodeFilter>)filter expandEntityReferences:(BOOL)expandEntityReferences
{
    RefPtr<WebCore::NodeFilter> cppFilter;
    if (filter)
        cppFilter = new WebCore::NodeFilter(new ObjCNodeFilterCondition(filter));
    WebCore::ExceptionCode ec = 0;
    RefPtr<WebCore::TreeWalker> impl = [self _document]->createTreeWalker([root _node], whatToShow, cppFilter, expandEntityReferences, ec);
    WebCore::raiseOnDOMError(ec);
    return [DOMTreeWalker _treeWalkerWith:impl.get() filter:filter];
}

@end

@implementation DOMDocument (DOMDocumentTraversalDeprecated)

- (DOMNodeIterator *)createNodeIterator:(DOMNode *)root :(unsigned)whatToShow :(id <DOMNodeFilter>)filter :(BOOL)expandEntityReferences
{
    return [self createNodeIterator:root whatToShow:whatToShow filter:filter expandEntityReferences:expandEntityReferences];
}

- (DOMTreeWalker *)createTreeWalker:(DOMNode *)root :(unsigned)whatToShow :(id <DOMNodeFilter>)filter :(BOOL)expandEntityReferences
{
    return [self createTreeWalker:root whatToShow:whatToShow filter:filter expandEntityReferences:expandEntityReferences];
}

@end

//------------------------------------------------------------------------------------------
// ObjCEventListener

ObjCEventListener* ObjCEventListener::find(id <DOMEventListener> listener)
{
    if (ListenerMap* map = listenerMap)
        return map->get(listener);
    return 0;
}

ObjCEventListener *ObjCEventListener::create(id <DOMEventListener> listener)
{
    ObjCEventListener* wrapper = find(listener);
    if (!wrapper)
        wrapper = new ObjCEventListener(listener);
    wrapper->ref();
    return wrapper;
}

ObjCEventListener::ObjCEventListener(id <DOMEventListener> listener)
    : m_listener([listener retain])
{
    ListenerMap* map = listenerMap;
    if (!map) {
        map = new ListenerMap;
        listenerMap = map;
    }
    map->set(listener, this);
}

ObjCEventListener::~ObjCEventListener()
{
    listenerMap->remove(m_listener);
    [m_listener release];
}

void ObjCEventListener::handleEvent(WebCore::Event* event, bool)
{
    [m_listener handleEvent:[DOMEvent _eventWith:event]];
}
