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
 * @file touchgfx/transitions/WipeTransition.hpp
 *
 * Declares the touchgfx::WipeTransition class.
 */
#ifndef TOUCHGFX_WIPETRANSITION_HPP
#define TOUCHGFX_WIPETRANSITION_HPP

#include <touchgfx/EasingEquations.hpp>
#include <touchgfx/containers/Container.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/hal/Types.hpp>
#include <touchgfx/lcd/LCD.hpp>
#include <touchgfx/transitions/Transition.hpp>

namespace touchgfx
{
/**
 * A Transition that expands the new screen over the previous from
 * the given direction.  This transition only draws the pixels in the
 * framebuffer once, and never moves any pixels. It is therefore very
 * useful on MCUs with limited performance.
 */
template <Direction templateDirection>
class WipeTransition : public Transition
{
public:
    /**
     * Initializes a new instance of the WipeTransition class.
     *
     * @param  transitionSteps (Optional) Number of steps in the transition animation.
     */
    WipeTransition(const uint8_t transitionSteps = 20)
        : Transition(),
          animationSteps(transitionSteps),
          animationCounter(0),
          calculatedValue(0),
          prevCalculatedValue(0)
    {
        switch (templateDirection)
        {
        case EAST:
        case WEST:
            targetValue = HAL::DISPLAY_WIDTH;
            break;
        case NORTH:
        case SOUTH:
            targetValue = HAL::DISPLAY_HEIGHT;
            break;
        }
    }

    /**
     * Handles the tick event when transitioning. It uncovers and
     * invalidates increasing parts of the new screen elements.
     */
    virtual void handleTickEvent()
    {
        Transition::handleTickEvent();
        animationCounter++;

        // Calculate new position or stop animation
        if (animationCounter > animationSteps)
        {
            // Final step: stop the animation
            done = true;
            animationCounter = 0;
            return;
        }

        // Calculate value in [0;targetValue]
        calculatedValue = EasingEquations::cubicEaseOut(animationCounter, 0, targetValue, animationSteps);

        // Note: Result of "calculatedValue & 1" is compiler dependent for negative values of calculatedValue
        if ((calculatedValue % 2) != 0)
        {
            // Optimization: calculatedValue is odd, add 1/-1 to move drawables modulo 32 bits in framebuffer
            calculatedValue += (calculatedValue > 0 ? 1 : -1);
        }

        // calculatedValue is the width/height of the visible area

        Rect rect;
        switch (templateDirection)
        {
        case EAST:
            {
                rect.x = HAL::DISPLAY_WIDTH - calculatedValue;
                rect.y = 0;
                rect.width = calculatedValue - prevCalculatedValue;
                rect.height = HAL::DISPLAY_HEIGHT;
                break;
            }
        case WEST:
            {
                rect.x = prevCalculatedValue;
                rect.y = 0;
                rect.width = calculatedValue - prevCalculatedValue;
                rect.height = HAL::DISPLAY_HEIGHT;
                break;
            }
        case NORTH:
            {
                rect.x = 0;
                rect.y = prevCalculatedValue;
                rect.width = HAL::DISPLAY_WIDTH;
                rect.height = calculatedValue - prevCalculatedValue;
                break;
            }
        case SOUTH:
            {
                rect.x = 0;
                rect.y = HAL::DISPLAY_HEIGHT - calculatedValue;
                rect.width = HAL::DISPLAY_WIDTH;
                rect.height = calculatedValue - prevCalculatedValue;
                break;
            }
        }
        prevCalculatedValue = calculatedValue;
        Application::getInstance()->invalidateArea(rect);

        // The WipeTransition only draws to parts of the non-TFT
        // framebuffer. To avoid glitches in Double buffering mode
        // both framebuffers must be made identical.
        if (animationCounter == 1 && HAL::USE_DOUBLE_BUFFERING)
        {
            // Synchronize framebuffers
            Application::getInstance()->copyInvalidatedAreasFromTFTToClientBuffer();
        }
    }

    virtual void init()
    {
        Transition::init();
    }

    virtual void invalidate()
    {
        // The last step when finalizing a transition (see MVPApplication::finalizeTransition)
        // is to call invalidate on the transition. For the WipeTransition we want to erase any
        // invalidated areas that might have been added when setting up the new screen, which
        // is the first step of finalizing a transition (see MVPApplication::finalizeTransition).
        Application::getInstance()->clearCachedAreas();
    }

private:
    const uint8_t animationSteps; ///< Number of steps the transition should move per complete animation.
    uint8_t animationCounter;     ///< Current step in the transition animation.
    int16_t targetValue;          ///< The target value for the transition animation.
    int16_t calculatedValue;      ///< The calculated X or Y value to move the snapshot and the children.
    int16_t prevCalculatedValue;  ///< The previous calculated value.
};

} // namespace touchgfx

#endif // TOUCHGFX_WIPETRANSITION_HPP
