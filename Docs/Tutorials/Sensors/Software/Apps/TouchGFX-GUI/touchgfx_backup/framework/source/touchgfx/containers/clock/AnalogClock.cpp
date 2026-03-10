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

#include <touchgfx/containers/clock/AnalogClock.hpp>

namespace touchgfx
{
AnalogClock::AnalogClock()
    : AbstractClock(),
      background(),
      hourHand(),
      minuteHand(),
      secondHand(),
      animationEquation(EasingEquations::linearEaseNone),
      animationDuration(0),
      clockRotationCenterX(0),
      clockRotationCenterY(0),
      lastHour(0),
      lastMinute(0),
      lastSecond(0),
      hourHandMinuteCorrectionActive(false),
      minuteHandSecondCorrectionActive(false)
{
    AnalogClock::add(background);

    hourHand.updateZAngle(0.f);
    minuteHand.updateZAngle(0.f);
    secondHand.updateZAngle(0.f);

    hourHand.setVisible(false);
    minuteHand.setVisible(false);
    secondHand.setVisible(false);
}

void AnalogClock::setBackground(const BitmapId backgroundBitmapId)
{
    setBackground(backgroundBitmapId, Bitmap(backgroundBitmapId).getWidth() / 2, Bitmap(backgroundBitmapId).getHeight() / 2);
}

void AnalogClock::setBackground(const BitmapId backgroundBitmapId, const int16_t rotationCenterX, const int16_t rotationCenterY)
{
    background.setBitmap(Bitmap(backgroundBitmapId));
    setWidthHeight(background);

    setRotationCenter(rotationCenterX, rotationCenterY);
}

void AnalogClock::setRotationCenter(int16_t rotationCenterX, int16_t rotationCenterY)
{
    clockRotationCenterX = rotationCenterX;
    clockRotationCenterY = rotationCenterY;
}

void AnalogClock::setupHourHand(const BitmapId hourHandBitmapId, int16_t rotationCenterX, int16_t rotationCenterY)
{
    setupHand(hourHand, hourHandBitmapId, rotationCenterX, rotationCenterY);
}

void AnalogClock::setupMinuteHand(const BitmapId minuteHandBitmapId, int16_t rotationCenterX, int16_t rotationCenterY)
{
    setupHand(minuteHand, minuteHandBitmapId, rotationCenterX, rotationCenterY);
}

void AnalogClock::setupSecondHand(const BitmapId secondHandBitmapId, int16_t rotationCenterX, int16_t rotationCenterY)
{
    setupHand(secondHand, secondHandBitmapId, rotationCenterX, rotationCenterY);
}

void AnalogClock::setupHand(TextureMapper& hand, const BitmapId bitmapId, int16_t rotationCenterX, int16_t rotationCenterY)
{
    remove(hand);

    hand.setBitmap(Bitmap(bitmapId));
    hand.setWidthHeight(*this);
    hand.setXY(0, 0);
    hand.setBitmapPosition(clockRotationCenterX - rotationCenterX, clockRotationCenterY - rotationCenterY);
    hand.setCameraDistance(300.0f);
    hand.setOrigo((float)clockRotationCenterX, (float)clockRotationCenterY, hand.getCameraDistance());
    hand.setCamera(hand.getOrigoX(), hand.getOrigoY());
    hand.setRenderingAlgorithm(TextureMapper::BILINEAR_INTERPOLATION);

    add(hand);
    hand.setVisible(true);
}

void AnalogClock::initializeTime24Hour(uint8_t hour, uint8_t minute, uint8_t second)
{
    lastHour = 255;
    lastMinute = 255;
    lastSecond = 255;

    // Disable animation and set time
    const uint16_t tempAnimationDuration = animationDuration;
    animationDuration = 1;
    setTime24Hour(hour, minute, second);

    animationDuration = tempAnimationDuration;
}

void AnalogClock::initializeTime12Hour(uint8_t hour, uint8_t minute, uint8_t second, bool am)
{
    initializeTime24Hour((hour % 12) + (am ? 0 : 12), minute, second);
}

void AnalogClock::setAlpha(uint8_t newAlpha)
{
    background.setAlpha(newAlpha);
    hourHand.setAlpha(newAlpha);
    minuteHand.setAlpha(newAlpha);
    secondHand.setAlpha(newAlpha);
}

uint8_t AnalogClock::getAlpha() const
{
    return background.getAlpha();
}

void AnalogClock::updateClock()
{
    float newHandAngle;
    float oldHandAngle;
    int16_t diff;

    // Move hour hand
    if (hourHand.isVisible() && ((currentHour % 12 != lastHour % 12) || (hourHandMinuteCorrectionActive && (currentMinute != lastMinute))))
    {
        newHandAngle = convertHandValueToAngle(12, currentHour % 12, hourHandMinuteCorrectionActive ? currentMinute : 0);

        // check if sign of absolute angles difference matches sign of normalized difference of angles
        oldHandAngle = hourHand.getZAngle();
        diff = (((int16_t)currentHour - (int16_t)lastHour + 24 + 6) % 12) - 6;
        if (diff > 0 && newHandAngle < oldHandAngle)
        {
            while (newHandAngle < oldHandAngle) // correct old angle to force clockwise rotation
            {
                oldHandAngle -= (2 * PI);
            }
            hourHand.updateZAngle(oldHandAngle);
        }
        else if (diff < 0 && newHandAngle > oldHandAngle)
        {
            while (newHandAngle > oldHandAngle) // correct old angle to force counterclockwise rotation
            {
                oldHandAngle += (2 * PI);
            }
            hourHand.updateZAngle(oldHandAngle);
        }

        if (animationEnabled() && !hourHand.isTextureMapperAnimationRunning())
        {
            hourHand.setupAnimation(AnimationTextureMapper::Z_ROTATION, newHandAngle, animationDuration, 0, animationEquation);
            hourHand.startAnimation();
        }
        else
        {
            if (animationEnabled())
            {
                hourHand.cancelAnimationTextureMapperAnimation();
            }
            hourHand.updateZAngle(newHandAngle);
        }
    }

    // Move minute hand
    if (minuteHand.isVisible() && ((currentMinute != lastMinute) || (minuteHandSecondCorrectionActive && (currentSecond != lastSecond))))
    {
        newHandAngle = convertHandValueToAngle(60, currentMinute, minuteHandSecondCorrectionActive ? currentSecond : 0);

        // check if sign of absolute angles difference matches sign of normalized difference of angles
        oldHandAngle = minuteHand.getZAngle();
        diff = (((int16_t)currentMinute - (int16_t)lastMinute + 60 + 30) % 60) - 30;
        if (diff > 0 && newHandAngle < oldHandAngle)
        {
            while (newHandAngle < oldHandAngle) // correct old angle to force clockwise rotation
            {
                oldHandAngle -= (2 * PI);
            }
            minuteHand.updateZAngle(oldHandAngle);
        }
        else if (diff < 0 && newHandAngle > oldHandAngle)
        {
            while (newHandAngle > oldHandAngle) // correct old angle to force counterclockwise rotation
            {
                oldHandAngle += (2 * PI);
            }
            minuteHand.updateZAngle(oldHandAngle);
        }

        if (animationEnabled() && !minuteHand.isTextureMapperAnimationRunning())
        {
            minuteHand.setupAnimation(AnimationTextureMapper::Z_ROTATION, newHandAngle, animationDuration, 0, animationEquation);
            minuteHand.startAnimation();
        }
        else
        {
            if (animationEnabled())
            {
                minuteHand.cancelAnimationTextureMapperAnimation();
            }
            minuteHand.updateZAngle(newHandAngle);
        }
    }

    // Move second hand
    if (secondHand.isVisible() && (currentSecond != lastSecond))
    {
        newHandAngle = convertHandValueToAngle(60, currentSecond);

        // check if sign of absolute angles difference matches sign of normalized difference of angles
        oldHandAngle = secondHand.getZAngle();
        diff = (((int16_t)currentSecond - (int16_t)lastSecond + 60 + 30) % 60) - 30;
        if (diff > 0 && newHandAngle < oldHandAngle)
        {
            while (newHandAngle < oldHandAngle) // correct old angle to force clockwise rotation
            {
                oldHandAngle -= (2 * PI);
            }
            secondHand.updateZAngle(oldHandAngle);
        }
        else if (diff < 0 && newHandAngle > oldHandAngle)
        {
            while (newHandAngle > oldHandAngle) // correct old angle to force counterclockwise rotation
            {
                oldHandAngle += (2 * PI);
            }
            secondHand.updateZAngle(oldHandAngle);
        }

        if (animationEnabled() && !secondHand.isTextureMapperAnimationRunning())
        {
            secondHand.setupAnimation(AnimationTextureMapper::Z_ROTATION, newHandAngle, animationDuration, 0, animationEquation);
            secondHand.startAnimation();
        }
        else
        {
            if (animationEnabled())
            {
                secondHand.cancelAnimationTextureMapperAnimation();
            }
            secondHand.updateZAngle(newHandAngle);
        }
    }

    lastHour = currentHour;
    lastMinute = currentMinute;
    lastSecond = currentSecond;
}

float AnalogClock::convertHandValueToAngle(uint8_t steps, uint8_t handValue, uint8_t secondHandValue /*= 0*/) const
{
    return ((handValue / (float)steps) + (secondHandValue / (steps * 60.f))) * 2 * PI;
}

void AnalogClock::setHourHandMinuteCorrection(bool active)
{
    hourHandMinuteCorrectionActive = active;
    setTime24Hour(getCurrentHour(), getCurrentMinute(), getCurrentSecond());
}

bool AnalogClock::getHourHandMinuteCorrection() const
{
    return hourHandMinuteCorrectionActive;
}

void AnalogClock::setMinuteHandSecondCorrection(bool active)
{
    minuteHandSecondCorrectionActive = active;
    setTime24Hour(getCurrentHour(), getCurrentMinute(), getCurrentSecond());
}

bool AnalogClock::getMinuteHandSecondCorrection() const
{
    return minuteHandSecondCorrectionActive;
}

bool AnalogClock::animationEnabled() const
{
    return animationDuration > 1;
}

void AnalogClock::setAnimation(uint16_t duration, EasingEquation animationProgressionEquation)
{
    animationDuration = duration;
    animationEquation = animationProgressionEquation;
}
} // namespace touchgfx
