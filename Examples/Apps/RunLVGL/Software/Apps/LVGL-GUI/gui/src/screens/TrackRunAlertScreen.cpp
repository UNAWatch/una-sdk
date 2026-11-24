/**
 ******************************************************************************
 * @file    TrackRunAlertScreen.cpp
 * @brief   Run alert pop-up (see TrackRunAlertScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/TrackRunAlertScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{
constexpr uint32_t kDismissMs = 3000;
} // namespace

TrackRunAlertScreen::TrackRunAlertScreen(Model& model)
    : Screen(model)
{
}

TrackRunAlertScreen::~TrackRunAlertScreen()
{
    if (mDismiss) {
        lv_timer_delete(mDismiss);
    }
}

void TrackRunAlertScreen::build()
{
    using F = Theme::Font;
    // Same layout as TrackRunAlertView in the TouchGFX app.
    mValue = Theme::label(mRoot, F::SemiBold40, "", 0, 86, 240);
    mUnit  = Theme::label(mRoot, F::Regular18, "", 0, 132, 240);
    Theme::hline(mRoot, 70, 164, 100);
    mTitle = std::make_unique<Widgets::Title>(mRoot, "");
}

void TrackRunAlertScreen::onShow()
{
    mModel.resetIdleTimer();

    const Model::RunAlertInfo& a = mModel.getRunAlert();
    const bool imperial = mModel.isUnitsImperial();
    char buf[16];

    mTitle->setText(RunAlerts::alertTitle(a.kind));
    RunAlerts::alertValueText(a.kind, a.value, imperial, buf, sizeof(buf));
    lv_label_set_text(mValue, buf);
    lv_label_set_text(mUnit, RunAlerts::alertUnit(a.kind, imperial));

    mDismiss = lv_timer_create(&TrackRunAlertScreen::dismissCb, kDismissMs, this);
    lv_timer_set_repeat_count(mDismiss, 1);
}

void TrackRunAlertScreen::dismissCb(lv_timer_t* t)
{
    auto* self = static_cast<TrackRunAlertScreen*>(lv_timer_get_user_data(t));
    self->mDismiss = nullptr;   // one-shot: LVGL deletes the timer after this call
    ScreenManager::instance().goTo(ScreenId::Track);
}
