/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2010 Apple Inc. All rights reserved.
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
#include "HTMLTableCellElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "HTMLNames.h"
#include "HTMLTableElement.h"
#include "RenderTableCell.h"

using std::max;
using std::min;

namespace WebCore {

// Clamp rowspan at 8k to match Firefox.
static const int maxRowspan = 8190;

using namespace HTMLNames;

inline HTMLTableCellElement::HTMLTableCellElement(const QualifiedName& tagName, Document* document)
    : HTMLTablePartElement(tagName, document)
    , m_row(-1)
    , m_col(-1)
    , m_rowSpan(1)
    , m_colSpan(1)
{
}

PassRefPtr<HTMLTableCellElement> HTMLTableCellElement::create(const QualifiedName& tagName, Document* document)
{
    return new HTMLTableCellElement(tagName, document);
}

int HTMLTableCellElement::cellIndex() const
{
    int index = 0;
    for (const Node * node = previousSibling(); node; node = node->previousSibling()) {
        if (node->hasTagName(tdTag) || node->hasTagName(thTag))
            index++;
    }
    
    return index;
}

bool HTMLTableCellElement::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == nowrapAttr) {
        result = eUniversal;
        return false;
    }
    
    if (attrName == widthAttr ||
        attrName == heightAttr) {
        result = eCell; // Because of the quirky behavior of ignoring 0 values, cells are special.
        return false;
    }

    return HTMLTablePartElement::mapToEntry(attrName, result);
}

void HTMLTableCellElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == rowspanAttr) {
        m_rowSpan = max(1, min(attr->value().toInt(), maxRowspan));
        if (renderer() && renderer()->isTableCell())
            toRenderTableCell(renderer())->updateFromElement();
    } else if (attr->name() == colspanAttr) {
        m_colSpan = max(1, attr->value().toInt());
        if (renderer() && renderer()->isTableCell())
            toRenderTableCell(renderer())->updateFromElement();
    } else if (attr->name() == nowrapAttr) {
        // FIXME: What about removing the property when the attribute becomes null?
        if (!attr->isNull())
            addCSSProperty(attr, CSSPropertyWhiteSpace, CSSValueWebkitNowrap);
    } else if (attr->name() == widthAttr) {
        // FIXME: What about removing the property when the attribute becomes empty, zero or negative?
        if (!attr->value().isEmpty()) {
            int widthInt = attr->value().toInt();
            if (widthInt > 0) // width="0" is ignored for compatibility with WinIE.
                addCSSLength(attr, CSSPropertyWidth, attr->value());
        }
    } else if (attr->name() == heightAttr) {
        // FIXME: What about removing the property when the attribute becomes empty, zero or negative?
        if (!attr->value().isEmpty()) {
            int heightInt = attr->value().toInt();
            if (heightInt > 0) // height="0" is ignored for compatibility with WinIE.
                addCSSLength(attr, CSSPropertyHeight, attr->value());
        }
    } else
        HTMLTablePartElement::parseMappedAttribute(attr);
}

// used by table cells to share style decls created by the enclosing table.
void HTMLTableCellElement::additionalAttributeStyleDecls(Vector<CSSMutableStyleDeclaration*>& results)
{
    Node* p = parentNode();
    while (p && !p->hasTagName(tableTag))
        p = p->parentNode();
    if (!p)
        return;
    static_cast<HTMLTableElement*>(p)->addSharedCellDecls(results);
}

bool HTMLTableCellElement::isURLAttribute(Attribute *attr) const
{
    return attr->name() == backgroundAttr;
}

String HTMLTableCellElement::abbr() const
{
    return getAttribute(abbrAttr);
}

void HTMLTableCellElement::setAbbr(const String &value)
{
    setAttribute(abbrAttr, value);
}

String HTMLTableCellElement::align() const
{
    return getAttribute(alignAttr);
}

void HTMLTableCellElement::setAlign(const String &value)
{
    setAttribute(alignAttr, value);
}

String HTMLTableCellElement::axis() const
{
    return getAttribute(axisAttr);
}

void HTMLTableCellElement::setAxis(const String &value)
{
    setAttribute(axisAttr, value);
}

String HTMLTableCellElement::bgColor() const
{
    return getAttribute(bgcolorAttr);
}

void HTMLTableCellElement::setBgColor(const String &value)
{
    setAttribute(bgcolorAttr, value);
}

String HTMLTableCellElement::ch() const
{
    return getAttribute(charAttr);
}

void HTMLTableCellElement::setCh(const String &value)
{
    setAttribute(charAttr, value);
}

String HTMLTableCellElement::chOff() const
{
    return getAttribute(charoffAttr);
}

void HTMLTableCellElement::setChOff(const String &value)
{
    setAttribute(charoffAttr, value);
}

void HTMLTableCellElement::setColSpan(int n)
{
    setAttribute(colspanAttr, String::number(n));
}

String HTMLTableCellElement::headers() const
{
    return getAttribute(headersAttr);
}

void HTMLTableCellElement::setHeaders(const String &value)
{
    setAttribute(headersAttr, value);
}

String HTMLTableCellElement::height() const
{
    return getAttribute(heightAttr);
}

void HTMLTableCellElement::setHeight(const String &value)
{
    setAttribute(heightAttr, value);
}

bool HTMLTableCellElement::noWrap() const
{
    return !getAttribute(nowrapAttr).isNull();
}

void HTMLTableCellElement::setNoWrap(bool b)
{
    setAttribute(nowrapAttr, b ? "" : 0);
}

void HTMLTableCellElement::setRowSpan(int n)
{
    setAttribute(rowspanAttr, String::number(n));
}

String HTMLTableCellElement::scope() const
{
    return getAttribute(scopeAttr);
}

void HTMLTableCellElement::setScope(const String &value)
{
    setAttribute(scopeAttr, value);
}

String HTMLTableCellElement::vAlign() const
{
    return getAttribute(valignAttr);
}

void HTMLTableCellElement::setVAlign(const String &value)
{
    setAttribute(valignAttr, value);
}

String HTMLTableCellElement::width() const
{
    return getAttribute(widthAttr);
}

void HTMLTableCellElement::setWidth(const String &value)
{
    setAttribute(widthAttr, value);
}

void HTMLTableCellElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    HTMLTablePartElement::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, document()->completeURL(getAttribute(backgroundAttr)));
}

}
