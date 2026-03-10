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
 * @file touchgfx/widgets/QRCode.hpp
 *
 * Declares the touchgfx::QRCode class.
 */
#ifndef TOUCHGFX_QRCODE_HPP
#define TOUCHGFX_QRCODE_HPP

#include <touchgfx/Color.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/hal/Types.hpp>
#include <touchgfx/widgets/Widget.hpp>
#include <touchgfx/widgets/utils/qrcodegen.hpp>

/**
 * Macro to calculate the size of the buffers required for QR code generation.
 */
#define QRCODE_BUFFER_SIZE(version) ((((version)*4 + 17) * ((version)*4 + 17) + 7) / 8 + 1)

namespace touchgfx
{
/**
 * Widget capable of generating and showing a QRCode.
 *
 * This widget uses the QR Code generator library (C).
 * https://www.nayuki.io/page/qr-code-generator-library
 *
 * Important: The application must allocate buffers and provide these
 * through QRCode::setBuffers.
 *
 * The QRCode supports framebuffer formats RGB565, RGB888, ARGB8888.
 */
class QRCode : public Widget
{
public:
    /**
     * Construct a new QRCode using black and white colors with
     * alpha 255. The user must supply buffers and set the scale and
     * QR version as wanted.
     */
    QRCode();

    /**
     * Convert a text to QRCode data and save in the buffer.
     * The function returns false if the encoding of the string does
     * not fit in the QRCode. This depends on the version of the
     * QRCode and the ECCLevel. If the text was not accepted, the
     * QRCode Widget does not draw anything.
     * User must call invalidate() after changing the
     * data.
     *
     * @param text The text to show in the QRCode.
     * @return True if data was converted.
     */
    bool convertStringToQRCode(const char* text);

    /**
     * Convert data to QRCode data and save in the buffer.
     * The function returns false if the encoding of the data does
     * not fit in the QRCode. This depends on the version of the
     * QRCode and the ECCLevel. If the data was not accepted, the
     * QRCode Widget does not draw anything.
     * User must call invalidate() after changing the data. This
     * function can e.g. be used to store UTF8 encoded strings.
     *
     * @param data The data to show in the QRCode.
     * @param length Length of the data.
     * @return True if data was converted.
     */
    bool convertBinaryDataToQRCode(const uint8_t* data, size_t length);

    /**
     * Sets the scale of the QRCode.
     * Each bit will be shown as scale pixels.
     *
     * @param newScale The scale to use.
     */
    void setScale(int newScale)
    {
        // Multiply the size of the QRCode Container by the scale
        // Every QR Code version has a specific size, so we can't use arbitrary dimensions
        scale = newScale;
        updateWidthAndHeight();
    }

    /**
     * Gets the current scale of the QRCode.
     *
     * @return The current scale.
     *
     * @see setScale
     */
    int getScale() const
    {
        return scale;
    }

    /**
     * Sets the version (size) of the QRCode.
     * Version is between 1 to 40.
     *
     * @param version The QRCode version to use.
     */
    void setQRCodeVersion(uint8_t version)
    {
        qrCodeVersion = version;
        sizeOfQRCodeSymbol = version * 4 + 17;
        updateWidthAndHeight();
    }

    /**
     * Gets the current QRCode version.
     *
     * @return The current version.
     *
     * @see setQRCodeVersion
     */
    uint8_t getQRCodeVersion() const
    {
        return qrCodeVersion;
    }

    /** ECC levels */
    enum ECCLevel
    {
        ECC_LOW = 0,      ///< Low level of ECC (handles around 7% faults)
        ECC_MED = 1,      ///< Medium level of ECC (handles around 15% faults)
        ECC_QUARTILE = 2, ///< Good level of ECC (handles around 25% faults)
        ECC_HIGH = 3      ///< High level of ECC (handles around 30% faults)
    };

    /**
     * Sets the desired level of error correction codes.
     *
     * @param level The level of ECC.
     */
    void setErrorCorrectionLevel(ECCLevel level)
    {
        eccLevel = level;
    }

    /**
     * Gets the current error correction level.
     *
     * @return The current error correction level.
     *
     * @see setErrorCorrectionLevel
     */
    ECCLevel getErrorCorrectionLevel() const
    {
        return eccLevel;
    }

    /**
     * Sets the buffers used for QRCode generation.
     * The length of the buffers must match the version of the QRCode
     * used.  Use the above macro QRCODE_BUFFER_SIZE(version) when
     * allocating buffers.
     * The qrBuffer is holding the QRCode bit-stream.  The tempBuffer
     * is only used during QRCode generation. It can be used for other
     * purposes afterwards and shared between Widgets.
     *
     * @param qrBuffer Buffer to store the QRcode.
     * @param tempBuffer Scratch buffer used during QRCode generation.
     */
    void setBuffers(uint8_t* qrBuffer, uint8_t* tempBuffer)
    {
        qrCodeData = qrBuffer;
        qrTempBuffer = tempBuffer;
        qrCodeData[0] = 0;
    }

    /**
     * Sets the opacity (alpha value). This can be used to fade it away by gradually
     * decreasing the alpha value from 255 (solid) to 0 (invisible).
     *
     * @param  newAlpha The new alpha value. 255=solid, 0=invisible.
     *
     * @note The user code must call invalidate() in order to update the display.
     */
    void setAlpha(uint8_t newAlpha)
    {
        alpha = newAlpha;
        ialpha = 0xFF - newAlpha;
        multiplyAlphaColors();
    }

    /**
     * Gets the current alpha value of the widget. The alpha value is in range 255
     * (solid) to 0 (invisible).
     *
     * @return The current alpha value.
     *
     * @see setAlpha
     */
    uint8_t getAlpha() const
    {
        return alpha;
    }

    /**
     * Sets the colors used for the QRCode.
     * Black and White is traditionally used, but other colors will
     * also work.
     *
     * @param colorBlack The color used for 'black' pixels.
     * @param colorWhite The color used the 'white' pixels.
     */
    void setColors(colortype colorBlack, colortype colorWhite)
    {
        // Split in RGB
        r0 = Color::getRed(colorBlack);
        g0 = Color::getGreen(colorBlack);
        b0 = Color::getBlue(colorBlack);
        r1 = Color::getRed(colorWhite);
        g1 = Color::getGreen(colorWhite);
        b1 = Color::getBlue(colorWhite);
        // Calculate RGB565 values
        color0_565 = getRGB565Color(colorBlack);
        color1_565 = getRGB565Color(colorWhite);
        // Premultiply alpha
        multiplyAlphaColors();
    }

    virtual void draw(const Rect& invalidatedArea) const;
    virtual Rect getSolidRect() const;

private:
    void updateWidthAndHeight();
    uint8_t* drawBitRGB565(uint8_t* dst, bool on, int pixels) const;
    uint8_t* drawBitRGB565Blend(uint8_t* dst, bool on, int pixels) const;
    uint8_t* drawBitRGB888(uint8_t* dst, bool on, int pixels) const;
    uint8_t* drawBitRGB888Blend(uint8_t* dst, bool on, int pixels) const;
    uint8_t* drawBitARGB8888(uint8_t* dst, bool on, int pixels) const;
    uint8_t* drawBitARGB8888Blend(uint8_t* dst, bool on, int pixels) const;

    void multiplyAlphaColors()
    {
        alphaR0 = alpha * r0;
        alphaR1 = alpha * r1;
        alphaG0 = alpha * g0;
        alphaG1 = alpha * g1;
        alphaB0 = alpha * b0;
        alphaB1 = alpha * b1;
    }

    FORCE_INLINE_FUNCTION static uint16_t getRGB565Color(colortype color)
    {
        return ((color >> 8) & 0xF800) | ((color >> 5) & 0x07E0) | ((color >> 3) & 0x001F);
    }

    FORCE_INLINE_FUNCTION static uint16_t getRGB565ColorFromRGB(uint8_t red, uint8_t green, uint8_t blue)
    {
        return ((red << 8) & 0xF800) | ((green << 3) & 0x07E0) | ((blue >> 3) & 0x001F);
    }

    FORCE_INLINE_FUNCTION static uint8_t div255(uint16_t num)
    {
        return (num + 1 + (num >> 8)) >> 8;
    }

    FORCE_INLINE_FUNCTION bool getQRBit(int x, int y) const
    {
        const int index = y * sizeOfQRCodeSymbol + x;
        const int value = qrCodeData[(index >> 3) + 1];
        return (value >> (index & 7)) & 1;
    }

    uint8_t* qrCodeData;
    uint8_t* qrTempBuffer;
    uint8_t qrCodeVersion;
    int sizeOfQRCodeSymbol;
    int scale;
    ECCLevel eccLevel;
    uint16_t color0_565, color1_565;
    uint16_t alphaR0, alphaR1, alphaG0, alphaG1, alphaB0, alphaB1;
    uint8_t r0, g0, b0, r1, g1, b1;
    uint8_t alpha, ialpha; ///< The Alpha and inverse Alpha for this QR code.
};

} // namespace touchgfx

#endif // TOUCHGFX_QRCODE_HPP
