/*
    Copyright (C) 2007 Eric Seidel <eric@webkit.org>

    This file is part of the WebKit project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef SVGPathSegList_h
#define SVGPathSegList_h

#if ENABLE(SVG)
#include "SVGList.h"
#include "SVGPathSeg.h"

namespace WebCore {

    class Path;
    class SVGElement;
 
    class SVGPathSegList : public SVGList<RefPtr<SVGPathSeg> > {
    public:
        static PassRefPtr<SVGPathSegList> create(const QualifiedName& attributeName) { return adoptRef(new SVGPathSegList(attributeName)); }
        virtual ~SVGPathSegList();

        unsigned getPathSegAtLength(double, ExceptionCode&);
        Path toPathData();
        
        static PassRefPtr<SVGPathSegList> createAnimated(const SVGPathSegList* fromList, const SVGPathSegList* toList, float progress);
        
    private:
        SVGPathSegList(const QualifiedName&);
    };

    float adjustAnimatedValue(float from, float to, float progress);

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
