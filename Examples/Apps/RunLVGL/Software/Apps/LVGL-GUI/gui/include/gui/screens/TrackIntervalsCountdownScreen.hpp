/**
 ******************************************************************************
 * @file    TrackIntervalsCountdownScreen.hpp
 * @brief   Five-second countdown before an intervals workout, summarising the
 *          configured repeats, run and rest. R1 starts now, R2 cancels; the
 *          ring drains and the workout starts when it empties.
 ******************************************************************************
 */

#ifndef TRACK_INTERVALS_COUNTDOWN_SCREEN_HPP
#define TRACK_INTERVALS_COUNTDOWN_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackIntervalsCountdownScreen : public Screen
{
public:
    explicit TrackIntervalsCountdownScreen(Model& model);
    ~TrackIntervalsCountdownScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    static constexpr uint32_t kTimeoutMs = 5000;

    static void animExecCb(void* var, int32_t value);
    static void animReadyCb(lv_anim_t* a);
    void startTrack();

    bool     mStarted = false;
    uint32_t mShownSeconds = 0;

    lv_obj_t* mReps  = nullptr;
    lv_obj_t* mRun   = nullptr;
    lv_obj_t* mRest  = nullptr;
    lv_obj_t* mCount = nullptr;
    std::unique_ptr<Widgets::TimerRing> mRing;
    std::unique_ptr<Widgets::Buttons>   mButtons;
};

#endif // TRACK_INTERVALS_COUNTDOWN_SCREEN_HPP
