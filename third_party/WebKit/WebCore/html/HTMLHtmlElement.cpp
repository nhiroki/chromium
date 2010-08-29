/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Simon Hausmann (hausmann@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2006, 2010 Apple Inc. All rights reserved.
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
#include "HTMLHtmlElement.h"

#include "ApplicationCacheHost.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "HTMLNames.h"

namespace WebCore {

using namespace HTMLNames;

HTMLHtmlElement::HTMLHtmlElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(htmlTag));
}

PassRefPtr<HTMLHtmlElement> HTMLHtmlElement::create(Document* document)
{
    return adoptRef(new HTMLHtmlElement(htmlTag, document));
}

PassRefPtr<HTMLHtmlElement> HTMLHtmlElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLHtmlElement(tagName, document));
}

bool HTMLHtmlElement::isURLAttribute(Attribute* attribute) const
{
    return attribute->name() == manifestAttr;
}

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
void HTMLHtmlElement::insertedIntoDocument()
{
    HTMLElement::insertedIntoDocument();
    
    if (!document()->parsing())
        return;

    if (!document()->frame())
        return;

    DocumentLoader* documentLoader = document()->frame()->loader()->documentLoader();
    if (!documentLoader)
        return;

    // Check the manifest attribute
    // FIXME: Revisit this when we get a clarification from whatwg on how to handle empty
    // manifest attributes. As spec'd, and coded here, the system will initiate an update
    // passing in the document URL as the manifest URL. That's not a good thing.
    const AtomicString& manifest = getAttribute(manifestAttr);
    if (manifest.isNull())
        documentLoader->applicationCacheHost()->selectCacheWithoutManifest();
    else
        documentLoader->applicationCacheHost()->selectCacheWithManifest(document()->completeURL(manifest));
}
#endif

}
