/**
 ******************************************************************************
 * @file    TrackNoticeScreen.hpp
 * @brief   The track notice over the run screen, as the lap pop-up: a known
 *          track the run joined by itself ("Track 2", "Lane 3"), or a new
 *          track's calibration locked ("Track locked", "Saves as Track 3").
 *          It goes back to the run after kDismissMs; a hold of Lap opens the
 *          lane picker (Settings > Lane shortcut).
 ******************************************************************************
 */

#ifndef TRACK_NOTICE_SCREEN_HPP
#define TRACK_NOTICE_SCREEN_HPP

#include <memory>

#include "gui/screens/LaneHold.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackNoticeScreen : public Screen
{
public:
    static constexpr uint32_t kDismissMs = 4000;

    explicit TrackNoticeScreen(Model& model);
    ~TrackNoticeScreen() override;

    void onShow() override;
    void onKey(uint8_t code) override;
    /// A hold of Lap under way stops: its release cannot come while suspended.
    void onSuspend() override { mLaneHold.stop(); }

protected:
    void build() override;

private:
    static void dismissCb(lv_timer_t* t);

    lv_timer_t* mDismiss = nullptr;
    LaneHold    mLaneHold;
    std::unique_ptr<Widgets::Title> mTitle;
};

#endif // TRACK_NOTICE_SCREEN_HPP
