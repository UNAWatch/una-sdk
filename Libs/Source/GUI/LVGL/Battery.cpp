/**
 ******************************************************************************
 * @file    Battery.cpp
 * @brief   Four-segment battery gauge, 90 x 28.
 ******************************************************************************
 */

#include "SDK/GUI/LVGL/Battery.hpp"

#include "SDK/GUI/LVGL/Draw.hpp"

namespace SDK::LVGL
{

namespace Color = SDK::GUI::Color;

Battery::Battery(lv_obj_t* parent, int32_t x, int32_t y)
{
    // Segment spans taken from the TouchGFX design (box plus round-cap edges).
    mSeg[0] = Draw::box(parent, x + 0,  y, 20, 28, Color::GRAY_DARK, 5);
    mSeg[1] = Draw::box(parent, x + 22, y, 20, 28, Color::GRAY_DARK, 3);
    mSeg[2] = Draw::box(parent, x + 44, y, 20, 28, Color::GRAY_DARK, 3);
    mSeg[3] = Draw::box(parent, x + 66, y, 20, 28, Color::GRAY_DARK, 5);
    mNub    = Draw::box(parent, x + 82, y + 6, 8, 16, Color::GRAY_DARK, 4);
    setLevel(0);
}

void Battery::setLevel(uint8_t level)
{
    const uint32_t c1 = (level == 0) ? Color::GRAY_DARK : (level < 25) ? Color::RED : Color::TEAL;
    const uint32_t c2 = level >= 25 ? Color::TEAL : Color::GRAY_DARK;
    const uint32_t c3 = level >= 50 ? Color::TEAL : Color::GRAY_DARK;
    const uint32_t c4 = level >= 75 ? Color::TEAL : Color::GRAY_DARK;
    lv_obj_set_style_bg_color(mSeg[0], Draw::rgb(c1), LV_PART_MAIN);
    lv_obj_set_style_bg_color(mSeg[1], Draw::rgb(c2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(mSeg[2], Draw::rgb(c3), LV_PART_MAIN);
    lv_obj_set_style_bg_color(mSeg[3], Draw::rgb(c4), LV_PART_MAIN);
    lv_obj_set_style_bg_color(mNub,    Draw::rgb(c4), LV_PART_MAIN);
}

} // namespace SDK::LVGL
