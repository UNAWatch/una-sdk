/**
 ******************************************************************************
 * @file    TimerRing.cpp
 * @brief   Progress ring around the face.
 ******************************************************************************
 */

#include "SDK/GUI/LVGL/TimerRing.hpp"

#include "SDK/GUI/LVGL/Draw.hpp"

namespace SDK::LVGL
{

namespace
{
namespace Color = SDK::GUI::Color;

constexpr int32_t kRingRadius = 112;
constexpr int32_t kRingWidth  = 9;
constexpr int32_t kRingStart  = 138;   // TouchGFX angles: 138 -> 402 (264 degrees)
constexpr int32_t kRingSpan   = 264;

int32_t spanFor(uint32_t permille)
{
    if (permille > 1000) {
        permille = 1000;
    }
    return static_cast<int32_t>((kRingSpan * permille + 500) / 1000);
}
} // namespace

TimerRing::TimerRing(lv_obj_t* parent)
{
    mTrack    = Draw::arc(parent, kCx, kCy, kRingRadius, kRingWidth,
                          kRingStart, kRingStart + kRingSpan, Color::GRAY_DARK);
    mProgress = Draw::arc(parent, kCx, kCy, kRingRadius, kRingWidth,
                          kRingStart, kRingStart + 1, Color::YELLOW_DARK);
    setProgress(0);
}

void TimerRing::setColor(uint32_t color)
{
    Draw::setArcColor(mProgress, color);
}

void TimerRing::setProgress(uint32_t permille)
{
    const int32_t span = spanFor(permille);
    if (span <= 0) {
        lv_obj_add_flag(mProgress, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    Draw::setArc(mProgress, kRingStart, kRingStart + span);
    lv_obj_remove_flag(mProgress, LV_OBJ_FLAG_HIDDEN);
}

void TimerRing::setRemaining(uint32_t permille)
{
    const int32_t span = spanFor(permille);
    if (span <= 0) {
        lv_obj_add_flag(mProgress, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    // The end stays at the track's end; the start advances as time drains.
    Draw::setArc(mProgress, kRingStart + kRingSpan - span, kRingStart + kRingSpan);
    lv_obj_remove_flag(mProgress, LV_OBJ_FLAG_HIDDEN);
}

} // namespace SDK::LVGL
