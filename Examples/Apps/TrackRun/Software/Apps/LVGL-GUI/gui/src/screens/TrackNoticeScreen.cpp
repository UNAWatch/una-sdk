/**
 ******************************************************************************
 * @file    TrackNoticeScreen.cpp
 * @brief   The track notice (see TrackNoticeScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/TrackNoticeScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"

#include <cstdio>

namespace
{
// Joined: the track, the lane, how to change it.
constexpr int32_t kEventY = 74;
constexpr int32_t kLine1Y = 122;
constexpr int32_t kLine2Y = 148;
// Locked: RunLVGL's circle tick over the words.
constexpr int32_t kTickY       = 46;
constexpr int32_t kLockedY     = 98;
constexpr int32_t kLockedLine1 = 136;
constexpr int32_t kLockedLine2 = 160;
} // namespace

TrackNoticeScreen::TrackNoticeScreen(Model& model)
    : Screen(model)
    , mLaneHold(model)
{
}

TrackNoticeScreen::~TrackNoticeScreen()
{
    if (mDismiss != nullptr) {
        lv_timer_delete(mDismiss);
    }
}

void TrackNoticeScreen::build()
{
    using F    = Theme::Font;
    using Kind = Model::NoticeInfo::Kind;
    const Model::NoticeInfo& n = mModel.getNotice();
    char buf[32];

    mTitle = std::make_unique<Widgets::Title>(mRoot, "TRACK");
    if (n.kind == Kind::Joined) {
        if (n.trackNumber != 0) {
            std::snprintf(buf, sizeof(buf), "Track %u", static_cast<unsigned>(n.trackNumber));
        } else {
            std::snprintf(buf, sizeof(buf), "Track");
        }
        Theme::label(mRoot, F::SemiBold30, buf, 20, kEventY, 200);
        std::snprintf(buf, sizeof(buf), "Lane %u", static_cast<unsigned>(n.lane));
        Theme::label(mRoot, F::Medium18, buf, 20, kLine1Y, 200);
        Theme::label(mRoot, F::Medium18, mModel.laneShortcut() ? "Hold Lap to change" : "Change it in Pause",
                     20, kLine2Y, 200);
    } else {
        Theme::imageTinted(mRoot, &img_circletick_50x50, 95, kTickY, SDK::GUI::Color::WHITE);
        Theme::label(mRoot, F::SemiBold30, "Track locked", 20, kLockedY, 200);
        if (n.newNumber != 0) {
            std::snprintf(buf, sizeof(buf), "Saves as Track %u", static_cast<unsigned>(n.newNumber));
        } else {
            std::snprintf(buf, sizeof(buf), "Saved with the run");
        }
        Theme::label(mRoot, F::Medium18, buf, 20, kLockedLine1, 200);
        Theme::label(mRoot, F::Medium18, "Distance from your lane", 20, kLockedLine2, 200);
    }
}

void TrackNoticeScreen::onShow()
{
    mModel.takePopup(Model::Popup::Notice);
    mModel.resetIdleTimer();
    mDismiss = lv_timer_create(&TrackNoticeScreen::dismissCb, kDismissMs, this);
    lv_timer_set_repeat_count(mDismiss, 1);
}

void TrackNoticeScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    mLaneHold.onKey(code);
    if (code == Btn::R1 || code == Btn::R2 || code == Btn::L1 || code == Btn::L2) {
        // Any click: back to the run now.
        ScreenManager::instance().goTo(ScreenId::Track);
    }
}

void TrackNoticeScreen::dismissCb(lv_timer_t* t)
{
    auto* self     = static_cast<TrackNoticeScreen*>(lv_timer_get_user_data(t));
    self->mDismiss = nullptr;   // one-shot: LVGL deletes the timer after this call
    ScreenManager::instance().goTo(ScreenId::Track);
}
