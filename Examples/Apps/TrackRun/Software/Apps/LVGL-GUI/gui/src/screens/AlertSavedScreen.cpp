/**
 ******************************************************************************
 * @file    AlertSavedScreen.cpp
 * @brief   "Saved" confirmation for an auto-lap value (see the header).
 ******************************************************************************
 */

#include "gui/screens/AlertSavedScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"
#include "gui/Format.hpp"

namespace
{
constexpr uint32_t kDismissMs = 1000;
} // namespace

AlertSavedScreen::AlertSavedScreen(Model& model, Kind kind)
    : Screen(model)
    , mKind(kind)
{
}

AlertSavedScreen::~AlertSavedScreen()
{
    if (mDismiss) {
        lv_timer_delete(mDismiss);
    }
}

void AlertSavedScreen::build()
{
    Theme::label(mRoot, Theme::Font::SemiBold30, "Saved", 0, 150, 240);
    Theme::imageTinted(mRoot, &img_circletick_50x50, 95, 95, SDK::GUI::Color::YELLOW_DARK);
    mMessage = Theme::label(mRoot, Theme::Font::Medium25, "", 60, 53, 120);
    mTitle = std::make_unique<Widgets::Title>(mRoot, mKind == Kind::Distance ? "DISTANCE" : "TIME");
}

void AlertSavedScreen::onShow()
{
    char buf[16];
    const Settings& s = mModel.getSettings();
    if (mKind == Kind::Distance) {
        Fmt::alertDistance(buf, sizeof(buf), s.alertDistanceId, mModel.isUnitsImperial());
    } else {
        Fmt::alertTime(buf, sizeof(buf), s.alertTimeId, false);
    }
    lv_label_set_text(mMessage, buf);

    mDismiss = lv_timer_create(&AlertSavedScreen::dismissCb, kDismissMs, this);
    lv_timer_set_repeat_count(mDismiss, 1);
}

void AlertSavedScreen::dismissCb(lv_timer_t* t)
{
    auto* self = static_cast<AlertSavedScreen*>(lv_timer_get_user_data(t));
    self->mDismiss = nullptr;   // one-shot: LVGL deletes the timer after this call
    ScreenManager::instance().goTo(ScreenId::MenuAlerts);
}
