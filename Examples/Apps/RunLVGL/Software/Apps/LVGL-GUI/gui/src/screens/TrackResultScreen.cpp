/**
 ******************************************************************************
 * @file    TrackResultScreen.cpp
 * @brief   "Saved" / "Discarded" confirmation shown for two seconds.
 ******************************************************************************
 */

#include "gui/screens/TrackResultScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"
#include "gui/Strings.hpp"

namespace
{
constexpr uint32_t kDismissMs = 2000;
} // namespace

TrackResultScreen::TrackResultScreen(Model& model, Result result)
    : Screen(model)
    , mResult(result)
{
}

TrackResultScreen::~TrackResultScreen()
{
    if (mDismiss) {
        lv_timer_delete(mDismiss);
    }
}

void TrackResultScreen::build()
{
    const bool saved = mResult == Result::Saved;
    Theme::label(mRoot, Theme::Font::SemiBold30, saved ? "Saved" : "Discarded", 41, 47, 159);
    Theme::imageTinted(mRoot, saved ? &img_circletick_50x50 : &img_circlecross_50x50, 95, 95,
                       SDK::GUI::Color::YELLOW_DARK);
    Theme::label(mRoot, Theme::Font::Medium18,
                 saved ? "Activity has\nbeen saved" : "Activity has\nbeen deleted", 48, 156, 144);
    mTitle = std::make_unique<Widgets::Title>(mRoot, Strings::kAppNameUc);
}

void TrackResultScreen::onShow()
{
    if (mResult == Result::Saved) {
        mModel.saveTrack();
    } else {
        mModel.resetIdleTimer();
        mModel.discardTrack();
    }
    mDismiss = lv_timer_create(&TrackResultScreen::dismissCb, kDismissMs, this);
    lv_timer_set_repeat_count(mDismiss, 1);
}

void TrackResultScreen::dismissCb(lv_timer_t* t)
{
    auto* self = static_cast<TrackResultScreen*>(lv_timer_get_user_data(t));
    self->mDismiss = nullptr;   // a one-shot timer deletes itself after this call
    if (self->mResult == Result::Saved) {
        ScreenManager::instance().goTo(ScreenId::TrackSummary);
    } else {
        self->mModel.exitApp();
    }
}
