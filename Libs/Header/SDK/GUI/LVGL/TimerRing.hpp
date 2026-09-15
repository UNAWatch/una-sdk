/**
 ******************************************************************************
 * @file    TimerRing.hpp
 * @brief   Progress ring around the face: a grey track with a coloured fill,
 *          as the UNA activity apps draw around a countdown or a hold-to-
 *          confirm prompt.
 ******************************************************************************
 */

#ifndef SDK_GUI_LVGL_TIMER_RING_HPP
#define SDK_GUI_LVGL_TIMER_RING_HPP

#include <cstdint>

#include "lvgl.h"

namespace SDK::LVGL
{

class TimerRing
{
public:
    explicit TimerRing(lv_obj_t* parent);
    void setColor(uint32_t color);
    /// FILL: the arc grows from the track's start; @p permille (0..1000) is how much.
    void setProgress(uint32_t permille);
    /// DRAIN: the arc shrinks towards the track's end; @p permille is how much is left.
    void setRemaining(uint32_t permille);

private:
    lv_obj_t* mTrack    = nullptr;
    lv_obj_t* mProgress = nullptr;
};

} // namespace SDK::LVGL

#endif // SDK_GUI_LVGL_TIMER_RING_HPP
