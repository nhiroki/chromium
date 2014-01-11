/*
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

#ifndef SVGVKernElement_h
#define SVGVKernElement_h

#if ENABLE(SVG_FONTS)
#include "SVGNames.h"
#include "core/svg/SVGElement.h"
#include "core/svg/SVGFontElement.h"

namespace WebCore {

class SVGVKernElement FINAL : public SVGElement {
public:
    static PassRefPtr<SVGVKernElement> create(Document&);

    void buildVerticalKerningPair(KerningPairVector&);

private:
    explicit SVGVKernElement(Document&);

    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;
    virtual void removedFrom(ContainerNode*) OVERRIDE;

    virtual bool rendererIsNeeded(const RenderStyle&) OVERRIDE { return false; }
};

DEFINE_NODE_TYPE_CASTS(SVGVKernElement, hasTagName(SVGNames::vkernTag));

} // namespace WebCore

#endif // ENABLE(SVG_FONTS)
#endif
