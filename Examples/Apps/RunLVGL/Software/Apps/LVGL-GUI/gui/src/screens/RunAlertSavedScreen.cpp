/**
 ******************************************************************************
 * @file    RunAlertSavedScreen.cpp
 * @brief   Run Alerts "Saved" confirmation (see RunAlertSavedScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/RunAlertSavedScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"

namespace
{
constexpr uint32_t kDismissMs = 1000;
} // namespace

RunAlertSavedScreen::RunAlertSavedScreen(Model& model)
    : Screen(model)
{
}

RunAlertSavedScreen::~RunAlertSavedScreen()
{
    if (mDismiss) {
        lv_timer_delete(mDismiss);
    }
}

void RunAlertSavedScreen::build()
{
    Theme::label(mRoot, Theme::Font::SemiBold30, "Saved", 0, 150, 240);
    Theme::imageTinted(mRoot, &img_circletick_50x50, 95, 95, SDK::GUI::Color::YELLOW_DARK);
    // Wider than the Lap Alerts value: "04:00 min" is the longest.
    mMessage = Theme::label(mRoot, Theme::Font::Medium25, "", 30, 53, 180);
    mTitle = std::make_unique<Widgets::Title>(mRoot, RunAlerts::fieldTitle(mModel.runAlertField()));
}

void RunAlertSavedScreen::onShow()
{
    char buf[16];
    RunAlerts::fieldText(mModel.runAlertField(), mModel.getSettings().runAlerts, mModel.isUnitsImperial(),
                         buf, sizeof(buf));
    lv_label_set_text(mMessage, buf);

    mDismiss = lv_timer_create(&RunAlertSavedScreen::dismissCb, kDismissMs, this);
    lv_timer_set_repeat_count(mDismiss, 1);
}

void RunAlertSavedScreen::dismissCb(lv_timer_t* t)
{
    auto* self = static_cast<RunAlertSavedScreen*>(lv_timer_get_user_data(t));
    self->mDismiss = nullptr;   // one-shot: LVGL deletes the timer after this call
    const RunAlerts::Field f = self->mModel.runAlertField();
    ScreenManager::instance().goTo(RunAlerts::isPace(f) ? ScreenId::MenuRunPace
                                   : RunAlerts::isHr(f) ? ScreenId::MenuRunHr
                                                        : ScreenId::MenuRunWalk);
}
