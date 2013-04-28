/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "core/platform/graphics/filters/FEDisplacementMap.h"

#include "core/platform/graphics/GraphicsContext.h"
#include "core/platform/graphics/filters/Filter.h"
#include "core/platform/text/TextStream.h"
#include "core/rendering/RenderTreeAsText.h"

#include <wtf/Uint8ClampedArray.h>

#include "SkBitmapSource.h"
#include "SkDisplacementMapEffect.h"
#include "core/platform/graphics/filters/SkiaImageFilterBuilder.h"
#include "core/platform/graphics/skia/NativeImageSkia.h"

namespace WebCore {

FEDisplacementMap::FEDisplacementMap(Filter* filter, ChannelSelectorType xChannelSelector, ChannelSelectorType yChannelSelector, float scale)
    : FilterEffect(filter)
    , m_xChannelSelector(xChannelSelector)
    , m_yChannelSelector(yChannelSelector)
    , m_scale(scale)
{
}

PassRefPtr<FEDisplacementMap> FEDisplacementMap::create(Filter* filter, ChannelSelectorType xChannelSelector,
    ChannelSelectorType yChannelSelector, float scale)
{
    return adoptRef(new FEDisplacementMap(filter, xChannelSelector, yChannelSelector, scale));
}

ChannelSelectorType FEDisplacementMap::xChannelSelector() const
{
    return m_xChannelSelector;
}

bool FEDisplacementMap::setXChannelSelector(const ChannelSelectorType xChannelSelector)
{
    if (m_xChannelSelector == xChannelSelector)
        return false;
    m_xChannelSelector = xChannelSelector;
    return true;
}

ChannelSelectorType FEDisplacementMap::yChannelSelector() const
{
    return m_yChannelSelector;
}

bool FEDisplacementMap::setYChannelSelector(const ChannelSelectorType yChannelSelector)
{
    if (m_yChannelSelector == yChannelSelector)
        return false;
    m_yChannelSelector = yChannelSelector;
    return true;
}

float FEDisplacementMap::scale() const
{
    return m_scale;
}

bool FEDisplacementMap::setScale(float scale)
{
    if (m_scale == scale)
        return false;
    m_scale = scale;
    return true;
}

void FEDisplacementMap::setResultColorSpace(ColorSpace)
{
    // Spec: The 'color-interpolation-filters' property only applies to the 'in2' source image
    // and does not apply to the 'in' source image. The 'in' source image must remain in its
    // current color space.
    // The result is in that smae color space because it is a displacement of the 'in' image.
    FilterEffect::setResultColorSpace(inputEffect(0)->resultColorSpace());
}

void FEDisplacementMap::transformResultColorSpace(FilterEffect* in, const int index)
{
    // Do not transform the first primitive input, as per the spec.
    if (index)
        in->transformResultColorSpace(operatingColorSpace());
}

void FEDisplacementMap::applySoftware()
{
    FilterEffect* in = inputEffect(0);
    FilterEffect* in2 = inputEffect(1);

    ASSERT(m_xChannelSelector != CHANNEL_UNKNOWN);
    ASSERT(m_yChannelSelector != CHANNEL_UNKNOWN);

    Uint8ClampedArray* dstPixelArray = createPremultipliedImageResult();
    if (!dstPixelArray)
        return;

    IntRect effectADrawingRect = requestedRegionOfInputImageData(in->absolutePaintRect());
    RefPtr<Uint8ClampedArray> srcPixelArrayA = in->asPremultipliedImage(effectADrawingRect);

    IntRect effectBDrawingRect = requestedRegionOfInputImageData(in2->absolutePaintRect());
    RefPtr<Uint8ClampedArray> srcPixelArrayB = in2->asUnmultipliedImage(effectBDrawingRect);

    ASSERT(srcPixelArrayA->length() == srcPixelArrayB->length());

    Filter* filter = this->filter();
    IntSize paintSize = absolutePaintRect().size();
    float scaleX = filter->applyHorizontalScale(m_scale);
    float scaleY = filter->applyVerticalScale(m_scale);
    float scaleForColorX = scaleX / 255.0;
    float scaleForColorY = scaleY / 255.0;
    float scaledOffsetX = 0.5 - scaleX * 0.5;
    float scaledOffsetY = 0.5 - scaleY * 0.5;
    int stride = paintSize.width() * 4;
    for (int y = 0; y < paintSize.height(); ++y) {
        int line = y * stride;
        for (int x = 0; x < paintSize.width(); ++x) {
            int dstIndex = line + x * 4;
            int srcX = x + static_cast<int>(scaleForColorX * srcPixelArrayB->item(dstIndex + m_xChannelSelector - 1) + scaledOffsetX);
            int srcY = y + static_cast<int>(scaleForColorY * srcPixelArrayB->item(dstIndex + m_yChannelSelector - 1) + scaledOffsetY);
            for (unsigned channel = 0; channel < 4; ++channel) {
                if (srcX < 0 || srcX >= paintSize.width() || srcY < 0 || srcY >= paintSize.height())
                    dstPixelArray->set(dstIndex + channel, static_cast<unsigned char>(0));
                else {
                    unsigned char pixelValue = srcPixelArrayA->item(srcY * stride + srcX * 4 + channel);
                    dstPixelArray->set(dstIndex + channel, pixelValue);
                }
            }
        }
    }
}

static SkDisplacementMapEffect::ChannelSelectorType toSkiaMode(ChannelSelectorType type)
{
    switch (type) {
    case CHANNEL_R:
        return SkDisplacementMapEffect::kR_ChannelSelectorType;
    case CHANNEL_G:
        return SkDisplacementMapEffect::kG_ChannelSelectorType;
    case CHANNEL_B:
        return SkDisplacementMapEffect::kB_ChannelSelectorType;
    case CHANNEL_A:
        return SkDisplacementMapEffect::kA_ChannelSelectorType;
    case CHANNEL_UNKNOWN:
    default:
        return SkDisplacementMapEffect::kUnknown_ChannelSelectorType;
    }
}

bool FEDisplacementMap::applySkia()
{
    // For now, only use the skia implementation for accelerated rendering.
    if (filter()->renderingMode() != Accelerated)
        return false;

    FilterEffect* in = inputEffect(0);
    FilterEffect* in2 = inputEffect(1);

    if (!in || !in2)
        return false;

    ImageBuffer* resultImage = createImageBufferResult();
    if (!resultImage)
        return false;

    RefPtr<Image> color = in->asImageBuffer()->copyImage(DontCopyBackingStore);
    RefPtr<Image> displ = in2->asImageBuffer()->copyImage(DontCopyBackingStore);

    RefPtr<NativeImageSkia> colorNativeImage = color->nativeImageForCurrentFrame();
    RefPtr<NativeImageSkia> displNativeImage = displ->nativeImageForCurrentFrame();

    if (!colorNativeImage || !displNativeImage)
        return false;

    SkBitmap colorBitmap = colorNativeImage->bitmap();
    SkBitmap displBitmap = displNativeImage->bitmap();

    SkAutoTUnref<SkImageFilter> colorSource(new SkBitmapSource(colorBitmap));
    SkAutoTUnref<SkImageFilter> displSource(new SkBitmapSource(displBitmap));
    SkDisplacementMapEffect::ChannelSelectorType typeX = toSkiaMode(m_xChannelSelector);
    SkDisplacementMapEffect::ChannelSelectorType typeY = toSkiaMode(m_yChannelSelector);
    SkAutoTUnref<SkImageFilter> displEffect(new SkDisplacementMapEffect(
        typeX, typeY, SkFloatToScalar(m_scale), displSource, colorSource));
    SkPaint paint;
    paint.setImageFilter(displEffect);
    resultImage->context()->platformContext()->drawBitmap(colorBitmap, 0, 0, &paint);
    return true;
}

SkImageFilter* FEDisplacementMap::createImageFilter(SkiaImageFilterBuilder* builder)
{
    SkImageFilter* color = builder->build(inputEffect(0));
    SkImageFilter* displ = builder->build(inputEffect(1));
    SkDisplacementMapEffect::ChannelSelectorType typeX = toSkiaMode(m_xChannelSelector);
    SkDisplacementMapEffect::ChannelSelectorType typeY = toSkiaMode(m_yChannelSelector);
    return new SkDisplacementMapEffect(typeX, typeY, SkFloatToScalar(m_scale), displ, color);
}

static TextStream& operator<<(TextStream& ts, const ChannelSelectorType& type)
{
    switch (type) {
    case CHANNEL_UNKNOWN:
        ts << "UNKNOWN";
        break;
    case CHANNEL_R:
        ts << "RED";
        break;
    case CHANNEL_G:
        ts << "GREEN";
        break;
    case CHANNEL_B:
        ts << "BLUE";
        break;
    case CHANNEL_A:
        ts << "ALPHA";
        break;
    }
    return ts;
}

TextStream& FEDisplacementMap::externalRepresentation(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << "[feDisplacementMap";
    FilterEffect::externalRepresentation(ts);
    ts << " scale=\"" << m_scale << "\" "
       << "xChannelSelector=\"" << m_xChannelSelector << "\" "
       << "yChannelSelector=\"" << m_yChannelSelector << "\"]\n";
    inputEffect(0)->externalRepresentation(ts, indent + 1);
    inputEffect(1)->externalRepresentation(ts, indent + 1);
    return ts;
}

} // namespace WebCore
