#include <gui/trackrunalert_screen/TrackRunAlertView.hpp>
#include <gui/common/RunAlertsUi.hpp>

static constexpr uint16_t kDismissTicks = SDK::Utils::secToTicks(3, App::Config::kFrameRate);

TrackRunAlertView::TrackRunAlertView()
    : mDismissCb(this, &TrackRunAlertView::onDismiss)
{
}

void TrackRunAlertView::setupScreen()
{
    TrackRunAlertViewBase::setupScreen();

    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::NONE);
    buttons.setR2(Buttons::NONE);

    mDismissTimer.setDuration(kDismissTicks);
    mDismissTimer.setCallback(mDismissCb);
    mDismissTimer.start();
}

void TrackRunAlertView::tearDownScreen()
{
    mDismissTimer.stop();
    TrackRunAlertViewBase::tearDownScreen();
}

void TrackRunAlertView::setAlert(Track::RunAlertKind kind, float value, bool imperial)
{
    title.set(RunAlertsUi::alertTitle(kind));

    char buf[VALUETEXT_SIZE];
    RunAlerts::alertValueText(kind, value, imperial, buf, sizeof(buf));
    Unicode::strncpy(valueTextBuffer, buf, VALUETEXT_SIZE - 1);
    valueTextBuffer[VALUETEXT_SIZE - 1] = 0;
    valueText.invalidate();

    Unicode::snprintf(unitTextBuffer, UNITTEXT_SIZE, "%s",
                      touchgfx::TypedText(RunAlertsUi::alertUnit(kind, imperial)).getText());
    unitText.invalidate();
}

void TrackRunAlertView::onDismiss()
{
    application().gotoTrackScreenNoTransition();
}
