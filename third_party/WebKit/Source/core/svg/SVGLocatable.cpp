/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2009 Google, Inc.  All rights reserved.
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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
#include "core/svg/SVGLocatable.h"

#include "SVGNames.h"
#include "bindings/v8/ExceptionState.h"
#include "core/dom/ExceptionCode.h"
#include "core/rendering/RenderObject.h"
#include "core/svg/SVGGraphicsElement.h"

namespace WebCore {

static bool isViewportElement(Node* node)
{
    return (node->hasTagName(SVGNames::svgTag)
        || node->hasTagName(SVGNames::symbolTag)
        || node->hasTagName(SVGNames::foreignObjectTag)
        || node->hasTagName(SVGNames::imageTag));
}

SVGElement* SVGLocatable::nearestViewportElement(const SVGElement* element)
{
    ASSERT(element);
    for (Element* current = element->parentOrShadowHostElement(); current; current = current->parentOrShadowHostElement()) {
        if (isViewportElement(current))
            return toSVGElement(current);
    }

    return 0;
}

SVGElement* SVGLocatable::farthestViewportElement(const SVGElement* element)
{
    ASSERT(element);
    SVGElement* farthest = 0;
    for (Element* current = element->parentOrShadowHostElement(); current; current = current->parentOrShadowHostElement()) {
        if (isViewportElement(current))
            farthest = toSVGElement(current);
    }
    return farthest;
}

SVGRect SVGLocatable::getBBox(SVGElement* element)
{
    ASSERT(element);
    element->document().updateLayoutIgnorePendingStylesheets();

    // FIXME: Eventually we should support getBBox for detached elements.
    if (!element->renderer())
        return SVGRect();

    return element->renderer()->objectBoundingBox();
}

SVGRect SVGLocatable::getStrokeBBox(SVGElement* element)
{
    ASSERT(element);
    element->document().updateLayoutIgnorePendingStylesheets();

    // FIXME: Eventually we should support getStrokeBBox for detached elements.
    if (!element->renderer())
        return SVGRect();

    return element->renderer()->strokeBoundingBox();
}

AffineTransform SVGLocatable::computeCTM(SVGElement* element, CTMScope mode, StyleUpdateStrategy styleUpdateStrategy)
{
    ASSERT(element);
    if (styleUpdateStrategy == AllowStyleUpdate)
        element->document().updateLayoutIgnorePendingStylesheets();

    AffineTransform ctm;

    SVGElement* stopAtElement = mode == NearestViewportScope ? nearestViewportElement(element) : 0;
    for (Element* currentElement = element; currentElement; currentElement = currentElement->parentOrShadowHostElement()) {
        if (!currentElement->isSVGElement())
            break;

        ctm = toSVGElement(currentElement)->localCoordinateSpaceTransform(mode).multiply(ctm);

        // For getCTM() computation, stop at the nearest viewport element
        if (currentElement == stopAtElement)
            break;
    }

    return ctm;
}

AffineTransform SVGLocatable::getTransformToElement(SVGElement* target, ExceptionState& es, StyleUpdateStrategy styleUpdateStrategy)
{
    AffineTransform ctm = getCTM(styleUpdateStrategy);

    if (target && target->isSVGGraphicsElement()) {
        AffineTransform targetCTM = toSVGGraphicsElement(target)->getCTM(styleUpdateStrategy);
        if (!targetCTM.isInvertible()) {
            es.throwUninformativeAndGenericDOMException(InvalidStateError);
            return ctm;
        }
        ctm = targetCTM.inverse() * ctm;
    }

    return ctm;
}

}
