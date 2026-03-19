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

/**
 * @file platform/driver/lcd/LCDDMA2D3_CMDList.hpp
 *
 * Declares the touchgfx::LCDDMA2D3_CMDList class.
 */
#ifndef TOUCHGFX_LCDDMA2D3_CMDLIST_HPP
#define TOUCHGFX_LCDDMA2D3_CMDLIST_HPP

#include <platform/driver/lcd/LCD16bpp.hpp>
#include <platform/driver/lcd/LCD24bpp.hpp>
#include <platform/driver/lcd/LCD32bpp.hpp>
#include <touchgfx/Bitmap.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/hal/Types.hpp>
#include <touchgfx/transforms/DisplayTransformation.hpp>

namespace touchgfx
{
struct GlyphNode;

namespace DMA2DV3
{
enum DMA2DV3_ColorMode
{
    COLOR_MODE_ARGB8888 = 0x0,
    COLOR_MODE_RGB888 = 0x1,
    COLOR_MODE_RGB565 = 0x2,
    COLOR_MODE_A8 = 0x9,
    COLOR_MODE_A4 = 0xA,
    COLOR_MODE_A2 = 0xC,
    COLOR_MODE_A1 = 0xD,
};

/** Struct giving the format of an L8 image header. */
typedef struct
{
    const uint8_t format;      ///< The format of the color palette (565/888/8888).
    const uint8_t compression; ///< The compression used for the image.
    const uint16_t size;       ///< The number of colors in the palette.
    const uint32_t data[1];    ///< First pixels.
} clutData_t;

void addFillRectCmdList(uint8_t* const wbuf, uint32_t stride, uint32_t dst_format, uint32_t width, uint32_t height, colortype color, uint8_t alpha);
void addBlitCopyRGB565CmdList(uint8_t* const wbuf, uint32_t stride, uint32_t dst_format, const uint8_t* const sourceData, uint32_t sourceStride, const Rect& blitRect, uint8_t alpha);
void addBlitCopyRGB888CmdList(uint8_t* const wbuf, uint32_t stride, uint32_t dst_format, const uint8_t* const sourceData, uint32_t sourceStride, const Rect& blitRect, uint8_t alpha);
void addBlitCopyARGB8888CmdList(uint8_t* const wbuf, uint32_t stride, uint32_t dst_format, const uint8_t* const sourceData, uint32_t sourceStride, const Rect& blitRect, uint8_t alpha);
void addBlitFitRGB565CmdList(uint8_t* const wbuf, uint32_t stride, uint32_t dst_format, const uint8_t* const sourceData, const Rect& blitRect, uint32_t sourceWidth, uint32_t sourceHeight, const Rect& invalidatedArea, uint8_t alpha);
void addBlitFitRGB888CmdList(uint8_t* const wbuf, uint32_t stride, uint32_t dst_format, const uint8_t* const sourceData, const Rect& blitRect, uint32_t sourceWidth, uint32_t sourceHeight, const Rect& invalidatedArea, uint8_t alpha);
void addBlitFitARGB8888CmdList(uint8_t* const wbuf, uint32_t stride, uint32_t dst_format, const uint8_t* const sourceData, const Rect& blitRect, uint32_t sourceWidth, uint32_t sourceHeight, const Rect& invalidatedArea, uint8_t alpha);
void addBlitCopyL8CmdList(uint8_t* const wbuf, uint32_t stride, uint32_t dst_format, const uint8_t* const sourceData, uint32_t sourceStride, const Rect& blitRect, Bitmap::ClutFormat clutFormat, uint8_t alpha);
void addBlitFitL8CmdList(uint8_t* const wbuf, uint32_t stride, uint32_t dst_format, const uint8_t* const sourceData, const Rect& blitRect, uint32_t sourceWidth, uint32_t sourceHeight, const Rect& invalidatedArea, Bitmap::ClutFormat clutFormat, uint8_t alpha);
void addBlitCopyGlyphCmdList(uint8_t* const wbuf, uint32_t stride, uint32_t dst_format, const uint8_t* const sourceData, uint32_t sourceFormat, uint32_t sourceStride, uint32_t width, uint32_t height, int preSkip, int trailSkip, colortype color, uint8_t alpha);
void addBlitCopyGlyphCmdListRotatedDestination(uint8_t* const wbuf, uint32_t stride, uint32_t dst_format, const uint8_t* const sourceData, uint32_t sourceFormat, uint32_t glyphStride, uint32_t glyphWidth, uint32_t glyphHeight, int preSkip, int trailSkip, colortype color, uint8_t alpha);
void addLoadCLUTCmdList(const uint32_t* const clut, uint32_t entries, Bitmap::ClutFormat clutFormat);
} //namespace DMA2DV3

/**
 * This class contains the various low-level drawing routines for drawing bitmaps, texts and
 * rectangles in a 16/24/32 bit framebuffer using DMA2DV3 command lists.
 *
 * The LCD_TYPE template argument is the corresponding "normal" LCD class name:
 * LCD16bpp, LCD24bpp, LCD32bpp.
 * The FORMAT argument is values from DMA2DV3_ColorMode. Must be in the range [0:2].
 * The BYTES_PR_PIXEL is 2/3/4 correspondingly.
 *
 * @see LCD
 *
 * @note All coordinates are expected to be in absolute coordinates!
 */
template <class LCD_TYPE, int FORMAT, int BYTES_PR_PIXEL>
class LCDDMA2D3_CMDList : public LCD_TYPE
{
public:
    /** Initializes members and super class. */
    LCDDMA2D3_CMDList()
        : LCD_TYPE(), clientFrameBuffer(0), clientFrameBufferWidth(0)
    {
    }

    /**
     * Draws a filled rectangle in the framebuffer in the specified color and opacity. By
     * default the rectangle will be drawn as a solid box. The rectangle can be drawn with
     * transparency by specifying alpha from 0=invisible to 255=solid.
     *
     * @param  rect  The rectangle to draw in absolute display coordinates.
     * @param  color The rectangle color.
     * @param  alpha (Optional) The rectangle opacity, from 0=invisible to 255=solid.
     *
     */
    void fillRect(const Rect& rect, colortype color, uint8_t alpha)
    {
        if (alpha == 0 || rect.isEmpty())
        {
            return;
        }

        Rect transformedRect = rect;
        DisplayTransformation::transformDisplayToFrameBuffer(transformedRect);

        uint8_t* wbuf = clientFrameBuffer + BYTES_PR_PIXEL * (transformedRect.x + clientFrameBufferWidth * transformedRect.y);

        if (FORMAT == DMA2DV3::COLOR_MODE_RGB565 && alpha == 255)
        {
            if ((clientFrameBufferWidth & 1) == 0 && (transformedRect.width & 1) == 0 && (reinterpret_cast<uintptr_t>(wbuf) & 3) == 0)
            {
                const uint16_t color16 = ((color >> 8) & 0xF800) | ((color >> 5) & 0x07E0) | ((color >> 3) & 0x001F);
                // Use optimized fill
                color = color16 << 16 | color16;
                DMA2DV3::addFillRectCmdList(wbuf, clientFrameBufferWidth / 2, DMA2DV3::COLOR_MODE_ARGB8888, transformedRect.width / 2, transformedRect.height, color, alpha);

                return;
            }
        }

        DMA2DV3::addFillRectCmdList(wbuf, clientFrameBufferWidth, FORMAT, transformedRect.width, transformedRect.height, color, alpha);
    }

    /**
     * Draws all (or a part) of a \a bitmap. The coordinates of the corner of the bitmap is
     * given in (\a x, \a y) and \a rect describes which part of the \a bitmap should be
     * drawn. The bitmap can be drawn as it is or more or less transparent depending on the
     * value of \a alpha. The value of \a alpha is independent of the transparency of the
     * individual pixels of the given \a bitmap.
     *
     * @param bitmap       The bitmap to draw.
     * @param x            The absolute x coordinate to place (0, 0) of the bitmap on the screen.
     * @param y            The absolute y coordinate to place (0, 0) of the bitmap on the screen.
     * @param rect         A rectangle describing what region of the bitmap is to be drawn.
     * @param alpha        (Optional) Alpha value ranging from 0=invisible to
     *                     255=solid. Default is 255 (solid).
     * @param useOptimized (Optional) If false, do not attempt to substitute (parts of) this
     *                     bitmap with faster fillrects.
     */
    void drawPartialBitmap(const Bitmap& bitmap, int16_t x, int16_t y, const Rect& rect, uint8_t alpha = 255, bool useOptimized = true)
    {
        if (!alpha)
        {
            // nothing to draw - 100% transparent
            return;
        }

        const Rect source(x, y, bitmap.getWidth(), bitmap.getHeight());
        const uint8_t* bitmapData = bitmap.getData();
        switch (bitmap.getFormat())
        {
        case Bitmap::ARGB8888:
            blitCopyARGB8888(bitmapData, source, rect, alpha);
            break;
        case Bitmap::RGB888:
            blitCopyRGB888(bitmapData, source, rect, alpha);
            break;
        case Bitmap::L8:
            blitCopyL8(bitmapData, bitmap.getExtraData(), source, rect, alpha);
            break;
        case Bitmap::RGB565:
            if (bitmap.hasTransparentPixels())
            {
                assert(0 && "LCDDMA2D3_CMDList: RGB565 with transparent pixels are not supported!");
            }
            else
            {
                blitCopyRGB565(bitmapData, source, rect, alpha);
            }
            break;
        case Bitmap::BW:
        case Bitmap::BW_RLE:
        case Bitmap::GRAY2:
        case Bitmap::GRAY4:
        case Bitmap::ABGR2222:
        case Bitmap::ARGB2222:
        case Bitmap::BGRA2222:
        case Bitmap::RGBA2222:
        case Bitmap::A4:
        case Bitmap::CUSTOM:
            assert(false && "LCDDMA2D3_CMDList: Unsupported bitmap format in LCDDMA2DV3::blitCopy!");
            break;
        case Bitmap::COMPRESSED_RGB565:
        case Bitmap::COMPRESSED_RGB888:
        case Bitmap::COMPRESSED_ARGB8888:
            // Call super class for support for compressed images
            LCD_TYPE::drawPartialBitmap(bitmap, x, y, rect, alpha, useOptimized);
            break;
        }
    }

    /**
     * Draws all (or a part) of a \a bitmap, scaled to fit desired rect.
     *
     * @param  bitmap          The bitmap to draw.
     * @param  blitRect        The destination area to draw the bitmap in, in absolute coordinates.
     * @param  invalidatedArea The subarea of the destination area that should be redrawn, in
     *                         relative coordinates to the destination area.
     * @param  renderVariant   The rendering variant to use for the scaling operation.
     * @param  alpha           Optional alpha value ranging from 0=invisible to 255=solid.
     */
    void drawScaledBitmap(const Bitmap& bitmap, const Rect& blitRect, const Rect& invalidatedArea, RenderingVariant renderVariant, uint8_t alpha = 255)
    {
        // Use optimized scaling function in cases where it is faster.
        // Otherwise, use fallback software implementation.
        if (blitRect.width <= bitmap.getWidth() && blitRect.height <= bitmap.getHeight())
        {
            Rect transformedBlitRect = blitRect;
            DisplayTransformation::transformDisplayToFrameBuffer(transformedBlitRect);
            Rect transformedInvalidatedArea = invalidatedArea;
            DisplayTransformation::transformDisplayToFrameBuffer(transformedInvalidatedArea, blitRect);

            const uint32_t srcW = HAL::DISPLAY_ROTATION == rotate90 ? bitmap.getHeight() : bitmap.getWidth();
            const uint32_t srcH = HAL::DISPLAY_ROTATION == rotate90 ? bitmap.getWidth() : bitmap.getHeight();
            const uint16_t* sourceData = (const uint16_t*)bitmap.getData();
            uint8_t* dst = clientFrameBuffer + (transformedInvalidatedArea.x + transformedBlitRect.x + HAL::FRAME_BUFFER_WIDTH * (transformedBlitRect.y + transformedInvalidatedArea.y)) * BYTES_PR_PIXEL;

            switch (renderVariant)
            {
            case (((int)Bitmap::RGB565 << RenderingVariant_FormatShift) | RenderingVariant_NoAlpha | RenderingVariant_NearestNeighbor):
                DMA2DV3::addBlitFitRGB565CmdList((uint8_t*)dst, HAL::FRAME_BUFFER_WIDTH, FORMAT, bitmap.getData(), transformedBlitRect, srcW, srcH, transformedInvalidatedArea, alpha);
                return;
            case (((int)Bitmap::RGB888 << RenderingVariant_FormatShift) | RenderingVariant_NoAlpha | RenderingVariant_NearestNeighbor):
                DMA2DV3::addBlitFitRGB888CmdList((uint8_t*)dst, HAL::FRAME_BUFFER_WIDTH, FORMAT, bitmap.getData(), transformedBlitRect, srcW, srcH, transformedInvalidatedArea, alpha);
                return;
            case (((int)Bitmap::ARGB8888 << RenderingVariant_FormatShift) | RenderingVariant_NoAlpha | RenderingVariant_NearestNeighbor):
            case (((int)Bitmap::ARGB8888 << RenderingVariant_FormatShift) | RenderingVariant_Alpha | RenderingVariant_NearestNeighbor):
                DMA2DV3::addBlitFitARGB8888CmdList((uint8_t*)dst, HAL::FRAME_BUFFER_WIDTH, FORMAT, bitmap.getData(), transformedBlitRect, srcW, srcH, transformedInvalidatedArea, alpha);
                return;
            case (((int)Bitmap::L8 << RenderingVariant_FormatShift) | RenderingVariant_NoAlpha | RenderingVariant_NearestNeighbor):
            case (((int)Bitmap::L8 << RenderingVariant_FormatShift) | RenderingVariant_Alpha | RenderingVariant_NearestNeighbor):
                {
                    const DMA2DV3::clutData_t* const palette = reinterpret_cast<const DMA2DV3::clutData_t*>(bitmap.getExtraData());

                    const Bitmap::Compression compressionAlgorithm = static_cast<Bitmap::Compression>(palette->compression);

                    if (compressionAlgorithm != Bitmap::COMPRESSION_L8_NONE)
                    {
                        break;
                    }

                    // Load palette
                    DMA2DV3::addLoadCLUTCmdList(palette->data, palette->size, static_cast<Bitmap::ClutFormat>(palette->format));

                    // Draw Image
                    DMA2DV3::addBlitFitL8CmdList((uint8_t*)dst, HAL::FRAME_BUFFER_WIDTH, FORMAT, bitmap.getData(), transformedBlitRect, srcW, srcH, transformedInvalidatedArea, static_cast<Bitmap::ClutFormat>(palette->format), alpha);

                    return;
                }
            default:
                break;
            }
        }
        LCD_TYPE::drawScaledBitmap(bitmap, blitRect, invalidatedArea, renderVariant, alpha);
    }

    /**
     * Blits (directly copies) a block of data to the framebuffer, performing alpha blending
     * as specified.
     *
     * @param  sourceData           The source array pointer (points to the beginning of the
     *                              data). The sourceData must be stored in the same format as the framebuffer.
     * @param  source               The position and dimensions of the source. The x and y of this
     *                              rect should both be 0.
     * @param  blitRect             A rectangle describing what region of the \a sourceData is to
     *                              be copied to the framebuffer.
     * @param  alpha                The alpha value to use for blending ranging from 0=invisible
     *                              to 255=solid=no blending.
     */
    void blitCopy(const uint16_t* sourceData, const Rect& source, const Rect& blitRect, uint8_t alpha, bool /* hasTransparentPixels */)
    {
        if (!alpha)
        {
            // nothing to draw - 100% transparent
            return;
        }

        switch (FORMAT)
        {
        case DMA2DV3::COLOR_MODE_ARGB8888:
            blitCopyARGB8888(reinterpret_cast<const uint8_t*>(sourceData), source, blitRect, alpha);
            break;
        case DMA2DV3::COLOR_MODE_RGB888:
            blitCopyRGB888(reinterpret_cast<const uint8_t*>(sourceData), source, blitRect, alpha);
            break;
        case DMA2DV3::COLOR_MODE_RGB565:
            blitCopyRGB565(reinterpret_cast<const uint8_t*>(sourceData), source, blitRect, alpha);
            break;
        }
    }

    /**
     * Blits (directly copies) a block of data to the framebuffer, performing alpha blending
     * as specified.
     *
     * If the display does not support the specified sourceFormat, an assert will be
     * raised.
     *
     * @param  sourceData           The source array pointer (points to the beginning of the
     *                              data). The sourceData must be stored in a format
     *                              suitable for the selected display.
     * @param  sourceFormat         The bitmap format used in the source data.
     * @param  source               The position and dimensions of the source. The x and y of this
     *                              rect should both be 0.
     * @param  blitRect             A rectangle describing what region of the sourceData is to
     *                              be copied to the framebuffer.
     * @param  alpha                The alpha value to use for blending ranging from 0=invisible
     *                              to 255=solid=no blending.
     */
    void blitCopy(const uint8_t* sourceData, Bitmap::BitmapFormat sourceFormat, const Rect& source, const Rect& blitRect, uint8_t alpha, bool /* hasTransparentPixels */)
    {
        switch (sourceFormat)
        {
        case Bitmap::RGB565:
            blitCopyRGB565(sourceData, source, blitRect, alpha);
            break;
        case Bitmap::ARGB8888:
            blitCopyARGB8888(sourceData, source, blitRect, alpha);
            break;
        case Bitmap::RGB888:
            blitCopyRGB888(sourceData, source, blitRect, alpha);
            break;
        case Bitmap::GRAY2:
        case Bitmap::GRAY4:
        case Bitmap::BW:
        case Bitmap::BW_RLE:
        case Bitmap::ARGB2222:
        case Bitmap::ABGR2222:
        case Bitmap::RGBA2222:
        case Bitmap::BGRA2222:
        case Bitmap::L8:
        case Bitmap::A4:
        case Bitmap::CUSTOM:
        case Bitmap::COMPRESSED_RGB565:
        case Bitmap::COMPRESSED_RGB888:
        case Bitmap::COMPRESSED_ARGB8888:
            assert(false && "LCDDMA2D3_CMDList: Unsupported bitmap format in LCD16::blitCopy!");
        }
    }

    /**
     * Private version of draw-glyph with explicit destination buffer pointer argument. For
     * all parameters (except the buffer pointer) see the public function drawString().
     *
     * @param [out] wbuf16          The destination (frame) buffer to draw to.
     * @param       widgetArea      The canvas to draw the glyph inside.
     * @param       x               Horizontal offset to start drawing the glyph.
     * @param       y               Vertical offset to start drawing the glyph.
     * @param       offsetX         Horizontal offset in the glyph to start drawing from.
     * @param       offsetY         Vertical offset in the glyph to start drawing from.
     * @param       invalidatedArea The area to draw inside.
     * @param       glyph           Specifications of the glyph to draw.
     * @param       glyphData       Data containing the actual glyph (dense format)
     * @param       byteAlignRow    Each row of glyph data starts in a new byte.
     * @param       color           The color of the glyph.
     * @param       bitsPerPixel    Bit depth of the glyph.
     * @param       alpha           The transparency of the glyph.
     * @param       rotation        Rotation to do before drawing the glyph.
     */
    void drawGlyph(uint16_t* wbuf16, Rect widgetArea, int16_t x, int16_t y, uint16_t offsetX, uint16_t offsetY, const Rect& invalidatedArea, const GlyphNode* glyph, const uint8_t* glyphData, uint8_t byteAlignRow, colortype color, uint8_t bitsPerPixel, uint8_t alpha, TextRotation rotation)
    {
        // Rotated text uses implementation in super class
        if (rotation != TEXT_ROTATE_0)
        {
            LCD_TYPE::drawGlyph(wbuf16, widgetArea, x, y, offsetX, offsetY, invalidatedArea, glyph, glyphData, byteAlignRow, color, bitsPerPixel, alpha, rotation);
            return;
        }

        const int glyph_width = (int)glyph->width();
        const int glyph_height = (int)glyph->height();

        if (!glyph_width || !glyph_height)
        {
            return;
        }

        if (offsetX)
        {
            offsetX -= glyph->left;
            if ((int16_t)offsetX < 0)
            {
                // Account for special case where glyph starts before 0, causing offsetX to become negative.
                x -= offsetX;
                offsetX = 0;
            }
        }
        else
        {
            x += glyph->left;
        }

        if (invalidatedArea.x > x)
        {
            offsetX += invalidatedArea.x - x;
            x = invalidatedArea.x;
        }

        if (!byteAlignRow && bitsPerPixel != 8)
        {
            assert(0 && "LCDDMA2D3_CMDList::drawGlyph only supports byteAligned font formats!");
            return;
        }

        const int16_t maxX = invalidatedArea.right();
        const int16_t maxY = invalidatedArea.bottom();

        // Calculate display coordinates
        int16_t real_x = widgetArea.x + x;
        int16_t real_y = widgetArea.y + y;

        const bool rotatedDisplay = HAL::DISPLAY_ROTATION == rotate90;
        if (rotatedDisplay)
        {
            const int glyphHeightToDraw = MIN(glyph_height - offsetY, maxY - y); // width or columns in fb!
            if (glyphHeightToDraw <= 0)
            {
                return;
            }

            DisplayTransformation::transformDisplayToFrameBuffer(real_x, real_y);

            switch (bitsPerPixel)
            {
            case 8:
                {
                    const uint16_t glyph_stride = glyph_width;
                    const int glyphColumns = MIN(glyph_width, maxX - (x - offsetX)) - offsetX;
                    if (glyphColumns == 0)
                    {
                        return;
                    }
                    uint8_t* const wbuf = clientFrameBuffer + BYTES_PR_PIXEL * (real_x + clientFrameBufferWidth * (real_y - (glyphColumns - 1)));
                    const uint8_t* const srcData = glyphData + glyph_stride * offsetY + offsetX;
                    DMA2DV3::addBlitCopyGlyphCmdListRotatedDestination(wbuf, clientFrameBufferWidth, FORMAT, srcData, DMA2DV3::COLOR_MODE_A8, glyph_stride, glyphColumns, glyphHeightToDraw, 0, 0, color, alpha);
                    break;
                }
            case 4:
                {
                    const uint16_t glyph_stride = ((glyph_width + 1) & ~1);

                    // Whole width of letter? Fast path
                    if ((offsetX == 0) && (maxX >= x + glyph_stride))
                    {
                        uint8_t* const wbuf = clientFrameBuffer + BYTES_PR_PIXEL * (real_x + clientFrameBufferWidth * (real_y - (glyph_stride - 1)));
                        const uint8_t* srcData = glyphData + (glyph_stride / 2) * offsetY;
                        DMA2DV3::addBlitCopyGlyphCmdListRotatedDestination(wbuf, clientFrameBufferWidth, FORMAT, srcData, DMA2DV3::COLOR_MODE_A4, glyph_stride, glyph_stride, glyphHeightToDraw, 0, 0, color, alpha);
                    }
                    else
                    {
                        // Even start and end?
                        const int glyphColumns = MIN(glyph_width, maxX - (x - offsetX)) - offsetX;
                        if (glyphColumns == 0)
                        {
                            return;
                        }
                        uint8_t* const wbuf = clientFrameBuffer + BYTES_PR_PIXEL * (real_x + clientFrameBufferWidth * (real_y - (glyphColumns - 1)));
                        const uint8_t* const srcData = glyphData + (glyph_stride / 2) * offsetY + offsetX / 2;
                        const int preSkip = offsetX & 1;
                        const int colsInLastByte = (offsetX + glyphColumns) & 1;
                        const int trailSkip = (colsInLastByte > 0) ? 2 - colsInLastByte : 0; // Skip on trail if last column is not 2*N
                        DMA2DV3::addBlitCopyGlyphCmdListRotatedDestination(wbuf, clientFrameBufferWidth, FORMAT, srcData, DMA2DV3::COLOR_MODE_A4, glyph_stride, glyphColumns, glyphHeightToDraw, preSkip, trailSkip, color, alpha);
                    }
                    break;
                }
            case 2:
                {
                    const uint16_t glyph_stride = ((glyph_width + 3) & ~3);

                    // Whole width of letter? Fast path
                    if ((offsetX == 0) && (maxX >= x + glyph_stride))
                    {
                        uint8_t* const wbuf = clientFrameBuffer + BYTES_PR_PIXEL * (real_x + clientFrameBufferWidth * (real_y - (glyph_stride - 1)));
                        const uint8_t* srcData = glyphData + (glyph_stride / 4) * offsetY;
                        DMA2DV3::addBlitCopyGlyphCmdListRotatedDestination(wbuf, clientFrameBufferWidth, FORMAT, srcData, DMA2DV3::COLOR_MODE_A2, glyph_stride, glyph_stride, glyphHeightToDraw, 0, 0, color, alpha);
                    }
                    else
                    {
                        // Even start and end?
                        const int glyphColumns = MIN(glyph_width, maxX - (x - offsetX)) - offsetX;
                        if (glyphColumns == 0)
                        {
                            return;
                        }
                        uint8_t* const wbuf = clientFrameBuffer + BYTES_PR_PIXEL * (real_x + clientFrameBufferWidth * (real_y - (glyphColumns - 1)));
                        const uint8_t* const srcData = glyphData + (glyph_stride / 4) * offsetY + offsetX / 4;
                        const int preSkip = offsetX & 3;
                        const int colsInLastByte = (offsetX + glyphColumns) & 3;
                        const int trailSkip = (colsInLastByte > 0) ? 4 - colsInLastByte : 0; // Skip on trail if last column is not 4*N
                        DMA2DV3::addBlitCopyGlyphCmdListRotatedDestination(wbuf, clientFrameBufferWidth, FORMAT, srcData, DMA2DV3::COLOR_MODE_A2, glyph_stride, glyphColumns, glyphHeightToDraw, preSkip, trailSkip, color, alpha);
                    }
                    break;
                }
            case 1:
                {
                    const uint16_t glyph_stride = ((glyph_width + 7) & ~7);

                    // Whole width of letter? Fast path
                    if ((offsetX == 0) && (maxX >= x + glyph_stride))
                    {
                        uint8_t* const wbuf = clientFrameBuffer + BYTES_PR_PIXEL * (real_x + clientFrameBufferWidth * (real_y - (glyph_stride - 1)));
                        const uint8_t* srcData = glyphData + (glyph_stride / 8) * offsetY;
                        DMA2DV3::addBlitCopyGlyphCmdListRotatedDestination(wbuf, clientFrameBufferWidth, FORMAT, srcData, DMA2DV3::COLOR_MODE_A1, glyph_stride, glyph_stride, glyphHeightToDraw, 0, 0, color, alpha);
                    }
                    else
                    {
                        // Even start and end?
                        const int glyphColumns = MIN(glyph_width, maxX - (x - offsetX)) - offsetX;
                        if (glyphColumns == 0)
                        {
                            return;
                        }
                        uint8_t* const wbuf = clientFrameBuffer + BYTES_PR_PIXEL * (real_x + clientFrameBufferWidth * (real_y - (glyphColumns - 1)));
                        const uint8_t* const srcData = glyphData + (glyph_stride / 8) * offsetY + offsetX / 8;
                        const int preSkip = offsetX & 7;
                        const int colsInLastByte = (offsetX + glyphColumns) & 7;
                        const int trailSkip = (colsInLastByte > 0) ? 8 - colsInLastByte : 0; // Skip on trail if last column is not 8*N
                        DMA2DV3::addBlitCopyGlyphCmdListRotatedDestination(wbuf, clientFrameBufferWidth, FORMAT, srcData, DMA2DV3::COLOR_MODE_A1, glyph_stride, glyphColumns, glyphHeightToDraw, preSkip, trailSkip, color, alpha);
                    }
                    break;
                }
            }
        }
        else /* !rotatedDisplay */
        {
            uint8_t* const wbuf = clientFrameBuffer + BYTES_PR_PIXEL * (real_x + clientFrameBufferWidth * real_y);
            const int glyphLinesToDraw = MIN(glyph_height - offsetY, maxY - y);
            if (glyphLinesToDraw <= 0)
            {
                return;
            }

            switch (bitsPerPixel)
            {
            case 8:
                {
                    const uint16_t glyph_stride = glyph_width;
                    const int columns = MIN(glyph_width, maxX - (x - offsetX)) - offsetX;
                    if (columns == 0)
                    {
                        return;
                    }
                    const uint8_t* const srcData = glyphData + glyph_stride * offsetY + offsetX;
                    DMA2DV3::addBlitCopyGlyphCmdList(wbuf, clientFrameBufferWidth, FORMAT, srcData, DMA2DV3::COLOR_MODE_A8, glyph_stride, columns, glyphLinesToDraw, 0, 0, color, alpha);
                    break;
                }
            case 4:
                {
                    const uint16_t glyph_stride = ((glyph_width + 1) & ~1);
                    // Whole letter? Fast path
                    if ((offsetX == 0) && (maxX >= x + glyph_width))
                    {
                        const uint8_t* srcData = glyphData + (glyph_stride / 2) * offsetY;
                        DMA2DV3::addBlitCopyGlyphCmdList(wbuf, clientFrameBufferWidth, FORMAT, srcData, DMA2DV3::COLOR_MODE_A4, glyph_stride, glyph_stride, glyphLinesToDraw, 0, 0, color, alpha);
                    }
                    else
                    {
                        // Even start and end?
                        const int columns = MIN(glyph_width, maxX - (x - offsetX)) - offsetX;
                        if (columns == 0)
                        {
                            return;
                        }
                        const uint8_t* const srcData = glyphData + (glyph_stride / 2) * offsetY + offsetX / 2;
                        const int preSkip = offsetX & 1;
                        const int colsInLastByte = (offsetX + columns) & 1;
                        const int trailSkip = (colsInLastByte > 0) ? 2 - colsInLastByte : 0; // Skip on trail if last column is not 2*N
                        DMA2DV3::addBlitCopyGlyphCmdList(wbuf, clientFrameBufferWidth, FORMAT, srcData, DMA2DV3::COLOR_MODE_A4, glyph_stride, columns, glyphLinesToDraw, preSkip, trailSkip, color, alpha);
                    }
                    break;
                }
            case 2:
                {
                    const uint16_t glyph_stride = ((glyph_width + 3) & ~3);
                    // Whole letter? Fast path
                    if ((offsetX == 0) && (maxX >= x + glyph_width))
                    {
                        const uint8_t* srcData = glyphData + (glyph_stride / 4) * offsetY;
                        DMA2DV3::addBlitCopyGlyphCmdList(wbuf, clientFrameBufferWidth, FORMAT, srcData, DMA2DV3::COLOR_MODE_A2, glyph_stride, glyph_stride, glyphLinesToDraw, 0, 0, color, alpha);
                    }
                    else
                    {
                        // Even start and end?
                        const int columns = MIN(glyph_width, maxX - (x - offsetX)) - offsetX;
                        if (columns == 0)
                        {
                            return;
                        }
                        const uint8_t* const srcData = glyphData + (glyph_stride / 4) * offsetY + offsetX / 4;
                        const int preSkip = offsetX & 0x3;
                        const int colsInLastByte = (offsetX + columns) & 3;
                        const int trailSkip = (colsInLastByte > 0) ? 4 - colsInLastByte : 0; // Skip on trail if last column is not 4*N
                        DMA2DV3::addBlitCopyGlyphCmdList(wbuf, clientFrameBufferWidth, FORMAT, srcData, DMA2DV3::COLOR_MODE_A2, glyph_stride, columns, glyphLinesToDraw, preSkip, trailSkip, color, alpha);
                    }
                    break;
                }
            case 1:
                {
                    const uint16_t glyph_stride = ((glyph_width + 7) & ~7);
                    // Whole letter? Fast path
                    if ((offsetX == 0) && (maxX >= x + glyph_width))
                    {
                        const uint8_t* srcData = glyphData + (glyph_stride / 8) * offsetY;
                        DMA2DV3::addBlitCopyGlyphCmdList(wbuf, clientFrameBufferWidth, FORMAT, srcData, DMA2DV3::COLOR_MODE_A1, glyph_stride, glyph_stride, glyphLinesToDraw, 0, 0, color, alpha);
                    }
                    else
                    {
                        // Even start and end?
                        const int columns = MIN(glyph_width, maxX - (x - offsetX)) - offsetX;
                        if (columns == 0)
                        {
                            return;
                        }
                        const uint8_t* const srcData = glyphData + (glyph_stride / 8) * offsetY + offsetX / 8;
                        const int preSkip = offsetX & 0x7;
                        const int colsInLastByte = (offsetX + columns) & 7;
                        const int trailSkip = (colsInLastByte > 0) ? 8 - colsInLastByte : 0; // Skip on trail if last column is not 8*N
                        DMA2DV3::addBlitCopyGlyphCmdList(wbuf, clientFrameBufferWidth, FORMAT, srcData, DMA2DV3::COLOR_MODE_A1, glyph_stride, columns, glyphLinesToDraw, preSkip, trailSkip, color, alpha);
                    }
                    break;
                }
            }
        }
    }

    /**
     * Copies part of the framebuffer to the data section of a bitmap. The bitmap must be a
     * dynamic bitmap or animation storage (BITMAP_ANIMATION_STORAGE). The two regions given
     * are the visible region and the absolute region on screen. This is used to copy only a
     * part of the framebuffer. This might be the case if a SnapshotWidget is placed inside
     * a Container where parts of the SnapshowWidget is outside the area defined by the
     * Container. The visible region must be completely inside the absolute region.
     *
     * @param  visRegion The visible region.
     * @param  absRegion The absolute region.
     * @param  bitmapId  Identifier for the bitmap.
     *
     * @return Null if it fails, else a pointer to the data in the given bitmap.
     *
     * @see blitCopy, copyFrameBufferRegionToMemory(const Rect&, const Rect&, uint8_t*, int16_t, int16_t)
     *
     * @note There is only one instance of animation storage. The content of the bitmap data
     *       /animation storage outside the given region is left untouched.
     */
    uint16_t* copyFrameBufferRegionToMemory(const Rect& visRegion, const Rect& absRegion, const BitmapId bitmapId)
    {
        assert((HAL::getInstance()->getFrameRefreshStrategy() != HAL::REFRESH_STRATEGY_PARTIAL_FRAMEBUFFER) && "Copy framebuffer region to memory is not supported in Partial Framebuffer mode!");
        const Bitmap bmp(bitmapId);
        assert((bmp.getFormat() == LCD_TYPE::framebufferFormat()) && "The given bitmap has an invalid format");
        uint8_t* bmp_data = const_cast<uint8_t*>(bmp.getData());
        assert((bmp_data != 0) && "No memory allocated for the given Bitmap");
        copyFrameBufferRegionToMemory(visRegion, absRegion, bmp_data, bmp.getWidth(), bmp.getHeight());
        return (uint16_t*)bmp_data;
    }

    /**
     * Copies part of the framebuffer to memory. The memory is assumed to have the same format as
     * the framebuffer. The two regions given are the visible region and the absolute region on
     * screen. This is used to copy only a part of the framebuffer. This might be the case if a
     * SnapshotWidget is placed inside a Container where parts of the SnapshowWidget is outside the
     * area defined by the Container. The visible region must be completely inside the absolute
     * region.
     *
     * @param           visRegion   The visible region.
     * @param           absRegion   The absolute region.
     * @param [in,out]  dst         Destination memory in same format as the framebuffer.
     * @param           dstWidth    Width of the destination.
     * @param           dstHeight   Height of the destination.
     *
     * @return  The rect that was actually copied to the destination buffer.
     *
     * @see blitCopy, copyFrameBufferRegionToMemory(const Rect&, const Rect&, const BitmapId)
     *
     * @note    There is only one instance of animation storage. The content of the bitmap data
     *          /animation storage outside the given region is left untouched.
     */
    Rect copyFrameBufferRegionToMemory(const Rect& visRegion, const Rect& absRegion, uint8_t* dst, int16_t dstWidth, int16_t dstHeight)
    {
        assert((HAL::getInstance()->getFrameRefreshStrategy() != HAL::REFRESH_STRATEGY_PARTIAL_FRAMEBUFFER) && "Copy framebuffer region to memory is not supported in Partial Framebuffer mode!");
        assert((dst != (void*)0) && "A correct destination memory pointer must be provided!");
        // Get the rectangle inside visRegion, absRegion and the bitmap region. Also limit to the display
        Rect srcRect(absRegion.x, absRegion.y, dstWidth, dstHeight); // Bitmap position on display
        srcRect &= absRegion;
        srcRect &= visRegion;
        srcRect.restrictTo(HAL::DISPLAY_WIDTH, HAL::DISPLAY_HEIGHT); // Limit to frame buffer
        if (srcRect.isEmpty())
        {
            return Rect();
        }
        // The solidRect inside the bitmap is relative to the absRegion
        Rect solidRect(srcRect);
        solidRect.x -= absRegion.x;
        solidRect.y -= absRegion.y;
        // Transform the solidRect to framebuffer coordinated. This is the area that must be copied to the destination
        Rect dstRect(solidRect);
        Rect bitmapRect(0, 0, dstWidth, dstHeight);
        DisplayTransformation::transformDisplayToFrameBuffer(dstRect, bitmapRect);

        DisplayTransformation::transformDisplayToFrameBuffer(srcRect);    // Get the rect in the frame buffer
        DisplayTransformation::transformDisplayToFrameBuffer(bitmapRect); // Only used for correct width and height

        const int16_t dstStride = bitmapRect.width;          // Pixels
        const int srcStride = LCD_TYPE::framebufferStride(); // Bytes

        dst = dst + BYTES_PR_PIXEL * (dstRect.x + dstRect.y * dstStride);
        uint8_t* src = reinterpret_cast<uint8_t*>(HAL::getInstance()->getTFTFrameBuffer()) + BYTES_PR_PIXEL * srcRect.x + srcRect.y * srcStride;

        switch (FORMAT)
        {
        case DMA2DV3::COLOR_MODE_ARGB8888:
            DMA2DV3::addBlitCopyARGB8888CmdList(dst, dstStride, FORMAT, src, clientFrameBufferWidth, srcRect, 255);
            break;
        case DMA2DV3::COLOR_MODE_RGB888:
            DMA2DV3::addBlitCopyRGB888CmdList(dst, dstStride, FORMAT, src, clientFrameBufferWidth, srcRect, 255);
            break;
        case DMA2DV3::COLOR_MODE_RGB565:
            DMA2DV3::addBlitCopyRGB565CmdList(dst, dstStride, FORMAT, src, clientFrameBufferWidth, srcRect, 255);
            break;
        }
        return solidRect;
    }

    /**
     * Copies part of the displayed framebuffer to current framebuffer.
     * The region given is the absolute region on screen.
     *
     * @param  region               A rectangle describing what region of the displayed framebuffer
     *                              is to be copied to the framebuffer.
     *
     * @note    The copy is performed only when double buffering is enabled. Otherwise the given
     *          region in current framebuffer is left untouched.
     */
    void copyAreaFromTFTToClientBuffer(const Rect& region)
    {
        Rect transformedSource;
        transformedSource.x = 0;
        transformedSource.y = 0;
        transformedSource.width = HAL::DISPLAY_ROTATION == rotate0 ? HAL::FRAME_BUFFER_WIDTH : HAL::FRAME_BUFFER_HEIGHT;
        transformedSource.height = HAL::DISPLAY_ROTATION == rotate0 ? HAL::FRAME_BUFFER_HEIGHT : HAL::FRAME_BUFFER_WIDTH;
        Rect transformedBlitRect = region;

        DisplayTransformation::transformDisplayToFrameBuffer(transformedBlitRect, transformedSource);
        DisplayTransformation::transformDisplayToFrameBuffer(transformedSource);

        // Transform to absolute coordinates
        transformedBlitRect.x += transformedSource.x;
        transformedBlitRect.y += transformedSource.y;

        uint8_t* dst = clientFrameBuffer + BYTES_PR_PIXEL * (transformedBlitRect.x + clientFrameBufferWidth * transformedBlitRect.y);
        uint8_t* src = reinterpret_cast<uint8_t*>(HAL::getInstance()->getTFTFrameBuffer()) + (dst - clientFrameBuffer);

        switch (FORMAT)
        {
        case DMA2DV3::COLOR_MODE_ARGB8888:
            DMA2DV3::addBlitCopyARGB8888CmdList((uint8_t*)dst, clientFrameBufferWidth, FORMAT, src, clientFrameBufferWidth, transformedBlitRect, 255);
            break;
        case DMA2DV3::COLOR_MODE_RGB888:
            DMA2DV3::addBlitCopyRGB888CmdList((uint8_t*)dst, clientFrameBufferWidth, FORMAT, src, clientFrameBufferWidth, transformedBlitRect, 255);
            break;
        case DMA2DV3::COLOR_MODE_RGB565:
            DMA2DV3::addBlitCopyRGB565CmdList((uint8_t*)dst, clientFrameBufferWidth, FORMAT, src, clientFrameBufferWidth, transformedBlitRect, 255);
            break;
        }
    }

    /**
     * Sets the current framebuffer address and width.
     *
     * @param buffer Address of the framebuffer.
     * @param width  Width of the framebuffer in pixels.
     */
    void setFrameBuffer(uint8_t* const buffer, uint32_t width)
    {
        clientFrameBuffer = buffer;
        clientFrameBufferWidth = width;
    }

private:
    uint8_t* clientFrameBuffer;      // Address
    uint32_t clientFrameBufferWidth; // Pixels

    void blitCopyARGB8888(const uint8_t* sourceData, const Rect& source, const Rect& blitRect, uint8_t alpha)
    {
        Rect transformedSource = source;
        Rect transformedBlitRect = blitRect;

        DisplayTransformation::transformDisplayToFrameBuffer(transformedSource);
        DisplayTransformation::transformDisplayToFrameBuffer(transformedBlitRect, source);

        // Move to blit rect in source data
        sourceData += (transformedBlitRect.x + transformedSource.width * transformedBlitRect.y) * 4;

        // Transform to absolute coordinates
        transformedBlitRect.x += transformedSource.x;
        transformedBlitRect.y += transformedSource.y;

        uint8_t* wbuf = clientFrameBuffer + BYTES_PR_PIXEL * (transformedBlitRect.x + clientFrameBufferWidth * transformedBlitRect.y);
        DMA2DV3::addBlitCopyARGB8888CmdList(wbuf, clientFrameBufferWidth, FORMAT, sourceData, transformedSource.width, transformedBlitRect, alpha);
    }

    void blitCopyRGB888(const uint8_t* sourceData, const Rect& source, const Rect& blitRect, uint8_t alpha)
    {
        Rect transformedSource = source;
        Rect transformedBlitRect = blitRect;

        DisplayTransformation::transformDisplayToFrameBuffer(transformedSource);
        DisplayTransformation::transformDisplayToFrameBuffer(transformedBlitRect, source);

        // Move to blit rect in source data
        sourceData += (transformedBlitRect.x + transformedSource.width * transformedBlitRect.y) * 3;

        // Transform to absolute coordinates
        transformedBlitRect.x += transformedSource.x;
        transformedBlitRect.y += transformedSource.y;

        uint8_t* wbuf = clientFrameBuffer + BYTES_PR_PIXEL * (transformedBlitRect.x + clientFrameBufferWidth * transformedBlitRect.y);
        DMA2DV3::addBlitCopyRGB888CmdList(wbuf, clientFrameBufferWidth, FORMAT, sourceData, transformedSource.width, transformedBlitRect, alpha);
    }

    void blitCopyRGB565(const uint8_t* sourceData, const Rect& source, const Rect& blitRect, uint8_t alpha)
    {
        Rect transformedSource = source;
        Rect transformedBlitRect = blitRect;

        DisplayTransformation::transformDisplayToFrameBuffer(transformedSource);
        DisplayTransformation::transformDisplayToFrameBuffer(transformedBlitRect, source);

        // Move to blit rect in source data
        sourceData += (transformedBlitRect.x + transformedSource.width * transformedBlitRect.y) * 2;

        // Transform to absolute coordinates
        transformedBlitRect.x += transformedSource.x;
        transformedBlitRect.y += transformedSource.y;

        uint8_t* wbuf = clientFrameBuffer + BYTES_PR_PIXEL * (transformedBlitRect.x + clientFrameBufferWidth * transformedBlitRect.y);
        DMA2DV3::addBlitCopyRGB565CmdList(wbuf, clientFrameBufferWidth, FORMAT, sourceData, transformedSource.width, transformedBlitRect, alpha);
    }

    void blitCopyL8(const uint8_t* sourceData, const uint8_t* clutData, const Rect& source, const Rect& blitRect, uint8_t alpha)
    {
        if (!alpha)
        {
            // nothing to draw - 100% transparent
            return;
        }

        const DMA2DV3::clutData_t* const palette = reinterpret_cast<const DMA2DV3::clutData_t*>(clutData);

        const Bitmap::Compression compressionAlgorithm = static_cast<Bitmap::Compression>(palette->compression);

        if (compressionAlgorithm != Bitmap::COMPRESSION_L8_NONE)
        {
            LCD_TYPE::blitCopyL8(sourceData, clutData, source, blitRect, alpha);
            return;
        }

        // Load palette
        DMA2DV3::addLoadCLUTCmdList(palette->data, palette->size, static_cast<Bitmap::ClutFormat>(palette->format));

        // Draw image
        Rect transformedSource = source;
        Rect transformedBlitRect = blitRect;

        DisplayTransformation::transformDisplayToFrameBuffer(transformedSource);
        DisplayTransformation::transformDisplayToFrameBuffer(transformedBlitRect, source);

        // Move to blit rect in source data
        sourceData += transformedBlitRect.x + transformedSource.width * transformedBlitRect.y;

        // Transform to absolute coordinates
        transformedBlitRect.x += transformedSource.x;
        transformedBlitRect.y += transformedSource.y;

        uint8_t* wbuf = clientFrameBuffer + BYTES_PR_PIXEL * (transformedBlitRect.x + clientFrameBufferWidth * transformedBlitRect.y);
        DMA2DV3::addBlitCopyL8CmdList(wbuf, clientFrameBufferWidth, FORMAT, sourceData, transformedSource.width, transformedBlitRect, static_cast<Bitmap::ClutFormat>(palette->format), alpha);
    }
};

typedef LCDDMA2D3_CMDList<LCD16bpp, DMA2DV3::COLOR_MODE_RGB565, 2> LCDDMA2D3_CMDList_16bpp;   ///< Short name for LCDDMA2D3_CMDList on RGB565 framebuffer.
typedef LCDDMA2D3_CMDList<LCD24bpp, DMA2DV3::COLOR_MODE_RGB888, 3> LCDDMA2D3_CMDList_24bpp;   ///< Short name for LCDDMA2D3_CMDList on RGB565 framebuffer.
typedef LCDDMA2D3_CMDList<LCD32bpp, DMA2DV3::COLOR_MODE_ARGB8888, 4> LCDDMA2D3_CMDList_32bpp; ///< Short name for LCDDMA2D3_CMDList on RGB565 framebuffer.

} // namespace touchgfx

#endif // TOUCHGFX_LCDDMA2D3_CMDLIST_HPP
