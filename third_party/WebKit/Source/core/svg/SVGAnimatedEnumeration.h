/*
 * Copyright (C) Research In Motion Limited 2010, 2012. All rights reserved.
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

#ifndef SVGAnimatedEnumeration_h
#define SVGAnimatedEnumeration_h

#include "core/svg/SVGAnimatedTypeAnimator.h"
#include "core/svg/properties/SVGAnimatedEnumerationPropertyTearOff.h"
#include "core/svg/properties/SVGAnimatedPropertyMacros.h"

namespace WebCore {

typedef SVGAnimatedStaticPropertyTearOff<unsigned> SVGAnimatedEnumeration;

// Helper macros to declare/define a SVGAnimatedEnumeration object
#define DECLARE_ANIMATED_ENUMERATION(UpperProperty, LowerProperty, EnumType) \
DECLARE_ANIMATED_PROPERTY(SVGAnimatedEnumerationPropertyTearOff<EnumType>, EnumType, UpperProperty, LowerProperty)

#define DEFINE_ANIMATED_ENUMERATION(OwnerType, DOMAttribute, UpperProperty, LowerProperty, EnumType) \
DEFINE_ANIMATED_PROPERTY(AnimatedEnumeration, OwnerType, DOMAttribute, DOMAttribute.localName(), UpperProperty, LowerProperty, SVGAnimatedEnumerationPropertyTearOff<EnumType>, EnumType)

class SVGAnimatedEnumerationAnimator FINAL : public SVGAnimatedTypeAnimator {
public:
    SVGAnimatedEnumerationAnimator(SVGAnimationElement*, SVGElement*);
    virtual ~SVGAnimatedEnumerationAnimator() { }

    virtual PassOwnPtr<SVGAnimatedType> constructFromString(const String&) OVERRIDE;
    virtual PassOwnPtr<SVGAnimatedType> startAnimValAnimation(const SVGElementAnimatedPropertyList&) OVERRIDE;
    virtual void stopAnimValAnimation(const SVGElementAnimatedPropertyList&) OVERRIDE;
    virtual void resetAnimValToBaseVal(const SVGElementAnimatedPropertyList&, SVGAnimatedType*) OVERRIDE;
    virtual void animValWillChange(const SVGElementAnimatedPropertyList&) OVERRIDE;
    virtual void animValDidChange(const SVGElementAnimatedPropertyList&) OVERRIDE;

    virtual void addAnimatedTypes(SVGAnimatedType*, SVGAnimatedType*) OVERRIDE;
    virtual void calculateAnimatedValue(float percentage, unsigned repeatCount, SVGAnimatedType*, SVGAnimatedType*, SVGAnimatedType*, SVGAnimatedType*) OVERRIDE;
    virtual float calculateDistance(const String& fromString, const String& toString) OVERRIDE;
};

} // namespace WebCore

#endif
