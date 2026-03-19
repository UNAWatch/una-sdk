/******************************************************************************
* Copyright (c) 2018(-2025) STMicroelectronics.
* All rights reserved.
*
* This file is part of the TouchGFX 4.26.0 distribution.
*
* This software is licensed under terms that can be found in the LICENSE file in
* the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
*******************************************************************************/

#include <touchgfx/Drawable.hpp>
#include <touchgfx/Utils.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/lcd/LCD.hpp>
#include <touchgfx/widgets/ScalableImage.hpp>

namespace touchgfx
{
ScalableImage::ScalableImage(const Bitmap& bmp /*= Bitmap() */)
    : Image(bmp),
      currentScalingAlgorithm(BILINEAR_INTERPOLATION)
{
}

void ScalableImage::setScalingAlgorithm(ScalingAlgorithm algorithm)
{
    currentScalingAlgorithm = algorithm;
}

ScalableImage::ScalingAlgorithm ScalableImage::getScalingAlgorithm()
{
    return currentScalingAlgorithm;
}

RenderingVariant ScalableImage::lookupRenderVariant() const
{
    RenderingVariant renderVariant;
    if (currentScalingAlgorithm == NEAREST_NEIGHBOR)
    {
        renderVariant = lookupNearestNeighborRenderVariant(bitmap);
    }
    else
    {
        renderVariant = lookupBilinearRenderVariant(bitmap);
    }
    return renderVariant;
}

void ScalableImage::draw(const Rect& invalidatedArea) const
{
    if (!alpha)
    {
        return;
    }
    Rect meAbs = getAbsoluteRect();
    HAL::lcd().drawScaledBitmap(bitmap, meAbs, invalidatedArea, lookupRenderVariant(), alpha);
}

Rect ScalableImage::getSolidRect() const
{
    // If original image is completely solid the scaled image will also be
    if (alpha == 255)
    {
        const Rect bmpSolid = bitmap.getSolidRect();
        if (bmpSolid.width == bitmap.getWidth() && bmpSolid.height == bitmap.getHeight())
        {
            const Rect widgetRect(0, 0, getWidth(), getHeight());
            // NOTE: TextureMapper may not fill solid pixels in the whole widget due to antialiasing,
            // so the intersect is needed as of now until TextureMapper is fixed.
            return bmpSolid & widgetRect;
        }
    }

    // Return empty rect otherwise
    return Rect();
}
} // namespace touchgfx
