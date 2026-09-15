/**
 ******************************************************************************
 * @file    WheelMenu.hpp
 * @brief   The SDK's scroll-wheel menu in the Run app's faces and timing.
 *
 * SDK::LVGL::WheelMenu takes its fonts and slide time from the app; this
 * class supplies Run's (SemiBold 30 selected, Medium 18 around it, Italic 18
 * hints, App::Config::kMenuAnimationMs) so the screens create a menu from
 * their item table alone. Items may still override the selected face.
 ******************************************************************************
 */

#ifndef WHEEL_MENU_HPP
#define WHEEL_MENU_HPP

#include <cstdint>

#include "lvgl.h"

#include "SDK/GUI/LVGL/WheelMenu.hpp"

class WheelMenu : public SDK::LVGL::WheelMenu
{
public:
    /**
     * @param items        Menu entries, kept alive (and possibly edited) by the caller.
     * @param count        Number of entries.
     * @param itemOffsetY  Vertical nudge of a Simple surrounding item's text.
     */
    WheelMenu(lv_obj_t* parent, const Item* items, uint16_t count, int16_t itemOffsetY = 0);
};

#endif // WHEEL_MENU_HPP
