/*
 * Copyright (C) 2006, 2007 Rob Buis
 * Copyright (C) 2008 Apple, Inc. All rights reserved.
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
#include "core/dom/StyleElement.h"

#include "core/css/MediaList.h"
#include "core/css/MediaQueryEvaluator.h"
#include "core/css/StyleSheetContents.h"
#include "core/dom/Document.h"
#include "core/dom/DocumentStyleSheetCollection.h"
#include "core/dom/Element.h"
#include "core/dom/ScriptableDocumentParser.h"
#include "core/page/ContentSecurityPolicy.h"
#include <wtf/text/StringBuilder.h>
#include <wtf/text/TextPosition.h>

namespace WebCore {

static bool isValidStyleChild(Node* node)
{
    ASSERT(node);
    Node::NodeType nodeType = node->nodeType();
    return nodeType == Node::TEXT_NODE || nodeType == Node::CDATA_SECTION_NODE;
}

static bool isCSS(Element* element, const AtomicString& type)
{
    return type.isEmpty() || (element->isHTMLElement() ? equalIgnoringCase(type, "text/css") : (type == "text/css"));
}

StyleElement::StyleElement(Document* document, bool createdByParser)
    : m_createdByParser(createdByParser)
    , m_loading(false)
    , m_startPosition(TextPosition::belowRangePosition())
{
    if (createdByParser && document && document->scriptableDocumentParser() && !document->isInDocumentWrite())
        m_startPosition = document->scriptableDocumentParser()->textPosition();
}

StyleElement::~StyleElement()
{
    ASSERT(!m_sheet);
    if (m_sheet)
        clearSheet();
}

void StyleElement::insertedIntoDocument(Document* document, Element* element)
{
    ASSERT(document);
    ASSERT(element);
    document->styleSheetCollection()->addStyleSheetCandidateNode(element, m_createdByParser);
    if (m_createdByParser)
        return;

    process(element);
}

void StyleElement::removedFromDocument(Document* document, Element* element)
{
    ASSERT(document);
    ASSERT(element);
    document->styleSheetCollection()->removeStyleSheetCandidateNode(element);

    if (m_sheet)
        clearSheet();

    // If we're in document teardown, then we don't need to do any notification of our sheet's removal.
    if (document->renderer())
        document->styleResolverChanged(DeferRecalcStyle);
}

void StyleElement::clearDocumentData(Document* document, Element* element)
{
    if (m_sheet)
        m_sheet->clearOwnerNode();

    if (element->inDocument())
        document->styleSheetCollection()->removeStyleSheetCandidateNode(element);
}

void StyleElement::childrenChanged(Element* element)
{
    ASSERT(element);
    if (m_createdByParser)
        return;

    process(element);
}

void StyleElement::finishParsingChildren(Element* element)
{
    ASSERT(element);
    process(element);
    m_createdByParser = false;
}

void StyleElement::process(Element* e)
{
    if (!e || !e->inDocument())
        return;

    unsigned resultLength = 0;
    for (Node* c = e->firstChild(); c; c = c->nextSibling()) {
        if (isValidStyleChild(c)) {
            unsigned length = c->nodeValue().length();
            if (length > std::numeric_limits<unsigned>::max() - resultLength) {
                createSheet(e, "");
                return;
            }
            resultLength += length;
        }
    }
    StringBuilder sheetText;
    sheetText.reserveCapacity(resultLength);

    for (Node* c = e->firstChild(); c; c = c->nextSibling()) {
        if (isValidStyleChild(c)) {
            sheetText.append(c->nodeValue());
        }
    }
    ASSERT(sheetText.length() == resultLength);

    createSheet(e, sheetText.toString());
}

void StyleElement::clearSheet()
{
    ASSERT(m_sheet);
    m_sheet.release()->clearOwnerNode();
}

void StyleElement::createSheet(Element* e, const String& text)
{
    ASSERT(e);
    ASSERT(e->inDocument());
    Document* document = e->document();
    if (m_sheet) {
        if (m_sheet->isLoading())
            document->styleSheetCollection()->removePendingSheet();
        clearSheet();
    }

    // If type is empty or CSS, this is a CSS style sheet.
    const AtomicString& type = this->type();
    if (document->contentSecurityPolicy()->allowInlineStyle(e->document()->url(), m_startPosition.m_line) && isCSS(e, type)) {
        RefPtr<MediaQuerySet> mediaQueries;
        if (e->isHTMLElement())
            mediaQueries = MediaQuerySet::createAllowingDescriptionSyntax(media());
        else
            mediaQueries = MediaQuerySet::create(media());

        MediaQueryEvaluator screenEval("screen", true);
        MediaQueryEvaluator printEval("print", true);
        if (screenEval.eval(mediaQueries.get()) || printEval.eval(mediaQueries.get())) {
            document->styleSheetCollection()->addPendingSheet();
            m_loading = true;

            TextPosition startPosition = m_startPosition == TextPosition::belowRangePosition() ? TextPosition::minimumPosition() : m_startPosition;
            m_sheet = CSSStyleSheet::createInline(e, KURL(), startPosition, document->inputEncoding());
            m_sheet->setMediaQueries(mediaQueries.release());
            m_sheet->setTitle(e->title());
            m_sheet->contents()->parseStringAtLine(text, startPosition.m_line.zeroBasedInt(), m_createdByParser);

            m_loading = false;
        }
    }

    if (m_sheet)
        m_sheet->contents()->checkLoaded();
}

bool StyleElement::isLoading() const
{
    if (m_loading)
        return true;
    return m_sheet ? m_sheet->isLoading() : false;
}

bool StyleElement::sheetLoaded(Document* document)
{
    ASSERT(document);
    if (isLoading())
        return false;

    document->styleSheetCollection()->removePendingSheet();
    return true;
}

void StyleElement::startLoadingDynamicSheet(Document* document)
{
    ASSERT(document);
    document->styleSheetCollection()->addPendingSheet();
}

}
