#include <gui/menurunalertsaved_screen/MenuRunAlertSavedView.hpp>
#include <gui/common/RunAlertsUi.hpp>

static constexpr uint16_t kDismissTicks = SDK::Utils::secToTicks(1, App::Config::kFrameRate);

MenuRunAlertSavedView::MenuRunAlertSavedView()
    : mDismissCb(this, &MenuRunAlertSavedView::onDismiss)
{
}

void MenuRunAlertSavedView::setupScreen()
{
    MenuRunAlertSavedViewBase::setupScreen();

    mDismissTimer.setDuration(kDismissTicks);
    mDismissTimer.setCallback(mDismissCb);
    mDismissTimer.start();
}

void MenuRunAlertSavedView::tearDownScreen()
{
    mDismissTimer.stop();
    MenuRunAlertSavedViewBase::tearDownScreen();
}

void MenuRunAlertSavedView::setSaved(RunAlerts::Field field, const char* value)
{
    mField = field;
    title.set(RunAlertsUi::title(field));
    Unicode::strncpy(messageTextBuffer, value, MESSAGETEXT_SIZE - 1);
    messageTextBuffer[MESSAGETEXT_SIZE - 1] = 0;
    messageText.invalidate();
}

void MenuRunAlertSavedView::onDismiss()
{
    if (RunAlerts::isPace(mField) || RunAlerts::isHr(mField)) {
        application().gotoMenuRunLimitsScreenNoTransition();
    } else {
        application().gotoMenuRunWalkScreenNoTransition();
    }
}
