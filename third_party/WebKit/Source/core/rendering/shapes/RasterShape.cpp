/*
 * Copyright (C) 2013 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "core/rendering/shapes/RasterShape.h"

#include "core/rendering/shapes/ShapeInterval.h"
#include "wtf/MathExtras.h"

namespace WebCore {

IntRect RasterShapeIntervals::bounds() const
{
    if (!m_bounds)
        m_bounds = adoptPtr(new IntRect(m_region.bounds()));
    return *m_bounds;
}

void RasterShapeIntervals::addInterval(int y, int x1, int x2)
{
    m_region.unite(Region(IntRect(x1, y, x2 - x1, 1)));
    m_bounds.clear();
}

static inline IntRect alignedRect(IntRect r, int y1, int y2)
{
    return IntRect(r.x(), y1, r.width(), y2 - y1);
}

void RasterShapeIntervals::getIncludedIntervals(int y1, int y2, SegmentList& result) const
{
    ASSERT(y2 >= y1);

    IntRect lineRect(bounds().x(), y1, bounds().width(), y2 - y1);
    Region lineRegion(lineRect);
    lineRegion.intersect(m_region);
    if (lineRegion.isEmpty())
        return;

    Vector<IntRect> lineRects = lineRegion.rects();
    ASSERT(lineRects.size() > 0);

    Region segmentsRegion(lineRect);
    Region intervalsRegion;

    // The loop below uses Regions to compute the intersection of the horizontal
    // shape intervals that fall within the line's box.

    int lineY = lineRects[0].y();
    for (unsigned i = 0; i < lineRects.size(); ++i) {
        if (lineRects[i].y() != lineY) {
            segmentsRegion.intersect(intervalsRegion);
            intervalsRegion = Region();
        }
        intervalsRegion.unite(Region(alignedRect(lineRects[i], y1, y2)));
        lineY = lineRects[i].y();
    }
    if (!intervalsRegion.isEmpty())
        segmentsRegion.intersect(intervalsRegion);

    Vector<IntRect> segmentRects = segmentsRegion.rects();
    for (unsigned i = 0; i < segmentRects.size(); ++i)
        result.append(LineSegment(segmentRects[i].x(), segmentRects[i].maxX()));
}

const RasterShapeIntervals& RasterShape::marginIntervals() const
{
    ASSERT(shapeMargin() >= 0);
    if (!shapeMargin())
        return *m_intervals;

    // FIXME: add support for non-zero margin, see https://code.google.com/p/chromium/issues/detail?id=252737.
    return *m_intervals;
}

const RasterShapeIntervals& RasterShape::paddingIntervals() const
{
    ASSERT(shapePadding() >= 0);
    if (!shapePadding())
        return *m_intervals;

    // FIXME: add support for non-zero padding, see https://code.google.com/p/chromium/issues/detail?id=252737.
    return *m_intervals;
}

void RasterShape::getExcludedIntervals(LayoutUnit, LayoutUnit, SegmentList&) const
{
    // FIXME: this method is only a stub, see https://code.google.com/p/chromium/issues/detail?id=252737.
}

void RasterShape::getIncludedIntervals(LayoutUnit logicalTop, LayoutUnit logicalHeight, SegmentList& result) const
{
    const RasterShapeIntervals& intervals = paddingIntervals();
    if (intervals.isEmpty())
        return;

    float y1 = logicalTop;
    float y2 = logicalTop + logicalHeight;

    if (y1 < intervals.bounds().y() || y2 > intervals.bounds().maxY())
        return;

    intervals.getIncludedIntervals(y1, y2, result);
}

bool RasterShape::firstIncludedIntervalLogicalTop(LayoutUnit minLogicalIntervalTop, const LayoutSize& minLogicalIntervalSize, LayoutUnit& result) const
{
    float minIntervalTop = minLogicalIntervalTop;
    float minIntervalHeight = minLogicalIntervalSize.height();
    float minIntervalWidth = minLogicalIntervalSize.width();

    const RasterShapeIntervals& intervals = paddingIntervals();
    if (intervals.isEmpty() || minIntervalWidth > intervals.bounds().width())
        return false;

    float minY = std::max<float>(intervals.bounds().y(), minIntervalTop);
    float maxY = minY + minIntervalHeight;

    if (maxY > intervals.bounds().maxY())
        return false;

    // FIXME: complete this method, see https://code.google.com/p/chromium/issues/detail?id=252737.

    result = minY;
    return true;
}

} // namespace WebCore
