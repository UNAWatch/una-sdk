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

#include <string.h>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/transforms/DisplayTransformation.hpp>
#include <touchgfx/widgets/QRCode.hpp>

namespace touchgfx
{
QRCode::QRCode()
    : qrCodeData(0),
      qrTempBuffer(0),
      qrCodeVersion(1),
      sizeOfQRCodeSymbol(0),
      scale(1),
      eccLevel(ECC_LOW),
      alpha(0xFF)
{
    setColors(0x000000, 0xFFFFFF);
}

bool QRCode::convertStringToQRCode(const char* text)
{
    if (qrTempBuffer && qrCodeData)
    {
        // Clear old QR size field.
        qrCodeData[0] = 0;
        bool ok = qrcodegen_encodeText(text, qrTempBuffer, qrCodeData, (qrcodegen_Ecc)eccLevel,
                                       qrCodeVersion, qrCodeVersion, qrcodegen_Mask_AUTO, true);

        return ok;
    }

    return false;
}

bool QRCode::convertBinaryDataToQRCode(const uint8_t* data, size_t length)
{
    const size_t bufferLength = qrcodegen_BUFFER_LEN_FOR_VERSION(qrCodeVersion);
    // Assuming qrTempBuffer has correct length (bufferLength)
    if (length <= bufferLength)
    {
        if (qrTempBuffer && qrCodeData)
        {
            // Copy data to scratch buffer
            memcpy(qrTempBuffer, data, length);
            // Clear old QR size field.
            qrCodeData[0] = 0;
            bool ok = qrcodegen_encodeBinary(qrTempBuffer, length, qrCodeData, (qrcodegen_Ecc)eccLevel,
                                             qrCodeVersion, qrCodeVersion, qrcodegen_Mask_AUTO, true);

            return ok;
        }
    }

    return false;
}

uint8_t* QRCode::drawBitRGB565(uint8_t* dst, bool on, int pixels) const
{
    uint16_t* dst16 = (uint16_t*)dst;
    uint16_t* const dst_end = dst16 + pixels;
    const uint16_t color = on ? color0_565 : color1_565;
    while (dst16 < dst_end)
    {
        *dst16++ = color;
    }
    return (uint8_t*)dst16;
}

uint8_t* QRCode::drawBitRGB565Blend(uint8_t* dst, bool on, int pixels) const
{
    uint16_t* dst16 = (uint16_t*)dst;
    uint16_t* const dst_end = dst16 + pixels;

    const uint16_t alphaR = on ? alphaR0 : alphaR1;
    const uint16_t alphaG = on ? alphaG0 : alphaG1;
    const uint16_t alphaB = on ? alphaB0 : alphaB1;

    while (dst16 < dst_end)
    {
        const uint16_t bufpix = *dst16;
        const uint8_t fbr = Color::getRedFromRGB565(bufpix);
        const uint8_t fbg = Color::getGreenFromRGB565(bufpix);
        const uint8_t fbb = Color::getBlueFromRGB565(bufpix);

        *dst16++ = getRGB565ColorFromRGB(div255(alphaR + fbr * ialpha), div255(alphaG + fbg * ialpha), div255(alphaB + fbb * ialpha));
    }
    return (uint8_t*)dst16;
}

uint8_t* QRCode::drawBitRGB888(uint8_t* dst, bool on, int pixels) const
{
    uint8_t* const dst_end = dst + pixels * 3;

    const uint8_t r = on ? r0 : r1;
    const uint8_t g = on ? g0 : g1;
    const uint8_t b = on ? b0 : b1;

    while (dst < dst_end)
    {
        *dst++ = b;
        *dst++ = g;
        *dst++ = r;
    }
    return dst;
}

uint8_t* QRCode::drawBitRGB888Blend(uint8_t* dst, bool on, int pixels) const
{
    uint8_t* const dst_end = dst + pixels * 3;
    const uint16_t alphaRed = on ? alphaR0 : alphaR1;
    const uint16_t alphaGreen = on ? alphaG0 : alphaG1;
    const uint16_t alphaBlue = on ? alphaB0 : alphaB1;

    while (dst < dst_end)
    {
        *dst = div255(alphaBlue + *dst * ialpha);
        dst++; // Avoid side effects
        *dst = div255(alphaGreen + *dst * ialpha);
        dst++; // Avoid side effects
        *dst = div255(alphaRed + *dst * ialpha);
        dst++; // Avoid side effects
    }
    return dst;
}

uint8_t* QRCode::drawBitARGB8888(uint8_t* dst, bool on, int pixels) const
{
    uint8_t* const dst_end = dst + pixels * 4;

    const uint8_t r = on ? r0 : r1;
    const uint8_t g = on ? g0 : g1;
    const uint8_t b = on ? b0 : b1;

    while (dst < dst_end)
    {
        *dst++ = b;
        *dst++ = g;
        *dst++ = r;
        *dst++ = 0xFF;
    }
    return dst;
}

uint8_t* QRCode::drawBitARGB8888Blend(uint8_t* dst, bool on, int pixels) const
{
    uint32_t* dst32 = (uint32_t*)dst;
    uint32_t* dst32_end = dst32 + pixels;
    const uint16_t alphaRed = on ? alphaR0 : alphaR1;
    const uint16_t alphaGreen = on ? alphaG0 : alphaG1;
    const uint16_t alphaBlue = on ? alphaB0 : alphaB1;

    while (dst32 < dst32_end)
    {
        const uint32_t rgbBg = *dst32;
        const uint8_t alphaBg = rgbBg >> 24;
        if (alphaBg == 0)
        {
            // Completely transparent background
            uint8_t* dst8 = (uint8_t*)dst32;
            if (on)
            {
                *dst8++ = b0;
                *dst8++ = g0;
                *dst8++ = r0;
                *dst8 = alpha;
            }
            else
            {
                *dst8++ = b1;
                *dst8++ = g1;
                *dst8++ = r1;
                *dst8 = alpha;
            }
            dst32++;
        }
        else
        {
            const uint8_t alphaMult = div255(alpha * alphaBg);
            const uint8_t alphaOut = alpha + alphaBg - alphaMult;

            const uint8_t blueOut = (alphaBlue + (rgbBg & 0xFF) * (alphaBg - alphaMult)) / alphaOut;
            const uint8_t greenOut = (alphaGreen + ((rgbBg >> 8) & 0xFF) * (alphaBg - alphaMult)) / alphaOut;
            const uint8_t redOut = (alphaRed + ((rgbBg >> 16) & 0xFF) * (alphaBg - alphaMult)) / alphaOut;

            *dst32++ = (alphaOut << 24) | (redOut << 16) | (greenOut << 8) | blueOut;
        }
    }

    return (uint8_t*)dst32;
}

void QRCode::draw(const Rect& invalidatedArea) const
{
    // To check if qrCodeData is ready to be drawn
    int result = qrCodeData[0];
    if (!((qrcodegen_VERSION_MIN * 4 + 17) <= result) || !(result <= (qrcodegen_VERSION_MAX * 4 + 17)))
    {
        return;
    }

    if (alpha == 0)
    {
        return;
    }

    Rect absInvalidated = invalidatedArea;
    translateRectToAbsolute(absInvalidated);

    // QR bit drawing function
    uint8_t* (QRCode::*drawfunc)(uint8_t * dst, bool on, int pixels) const = 0;

    const int bpp = HAL::lcd().bitDepth();

    // Select correct drawing function for framebuffer format and alpha
    switch (bpp)
    {
    case 16:
        // RGB565
        drawfunc = (alpha < 255) ? &QRCode::drawBitRGB565Blend : &QRCode::drawBitRGB565;
        break;
    case 24:
        // RGB888
        drawfunc = (alpha < 255) ? &QRCode::drawBitRGB888Blend : &QRCode::drawBitRGB888;
        break;
    case 32:
        // ARGB8888
        drawfunc = (alpha < 255) ? &QRCode::drawBitARGB8888Blend : &QRCode::drawBitARGB8888;
        break;
    default:
        return;
    }

    const int bytespp = bpp / 8;
    const int line_stride = HAL::lcd().framebufferStride();

    uint8_t* const fb = (uint8_t*)HAL::getInstance()->lockFrameBuffer();

    if (HAL::DISPLAY_ROTATION == rotate90)
    {
        Rect transformedAbs = absInvalidated;
        DisplayTransformation::transformDisplayToFrameBuffer(transformedAbs);
        // Find address of first pixel in (unrotated) framebuffer = top right corner of inv. area.
        uint8_t* fb_line = fb + bytespp * (transformedAbs.x + transformedAbs.y * HAL::FRAME_BUFFER_WIDTH);
        // Start in rightmost column = top row in framebuffer
        // Drawing downwards in display = increasing address in framebuffer
        for (int col = 0; col < invalidatedArea.width; col++)
        {
            const int qr_x = (invalidatedArea.right() - 1 - col) / scale;
            const int row_end = invalidatedArea.y + invalidatedArea.height;
            uint8_t* dest = fb_line;
            for (int row = invalidatedArea.y; row < row_end;)
            {
                const int length = MIN(scale - row % scale, row_end - row);
                const bool bit = getQRBit(qr_x, row / scale);
                dest = (this->*drawfunc)(dest, bit, length);
                row += length;
            }
            fb_line += line_stride;
        }
    }
    else
    {
        uint8_t* fb_line = fb + (absInvalidated.x + absInvalidated.y * HAL::FRAME_BUFFER_WIDTH) * bytespp;
        for (int y = 0; y < absInvalidated.height; y++)
        {
            const int qr_y = (invalidatedArea.y + y) / scale;
            const int x_end = invalidatedArea.x + invalidatedArea.width;
            uint8_t* dest = fb_line;
            for (int x = invalidatedArea.x; x < x_end;)
            {
                const int length = MIN(scale - x % scale, x_end - x);
                const bool bit = getQRBit(x / scale, qr_y);
                dest = (this->*drawfunc)(dest, bit, length);
                x += length;
            }
            fb_line += line_stride;
        }
    }

    HAL::getInstance()->unlockFrameBuffer();
}

Rect QRCode::getSolidRect() const
{
    // Widget does not draw anything if it has no valid qrCodeData, so
    // report transparent to show background
    if (alpha == 255 && qrCodeData && qrCodeData[0] != 0)
    {
        return Rect(0, 0, getWidth(), getHeight());
    }
    return Rect();
}

void QRCode::updateWidthAndHeight()
{
    setWidth(sizeOfQRCodeSymbol * scale);
    setHeight(sizeOfQRCodeSymbol * scale);
}
} // namespace touchgfx
