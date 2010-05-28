/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2003, 2006, 2010 Apple Inc. All rights reserved.
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
#include "HTMLQuoteElement.h"

#include "Document.h"
#include "HTMLNames.h"

namespace WebCore {

using namespace HTMLNames;

inline HTMLQuoteElement::HTMLQuoteElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(qTag));
}

PassRefPtr<HTMLQuoteElement> HTMLQuoteElement::create(const QualifiedName& tagName, Document* document)
{
    return new HTMLQuoteElement(tagName, document);
}

void HTMLQuoteElement::insertedIntoDocument()
{
    document()->setUsesBeforeAfterRules(true);

    HTMLElement::insertedIntoDocument();
}

String HTMLQuoteElement::cite() const
{
    return getAttribute(citeAttr);
}

void HTMLQuoteElement::setCite(const String &value)
{
    setAttribute(citeAttr, value);
}

}
