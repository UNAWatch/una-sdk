/**
 ******************************************************************************
 * @file    Title.cpp
 * @brief   Screen title at the top: text over a short grey rule.
 ******************************************************************************
 */

#include "SDK/GUI/LVGL/Title.hpp"

#include "SDK/GUI/LVGL/Draw.hpp"

namespace SDK::LVGL
{

Title::Title(lv_obj_t* parent, const lv_font_t* font, const char* text)
{
    mLine  = Draw::hline(parent, 50, 38, 140, SDK::GUI::Color::GRAY_DARK);
    mLabel = Draw::label(parent, font, text, 60, 11, 120);
}

void Title::setText(const char* text)
{
    lv_label_set_text(mLabel, text);
}

void Title::setColor(uint32_t color)
{
    lv_obj_set_style_bg_color(mLine, Draw::rgb(color), LV_PART_MAIN);
    lv_obj_set_style_text_color(mLabel, Draw::rgb(color), LV_PART_MAIN);
}

void Title::setVisible(bool visible)
{
    Draw::setHidden(mLine, !visible);
    Draw::setHidden(mLabel, !visible);
}

} // namespace SDK::LVGL
