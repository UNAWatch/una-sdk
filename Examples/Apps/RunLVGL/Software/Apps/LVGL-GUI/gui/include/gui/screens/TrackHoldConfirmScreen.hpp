/**
 ******************************************************************************
 * @file    TrackHoldConfirmScreen.hpp
 * @brief   Hold-to-confirm for ending an activity: a ring fills over 1.5 s
 *          while R1 stays held and the centre counts 3 -> 2 -> 1. Releasing
 *          early returns to the action menu; completing saves or discards.
 *
 * Port of the Run app's TrackHoldConfirmationView. The ring is driven by an
 * lv_anim, which is what TouchGFX's tick-driven TimerRing amounted to.
 ******************************************************************************
 */

#ifndef TRACK_HOLD_CONFIRM_SCREEN_HPP
#define TRACK_HOLD_CONFIRM_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackHoldConfirmScreen : public Screen
{
public:
    explicit TrackHoldConfirmScreen(Model& model);
    ~TrackHoldConfirmScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;
    void onIdleTimeout() override;
    void onSuspend() override;

protected:
    void build() override;

private:
    static constexpr uint32_t kHoldMs = 1500;

    static void animExecCb(void* var, int32_t value);
    static void animReadyCb(lv_anim_t* a);
    void setCountdown(uint32_t number);
    void cancel();

    Model::HoldConfirmMode mMode  = Model::HoldConfirmMode::Discard;
    bool                   mFired = false;
    uint32_t               mShown = 3;

    lv_obj_t* mNumber = nullptr;
    std::unique_ptr<Widgets::TimerRing> mRing;
    std::unique_ptr<Widgets::Buttons>   mButtons;
};

#endif // TRACK_HOLD_CONFIRM_SCREEN_HPP
