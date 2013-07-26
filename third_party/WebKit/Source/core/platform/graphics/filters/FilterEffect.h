/*
 * Copyright (C) 2008 Alex Mathews <possessedpenguinbob@gmail.com>
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

#ifndef FilterEffect_h
#define FilterEffect_h

#include "core/platform/graphics/ColorSpace.h"
#include "core/platform/graphics/FloatRect.h"
#include "core/platform/graphics/IntRect.h"

#include "wtf/PassOwnPtr.h"
#include "wtf/RefCounted.h"
#include "wtf/RefPtr.h"
#include "wtf/Uint8ClampedArray.h"
#include "wtf/Vector.h"

static const float kMaxFilterSize = 5000.0f;

class SkImageFilter;

namespace WebCore {

class Filter;
class FilterEffect;
class ImageBuffer;
class TextStream;

class SkiaImageFilterBuilder;

typedef Vector<RefPtr<FilterEffect> > FilterEffectVector;

enum FilterEffectType {
    FilterEffectTypeUnknown,
    FilterEffectTypeImage,
    FilterEffectTypeTile,
    FilterEffectTypeSourceInput
};

class FilterEffect : public RefCounted<FilterEffect> {
public:
    virtual ~FilterEffect();

    void clearResult();
    void clearResultsRecursive();

    ImageBuffer* asImageBuffer();
    PassRefPtr<Uint8ClampedArray> asUnmultipliedImage(const IntRect&);
    PassRefPtr<Uint8ClampedArray> asPremultipliedImage(const IntRect&);
    void copyUnmultipliedImage(Uint8ClampedArray* destination, const IntRect&);
    void copyPremultipliedImage(Uint8ClampedArray* destination, const IntRect&);

    FilterEffectVector& inputEffects() { return m_inputEffects; }
    FilterEffect* inputEffect(unsigned) const;
    unsigned numberOfEffectInputs() const { return m_inputEffects.size(); }

    inline bool hasResult() const
    {
        // This function needs platform specific checks, if the memory managment is not done by FilterEffect.
        return m_imageBufferResult
            || m_unmultipliedImageResult
            || m_premultipliedImageResult;
    }

    IntRect drawingRegionOfInputImage(const IntRect&) const;
    IntRect requestedRegionOfInputImageData(const IntRect&) const;

    // Solid black image with different alpha values.
    bool isAlphaImage() const { return m_alphaImage; }
    void setIsAlphaImage(bool alphaImage) { m_alphaImage = alphaImage; }

    IntRect absolutePaintRect() const { return m_absolutePaintRect; }
    void setAbsolutePaintRect(const IntRect& absolutePaintRect) { m_absolutePaintRect = absolutePaintRect; }

    FloatRect maxEffectRect() const { return m_maxEffectRect; }
    void setMaxEffectRect(const FloatRect& maxEffectRect) { m_maxEffectRect = maxEffectRect; }

    void apply();

    // Correct any invalid pixels, if necessary, in the result of a filter operation.
    // This method is used to ensure valid pixel values on filter inputs and the final result.
    // Only the arithmetic composite filter ever needs to perform correction.
    virtual void correctFilterResultIfNeeded() { }

    virtual SkImageFilter* createImageFilter(SkiaImageFilterBuilder*) { return 0; }

    virtual void determineAbsolutePaintRect();

    // Mapping a rect forwards determines which which destination pixels a
    // given source rect would affect. Mapping a rect backwards determines
    // which pixels from the source rect would be required to fill a given
    // destination rect. Note that these are not necessarily the inverse of
    // each other. For example, for FEGaussianBlur, they are the same
    // transformation.
    virtual FloatRect mapRect(const FloatRect& rect, bool forward = true) { return rect; }
    FloatRect mapRectRecursive(const FloatRect&);

    // This is a recursive version of a backwards mapRect(), which also takes
    // into account the filter primitive subregion of each effect.
    FloatRect getSourceRect(const FloatRect& destRect, const FloatRect& clipRect);

    virtual FilterEffectType filterEffectType() const { return FilterEffectTypeUnknown; }

    virtual TextStream& externalRepresentation(TextStream&, int indention = 0) const;

    // The following functions are SVG specific and will move to RenderSVGResourceFilterPrimitive.
    // See bug https://bugs.webkit.org/show_bug.cgi?id=45614.
    bool hasX() const { return m_hasX; }
    void setHasX(bool value) { m_hasX = value; }

    bool hasY() const { return m_hasY; }
    void setHasY(bool value) { m_hasY = value; }

    bool hasWidth() const { return m_hasWidth; }
    void setHasWidth(bool value) { m_hasWidth = value; }

    bool hasHeight() const { return m_hasHeight; }
    void setHasHeight(bool value) { m_hasHeight = value; }

    FloatRect filterPrimitiveSubregion() const { return m_filterPrimitiveSubregion; }
    void setFilterPrimitiveSubregion(const FloatRect& filterPrimitiveSubregion) { m_filterPrimitiveSubregion = filterPrimitiveSubregion; }

    FloatRect effectBoundaries() const { return m_effectBoundaries; }
    void setEffectBoundaries(const FloatRect& effectBoundaries) { m_effectBoundaries = effectBoundaries; }

    Filter* filter() { return m_filter; }

    bool clipsToBounds() const { return m_clipsToBounds; }
    void setClipsToBounds(bool value) { m_clipsToBounds = value; }

    ColorSpace operatingColorSpace() const { return m_operatingColorSpace; }
    virtual void setOperatingColorSpace(ColorSpace colorSpace) { m_operatingColorSpace = colorSpace; }
    ColorSpace resultColorSpace() const { return m_resultColorSpace; }
    virtual void setResultColorSpace(ColorSpace colorSpace) { m_resultColorSpace = colorSpace; }

    virtual void transformResultColorSpace(FilterEffect* in, const int) { in->transformResultColorSpace(m_operatingColorSpace); }
    void transformResultColorSpace(ColorSpace);

    FloatRect determineFilterPrimitiveSubregion();

protected:
    FilterEffect(Filter*);

    ImageBuffer* createImageBufferResult();
    Uint8ClampedArray* createUnmultipliedImageResult();
    Uint8ClampedArray* createPremultipliedImageResult();

    // Return true if the filter will only operate correctly on valid RGBA values, with
    // alpha in [0,255] and each color component in [0, alpha].
    virtual bool requiresValidPreMultipliedPixels() { return true; }

    // If a pre-multiplied image, check every pixel for validity and correct if necessary.
    void forceValidPreMultipliedPixels();

private:
    virtual void applySoftware() = 0;
    virtual bool applySkia() { return false; }

    inline void copyImageBytes(Uint8ClampedArray* source, Uint8ClampedArray* destination, const IntRect&);

    OwnPtr<ImageBuffer> m_imageBufferResult;
    RefPtr<Uint8ClampedArray> m_unmultipliedImageResult;
    RefPtr<Uint8ClampedArray> m_premultipliedImageResult;
    FilterEffectVector m_inputEffects;

    bool m_alphaImage;

    IntRect m_absolutePaintRect;

    // The maximum size of a filter primitive. In SVG this is the primitive subregion in absolute coordinate space.
    // The absolute paint rect should never be bigger than m_maxEffectRect.
    FloatRect m_maxEffectRect;
    Filter* m_filter;

    // The following member variables are SVG specific and will move to RenderSVGResourceFilterPrimitive.
    // See bug https://bugs.webkit.org/show_bug.cgi?id=45614.

    // The subregion of a filter primitive according to the SVG Filter specification in local coordinates.
    // This is SVG specific and needs to move to RenderSVGResourceFilterPrimitive.
    FloatRect m_filterPrimitiveSubregion;

    // x, y, width and height of the actual SVGFE*Element. Is needed to determine the subregion of the
    // filter primitive on a later step.
    FloatRect m_effectBoundaries;
    bool m_hasX;
    bool m_hasY;
    bool m_hasWidth;
    bool m_hasHeight;

    // Should the effect clip to its primitive region, or expand to use the combined region of its inputs.
    bool m_clipsToBounds;

    ColorSpace m_operatingColorSpace;
    ColorSpace m_resultColorSpace;
};

} // namespace WebCore

#endif // FilterEffect_h
