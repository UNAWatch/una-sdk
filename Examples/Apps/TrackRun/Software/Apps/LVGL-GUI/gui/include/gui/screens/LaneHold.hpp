/**
 ******************************************************************************
 * @file    LaneHold.hpp
 * @brief   Settings > Lane shortcut: a hold of Lap (R2) on a run screen (the
 *          run screen, the lap flash, a moment such as the track notice)
 *          opens the lane picker without pausing.
 *
 * With the setting on, the press starts a one-shot timer and the
 * release stops it; at kHoldMs, with Lap still down, the lane picker
 * (LaneScreen) opens and goes back to the run screen. The watch sends a
 * click only for a press under 500 ms, so a hold is never also a lap; the
 * release that follows lands on the picker, which ignores it.
 ******************************************************************************
 */

#ifndef LANE_HOLD_HPP
#define LANE_HOLD_HPP

#include <cstdint>

#include "lvgl.h"

class Model;

class LaneHold
{
public:
    static constexpr uint32_t kHoldMs = 800;

    explicit LaneHold(Model& model) : mModel(model) {}
    ~LaneHold() { stop(); }

    LaneHold(const LaneHold&)            = delete;
    LaneHold& operator=(const LaneHold&) = delete;

    /// A screen's key codes: the R2 press and release are read, the rest ignored.
    void onKey(uint8_t code);
    void stop();

private:
    static void fireCb(lv_timer_t* t);

    Model&      mModel;
    lv_timer_t* mTimer = nullptr;
};

#endif // LANE_HOLD_HPP
