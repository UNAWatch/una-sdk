/**
 ******************************************************************************
 * @file    WheelMenu.cpp
 * @brief   The SDK's scroll-wheel menu in the Run app's faces and timing.
 ******************************************************************************
 */

#include "gui/widgets/WheelMenu.hpp"

#include "gui/model/Model.hpp"
#include "gui/theme/Theme.hpp"

namespace
{
SDK::LVGL::WheelMenu::Fonts runFonts()
{
    return { Theme::font(Theme::Font::SemiBold30),
             Theme::font(Theme::Font::Medium18),
             Theme::font(Theme::Font::Italic18) };
}
} // namespace

WheelMenu::WheelMenu(lv_obj_t* parent, const Item* items, uint16_t count, int16_t itemOffsetY)
    : SDK::LVGL::WheelMenu(parent, items, count, runFonts(), itemOffsetY)
{
    setAnimationMs(App::Config::kMenuAnimationMs);
}
