/**
 ******************************************************************************
 * @file    Toggle.hpp
 * @brief   Two-state switch, 60 x 30: a pill rail (black off, amber on) and
 *          a white knob, as the UNA activity apps draw in their menus.
 ******************************************************************************
 */

#ifndef SDK_GUI_LVGL_TOGGLE_HPP
#define SDK_GUI_LVGL_TOGGLE_HPP

#include <cstdint>

#include "lvgl.h"

namespace SDK::LVGL
{

class Toggle
{
public:
    Toggle(lv_obj_t* parent, int32_t x, int32_t y);
    void setState(bool on);
    bool state() const { return mOn; }
    void setVisible(bool visible);

private:
    lv_obj_t* mRail   = nullptr;
    lv_obj_t* mHandle = nullptr;
    int32_t   mX = 0, mY = 0;
    bool      mOn = false;
};

} // namespace SDK::LVGL

#endif // SDK_GUI_LVGL_TOGGLE_HPP
