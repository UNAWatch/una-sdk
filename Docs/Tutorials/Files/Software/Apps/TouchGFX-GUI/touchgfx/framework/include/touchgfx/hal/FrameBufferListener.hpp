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
 * @file touchgfx/hal/FrameBufferListener.hpp
 *
 * Declares the touchgfx::FrameBufferListener class.
 */
#ifndef TOUCHGFX_FRAMEBUFFERLISTENER_HPP
#define TOUCHGFX_FRAMEBUFFERLISTENER_HPP

#include <touchgfx/hal/Types.hpp>

namespace touchgfx
{
/**
 * This class is an abstract interface for a class listening for
 * framebuffer address changes.
 */
class FrameBufferListener
{
public:
    /**
     * Called when the client (drawing) framebuffer changes.
     *
     * @param buffer The new client framebuffer address.
     * @param width  The width (stride) of the framebuffer in pixels.
     */
    virtual void setFrameBuffer(uint8_t* const buffer, uint32_t width) = 0;

    /** Virtual destructor. */
    virtual ~FrameBufferListener()
    {
    }
};

} // namespace touchgfx

#endif // TOUCHGFX_FRAMEBUFFERLISTENER_HPP
