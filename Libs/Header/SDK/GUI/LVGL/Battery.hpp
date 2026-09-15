/**
 ******************************************************************************
 * @file    Battery.hpp
 * @brief   Four-segment battery gauge, 90 x 28, as the UNA activity apps
 *          show on their status face.
 ******************************************************************************
 */

#ifndef SDK_GUI_LVGL_BATTERY_HPP
#define SDK_GUI_LVGL_BATTERY_HPP

#include <cstdint>

#include "lvgl.h"

namespace SDK::LVGL
{

class Battery
{
public:
    Battery(lv_obj_t* parent, int32_t x, int32_t y);
    /// Segments light at 25 % steps. The first is red from 1 to 24 % and
    /// grey at 0 %, as in the TouchGFX gauge.
    void setLevel(uint8_t level);

private:
    lv_obj_t* mSeg[4] = {};
    lv_obj_t* mNub    = nullptr;
};

} // namespace SDK::LVGL

#endif // SDK_GUI_LVGL_BATTERY_HPP
