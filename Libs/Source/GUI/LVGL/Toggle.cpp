/**
 ******************************************************************************
 * @file    Toggle.cpp
 * @brief   Two-state switch, 60 x 30.
 ******************************************************************************
 */

#include "SDK/GUI/LVGL/Toggle.hpp"

#include "SDK/GUI/LVGL/Draw.hpp"

namespace SDK::LVGL
{

namespace Color = SDK::GUI::Color;

Toggle::Toggle(lv_obj_t* parent, int32_t x, int32_t y)
    : mX(x), mY(y)
{
    // TouchGFX: a 30 px round-capped line from (15,15) to (45,15) -> a 60 x 30 pill.
    mRail   = Draw::box(parent, x, y, 60, 30, Color::BLACK, LV_RADIUS_CIRCLE);
    mHandle = Draw::dot(parent, x + 15, y + 15, 15, Color::WHITE);
    setState(false);
}

void Toggle::setState(bool on)
{
    mOn = on;
    lv_obj_set_style_bg_color(mRail, Draw::rgb(on ? Color::YELLOW_DARK : Color::BLACK), LV_PART_MAIN);
    lv_obj_set_pos(mHandle, mX + (on ? 30 : 0), mY);
}

void Toggle::setVisible(bool visible)
{
    Draw::setHidden(mRail, !visible);
    Draw::setHidden(mHandle, !visible);
}

} // namespace SDK::LVGL
