/**
 ******************************************************************************
 * @file    Buttons.cpp
 * @brief   Bezel arcs showing which of the watch's four buttons do something.
 ******************************************************************************
 */

#include "SDK/GUI/LVGL/Buttons.hpp"

#include "SDK/GUI/LVGL/Draw.hpp"

namespace SDK::LVGL
{

namespace
{
namespace Color = SDK::GUI::Color;

constexpr int32_t kButtonRadius = 113;
constexpr int32_t kButtonWidth  = 6;
// Arc bounds per button (TouchGFX angles, 0 = 12 o'clock): L1, L2, R1, R2.
// R2's upper end is drawn one degree short of the activity apps' 111: LVGL's
// rounded cap there rendered with a kink towards the screen centre, and the
// TouchGFX cap ends a pixel earlier.
constexpr int32_t kButtonArcs[4][2] = { { 291, 308 }, { 232, 249 }, { 52, 69 }, { 112, 128 } };

uint32_t buttonColor(Buttons::Color c)
{
    switch (c) {
        case Buttons::WHITE: return Color::WHITE;
        case Buttons::AMBER: return Color::YELLOW_DARK;   // C0 80 00
        case Buttons::RED:   return Color::RED;
        case Buttons::GREEN: return Color::CHARTREUSE;    // 40 C0 00
        default:             return Color::BLACK;
    }
}
} // namespace

Buttons::Buttons(lv_obj_t* parent)
{
    for (int i = 0; i < 4; ++i) {
        mArc[i] = Draw::arc(parent, kCx, kCy, kButtonRadius, kButtonWidth,
                            kButtonArcs[i][0], kButtonArcs[i][1], Color::WHITE);
        lv_obj_add_flag(mArc[i], LV_OBJ_FLAG_HIDDEN);
    }
}

void Buttons::set(Color l1, Color l2, Color r1, Color r2)
{
    apply(0, l1);
    apply(1, l2);
    apply(2, r1);
    apply(3, r2);
}

void Buttons::apply(int idx, Color c)
{
    if (c == NONE) {
        lv_obj_add_flag(mArc[idx], LV_OBJ_FLAG_HIDDEN);
        return;
    }
    Draw::setArcColor(mArc[idx], buttonColor(c));
    lv_obj_remove_flag(mArc[idx], LV_OBJ_FLAG_HIDDEN);
}

} // namespace SDK::LVGL
