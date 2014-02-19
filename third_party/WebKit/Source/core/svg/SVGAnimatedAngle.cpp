/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "core/svg/SVGAnimatedAngle.h"

#include "SVGNames.h"
#include "core/svg/SVGAngleTearOff.h"
#include "core/svg/SVGMarkerElement.h"

namespace WebCore {

SVGAnimatedAngle::SVGAnimatedAngle(SVGMarkerElement* contextElement)
    : NewSVGAnimatedProperty<SVGAngle>(contextElement, SVGNames::orientAttr, SVGAngle::create())
    , m_orientType(SVGAnimatedEnumeration<SVGMarkerOrientType>::create(contextElement, SVGNames::orientAttr, baseValue()->orientType()))
{
}

SVGAnimatedAngle::~SVGAnimatedAngle()
{
}

void SVGAnimatedAngle::synchronizeAttribute()
{
    ASSERT(needsSynchronizeAttribute());

    AtomicString value;
    if (m_orientType->currentValue()->enumValue() == SVGMarkerOrientAuto)
        value = "auto";
    else
        value = AtomicString(currentValue()->valueAsString());

    contextElement()->setSynchronizedLazyAttribute(attributeName(), value);
}

void SVGAnimatedAngle::animationStarted()
{
    NewSVGAnimatedProperty<SVGAngle>::animationStarted();
    m_orientType->animationStarted();
}

void SVGAnimatedAngle::setAnimatedValue(PassRefPtr<NewSVGPropertyBase> value)
{
    NewSVGAnimatedProperty<SVGAngle>::setAnimatedValue(value);
    m_orientType->setAnimatedValue(currentValue()->orientType());
}

void SVGAnimatedAngle::animationEnded()
{
    NewSVGAnimatedProperty<SVGAngle>::animationEnded();
    m_orientType->animationEnded();
}

}
