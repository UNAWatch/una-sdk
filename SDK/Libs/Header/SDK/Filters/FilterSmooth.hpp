/**
 ******************************************************************************
 * @file    FilterSmooth.hpp
 * @date    09-October-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Smooth filter
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef SDK_FILTER_SMOOTH_H
#define SDK_FILTER_SMOOTH_H

#include <cstddef>

namespace SDK::Filter {

class Smooth {
public:
    explicit Smooth(float alpha = 0.5f);

    void  reset();
    float execute(float value);
    float getValue() const;
    void  setAlpha(float alpha);
    float getAlpha() const;

private:
    bool  mCleared;
    float mValue;
    float mAlpha;
};

} // namespace SDK::Filter

#endif // SDK_FILTER_SMOOTH_H
