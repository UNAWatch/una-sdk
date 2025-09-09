/**
 ******************************************************************************
 * @file    FilterSmooth.cpp
 * @date    09-October-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Smooth filter
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/Filters/FilterSmooth.hpp"

namespace SDK::Filter {

/**
 * @brief Construct a new Smooth filter.
 * @param alpha Smoothing factor (0 < alpha < 1).
 */
Smooth::Smooth(float alpha)
    : mCleared(true)
    , mValue(0.0f)
    , mAlpha(0.5f)
{
    setAlpha(alpha);
}

/**
 * @brief Reset the filter state.
 *
 * After reset, the first input value is taken directly as output.
 */
void Smooth::reset()
{
    mCleared = true;
}

/**
 * @brief Process a new sample.
 * @param value Input sample.
 * @return Smoothed output value.
 *
 * Formula:
 * @f[
 * y_{k} = \alpha \cdot y_{k-1} + (1 - \alpha) \cdot x_{k}
 * @f]
 */
float Smooth::execute(float value)
{
    if (mCleared) {
        mValue   = value;
        mCleared = false;
    } else {
        mValue = mValue * mAlpha + value * (1.0f - mAlpha);
    }
    return mValue;
}

/**
 * @brief Get the current filter output.
 * @return Last smoothed value.
 */
float Smooth::getValue() const
{
    return mValue;
}

/**
 * @brief Set smoothing factor alpha.
 * @param alpha New alpha (must be 0 < alpha < 1).
 */
void Smooth::setAlpha(float alpha)
{
    if (alpha > 0.0f && alpha < 1.0f) {
        mAlpha = alpha;
    }
}

/**
 * @brief Get the smoothing factor alpha.
 * @return Current alpha.
 */
float Smooth::getAlpha() const
{
    return mAlpha;
}

} // namespace SDK::Filter
