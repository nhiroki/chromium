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

#ifndef SVGPathByteStreamSource_h
#define SVGPathByteStreamSource_h

#include "core/svg/SVGPathByteStream.h"
#include "core/svg/SVGPathSource.h"
#include "platform/geometry/FloatPoint.h"
#include "wtf/PassOwnPtr.h"

namespace WebCore {

class SVGPathByteStreamSource FINAL : public SVGPathSource {
public:
    static PassOwnPtr<SVGPathByteStreamSource> create(SVGPathByteStream* stream)
    {
        return adoptPtr(new SVGPathByteStreamSource(stream));
    }

private:
    SVGPathByteStreamSource(SVGPathByteStream*);

    virtual bool hasMoreData() const OVERRIDE;
    virtual bool moveToNextToken() OVERRIDE { return true; }
    virtual bool parseSVGSegmentType(SVGPathSegType&) OVERRIDE;
    virtual SVGPathSegType nextCommand(SVGPathSegType) OVERRIDE;

    virtual bool parseMoveToSegment(FloatPoint&) OVERRIDE;
    virtual bool parseLineToSegment(FloatPoint&) OVERRIDE;
    virtual bool parseLineToHorizontalSegment(float&) OVERRIDE;
    virtual bool parseLineToVerticalSegment(float&) OVERRIDE;
    virtual bool parseCurveToCubicSegment(FloatPoint&, FloatPoint&, FloatPoint&) OVERRIDE;
    virtual bool parseCurveToCubicSmoothSegment(FloatPoint&, FloatPoint&) OVERRIDE;
    virtual bool parseCurveToQuadraticSegment(FloatPoint&, FloatPoint&) OVERRIDE;
    virtual bool parseCurveToQuadraticSmoothSegment(FloatPoint&) OVERRIDE;
    virtual bool parseArcToSegment(float&, float&, float&, bool&, bool&, FloatPoint&) OVERRIDE;

#if COMPILER(MSVC)
#pragma warning(disable: 4701)
#endif
    template<typename DataType, typename ByteType>
    DataType readType()
    {
        ByteType data;
        size_t typeSize = sizeof(ByteType);

        for (size_t i = 0; i < typeSize; ++i) {
            ASSERT(m_streamCurrent < m_streamEnd);
            data.bytes[i] = *m_streamCurrent;
            ++m_streamCurrent;
        }

        return data.value;
    }

    bool readFlag()
    {
        return readType<bool, BoolByte>();
    }

    float readFloat()
    {
        return readType<float, FloatByte>();
    }

    unsigned short readSVGSegmentType()
    {
        return readType<unsigned short, UnsignedShortByte>();
    }

    FloatPoint readFloatPoint()
    {
        float x = readType<float, FloatByte>();
        float y = readType<float, FloatByte>();
        return FloatPoint(x, y);
    }

    SVGPathByteStream::DataIterator m_streamCurrent;
    SVGPathByteStream::DataIterator m_streamEnd;
};

} // namespace WebCore

#endif // SVGPathByteStreamSource_h
