/******************************************************************************
* Copyright (c) 2018(-2025) STMicroelectronics.
* All rights reserved.
*
* This file is part of the TouchGFX 4.25.0 distribution.
*
* This software is licensed under terms that can be found in the LICENSE file in
* the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
*******************************************************************************/

/**
 * @file touchgfx/widgets/canvas/AbstractPainterLinearGradient.hpp
 *
 * Declares the touchgfx::AbstractPainterLinearGradient class.
 */
#ifndef TOUCHGFX_ABSTRACTPAINTERLINEARGRADIENT_HPP
#define TOUCHGFX_ABSTRACTPAINTERLINEARGRADIENT_HPP

#include <touchgfx/Matrix3x3.hpp>
#include <touchgfx/hal/Types.hpp>
#include <touchgfx/widgets/canvas/AbstractPainter.hpp>

namespace touchgfx
{
/**
 * An abstract class for creating painter classes for drawing canvas widgets. All canvas widgets
 * need a painter to fill the shape drawn with a CanvasWidgetRenderer. The painter must provide the
 * color of a pixel on a given coordinate, which will the be blended into the framebuffer depending
 * on the position of the canvas widget and the transparency of the given pixel.
 */
class AbstractPainterLinearGradient : public AbstractPainter
{
public:
    /** Constructor. */
    AbstractPainterLinearGradient()
        : AbstractPainter(),
          coord0(0), coord1(100), texture(0), isSolid(false), isVertical(false), isHorizontal(false), clSlope(0.0f), clOffset(0.0f), horizontalDistance(0.0f), deltaColor(0.0f)
    {
    }

    /** Destructor */
    virtual ~AbstractPainterLinearGradient()
    {
    }

    /**
     * Set the gradient line. First coordinate (startX, startY)
     * specifies the position of the first color. Last coordinate (endX, endY) specifies the
     * position of the last color.
     *
     * @param  startX X coordinate of the gradient line start point.
     * @param  startY Y coordinate of the gradient line start point.
     * @param  endX   X coordinate of the gradient line end point.
     * @param  endY   Y coordinate of the gradient line end point.
     * @param  width  Width of gradient.
     * @param  height Height of gradient.
     * @param  m      Transformation.
     */
    void setGradientEndPoints(float startX, float startY, float endX, float endY, float width, float height, const Matrix3x3& m)
    {
        Matrix3x3::Point start = m.affineTransform(startX, startY);
        Matrix3x3::Point end = m.affineTransform(endX, endY);

        isVertical = (start.x == end.x);
        isHorizontal = (start.y == end.y);

        if (!isVertical && !isHorizontal)
        {
            // Calculate slope and position of first color line and horizontal distance to last
            // Note: startX, startY, endX, endY are given in bounding box coordinate system [0;width]x[0;height]
            // Color lines are perpendicular to gradient vector (cvy, cvx), in the square bounding-box coordinate system [0;1]x[0;1].
            const float cvx = endX - startX;
            const float cvy = endY - startY;

            // So we scale gradient back from width x height
            const float cvxbb = cvx / width;
            const float cvybb = cvy / height;

            // Equal Colors vector is orthogonal to gradient vector, but scale to real size of shape
            const float colorlinedx = -cvybb * width;
            const float colorlinedy = cvxbb * height;

            // Rotate color vector according to transformation
            Matrix3x3::Point transformed = m.affineTransform(startX + colorlinedx, startY + colorlinedy);

            // Translate back to startX,Y
            float transformedlinedx = transformed.x - start.x;
            float transformedlinedy = transformed.y - start.y;

            if (HAL::DISPLAY_ROTATION == rotate90)
            {
                float temp = transformedlinedx;
                transformedlinedx = -transformedlinedy;
                transformedlinedy = temp;
            }

            // If the color line has dx==0, the gradient (not the color line) is horizontal
            if (transformedlinedx == 0.0f)
            {
                isHorizontal = true;
            }
            else
            {
                clSlope = transformedlinedy / transformedlinedx;

                // If the slope of the colorline is zero, the gradient is vertical
                if (clSlope == 0.0f)
                {
                    isVertical = true;
                }
                else
                {
                    // First color line intersects (x0, coord0)
                    if (HAL::DISPLAY_ROTATION == rotate90)
                    {
                        float sy = start.y;
                        float sx = start.x;
                        //rotate
                        float temp = sy;
                        sy = widgetWidth - sx;
                        sx = temp;
                        clOffset = sy - sx * clSlope;
                    }
                    else
                    {
                        clOffset = start.y - start.x * clSlope;
                    }

                    // Pixel distance between first and last color line horizontally
                    float transformed_cvx = end.x - start.x;
                    float transformed_cvy = end.y - start.y;
                    if (HAL::DISPLAY_ROTATION == rotate90)
                    {
                        float temp = transformed_cvy;
                        transformed_cvy = -transformed_cvx;
                        transformed_cvx = temp;
                    }
                    horizontalDistance = transformed_cvx - transformed_cvy / clSlope;

                    // Color increment when moving 1 pixel horizontally
                    deltaColor = 1024.0f / horizontalDistance;

                    if (horizontalDistance < 0)
                    {
                        horizontalDistance = -horizontalDistance;
                    }
                    return;
                }
            }
        }

        // Gradient was horizontal or vertical, or is now after transformation
        if (HAL::DISPLAY_ROTATION == rotate90)
        {
            if (isVertical)
            {
                // Vertical gradient becomes horizontal in rotate90
                // Using the display y values as x in framebuffer!

                // Color change over one line
                deltaColor = 1023.9999f / (end.y - start.y);

                // Save x-coordinates for the horizontal gradient.
                // With x0 lowest x coordinate, x1 highest
                if (start.y < end.y)
                {
                    coord0 = static_cast<int16_t>(start.y);
                    coord1 = static_cast<int16_t>(end.y);
                }
                else
                {
                    coord0 = static_cast<int16_t>(end.y);
                    coord1 = static_cast<int16_t>(start.y);
                }
                horizontalDistance = static_cast<float>(coord1 - coord0);
                isVertical = false;
                isHorizontal = true;
                return;
            }
            if (isHorizontal)
            {
                // Horizontal gradient becomes vertical in rotate90
                // Using the display x as y in framebuffer

                const float endx = widgetWidth - end.x;
                const float startx = widgetWidth - start.x;
                // Color change over one line
                deltaColor = 1023.9999f / (endx - startx);

                // Save y-coordinates for the vertical gradient.
                // With coord0 lowest y coordinate, coord1 highest
                if (startx < endx)
                {
                    coord0 = static_cast<int16_t>(startx);
                    coord1 = static_cast<int16_t>(endx);
                }
                else
                {
                    coord0 = static_cast<int16_t>(endx);
                    coord1 = static_cast<int16_t>(startx);
                }
                isVertical = true;
                isHorizontal = false;
                return;
            }
        }
        else
        {
            if (isVertical)
            {
                // Color change over one line
                deltaColor = 1023.9999f / (end.y - start.y);

                // Save y-coordinates for the vertical gradient.
                // With coord0 lowest y coordinate, coord1 highest
                if (start.y < end.y)
                {
                    coord0 = static_cast<int16_t>(start.y);
                    coord1 = static_cast<int16_t>(end.y);
                }
                else
                {
                    coord0 = static_cast<int16_t>(end.y);
                    coord1 = static_cast<int16_t>(start.y);
                }
            }
            else if (isHorizontal)
            {
                // Color change over one line
                deltaColor = 1023.9999f / (end.x - start.x);

                // Save x-coordinates for the horizontal gradient.
                // With x0 lowest x coordinate, x1 highest
                if (start.x < end.x)
                {
                    coord0 = static_cast<int16_t>(start.x);
                    coord1 = static_cast<int16_t>(end.x);
                }
                else
                {
                    coord0 = static_cast<int16_t>(end.x);
                    coord1 = static_cast<int16_t>(start.x);
                }
                horizontalDistance = static_cast<float>(coord1 - coord0);
            }
        }
    }

    /**
     * Set the 1024x1 texture used to color this texture. The texture is mapped to the gradient line
     * (startX, startY) to (endX, endY).
     *
     * The texture must be in ARGB8888 format.
     *
     * @param  tex   The gradient texture.
     * @param  solid True if all colors in the gradient are solid.
     */
    void setGradientTexture(const uint32_t* tex, bool solid)
    {
        texture = tex;
        isSolid = solid;
    }

    /**
     * Set the width of the widget using this gradient painter. Used in rotate90 calculations.
     *
     * @param w Width of the widget in pixels.
     */
    void setWidgetWidth(int16_t w)
    {
        widgetWidth = w;
    }

protected:
    int16_t widgetWidth;      ///< Width of widget, used in rotate90
    int16_t coord0;           ///< The gradient line start.
    int16_t coord1;           ///< The gradient line end.
    const uint32_t* texture;  ///< The gradient color texture 1 x 1024.
    bool isSolid;             ///< True if all the gradient colors are solid.
    bool isVertical;          ///< True if the gradient is vertical (x0 == x1).
    bool isHorizontal;        ///< True if the gradient is horizontal (y0 == y1).
    float clSlope;            ///< Slope of lines with same color
    float clOffset;           ///< Vertical offset for line with first color
    float horizontalDistance; ///< Horizontal distance between pixels with the first color and the last color
    float deltaColor;         ///< Color increment when moving 1 pixel horizontally
};

} // namespace touchgfx

#endif // TOUCHGFX_ABSTRACTPAINTERLINEARGRADIENT_HPP
