/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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

#ifndef SVGAElement_h
#define SVGAElement_h

#include "core/svg/SVGAnimatedBoolean.h"
#include "core/svg/SVGGraphicsElement.h"
#include "core/svg/SVGURIReference.h"

namespace WebCore {

class SVGAElement FINAL : public SVGGraphicsElement,
                          public SVGURIReference {
public:
    static PassRefPtr<SVGAElement> create(Document&);

private:
    explicit SVGAElement(Document&);

    virtual bool isValid() const OVERRIDE { return SVGTests::isValid(); }

    virtual String title() const OVERRIDE;
    virtual AtomicString target() const OVERRIDE { return AtomicString(svgTargetCurrentValue()); }

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual void svgAttributeChanged(const QualifiedName&) OVERRIDE;

    virtual RenderObject* createRenderer(RenderStyle*) OVERRIDE;

    virtual void defaultEventHandler(Event*) OVERRIDE;

    virtual bool supportsFocus() const OVERRIDE;
    virtual bool isMouseFocusable() const OVERRIDE;
    virtual bool isKeyboardFocusable() const OVERRIDE;
    virtual bool rendererIsFocusable() const OVERRIDE;
    virtual bool isURLAttribute(const Attribute&) const OVERRIDE;

    virtual bool willRespondToMouseClickEvents() OVERRIDE;

    virtual bool childShouldCreateRenderer(const Node& child) const OVERRIDE;

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGAElement)
        // This declaration used to define a non-virtual "String& target() const" method, that clashes with "virtual String Element::target() const".
        // That's why it has been renamed to "svgTarget", the CodeGenerators take care of calling svgTargetAnimated() instead of targetAnimated(), see CodeGenerator.pm.
        DECLARE_ANIMATED_STRING(SVGTarget, svgTarget)
        DECLARE_ANIMATED_STRING(Href, href)
    END_DECLARE_ANIMATED_PROPERTIES
};

} // namespace WebCore

#endif // SVGAElement_h
