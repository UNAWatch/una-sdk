#include <gui/menutimesaved_screen/MenuTimeSavedView.hpp>


static constexpr uint16_t kDismissTicks = SDK::Utils::secToTicks(1, App::Config::kFrameRate);

MenuTimeSavedView::MenuTimeSavedView()
    : mDismissCb(this, &MenuTimeSavedView::onDismiss)
{
}

void MenuTimeSavedView::setupScreen()
{
    MenuTimeSavedViewBase::setupScreen();

    title.set(T_TEXT_TIME_UC);

    mDismissTimer.setDuration(kDismissTicks);
    mDismissTimer.setCallback(mDismissCb);
    mDismissTimer.start();
}

void MenuTimeSavedView::tearDownScreen()
{
    mDismissTimer.stop();
    MenuTimeSavedViewBase::tearDownScreen();
}

void MenuTimeSavedView::setTime(Menu::Id id)
{
    if (id == Menu::ID_OFF) {
        Unicode::snprintf(messageTextBuffer, MESSAGETEXT_SIZE, "%s", touchgfx::TypedText(T_TEXT_OFF_UC).getText());
    } else {
        Unicode::snprintf(messageTextBuffer, MESSAGETEXT_SIZE, "%d %s",
            Menu::kValues[id], touchgfx::TypedText(T_TEXT_MIN).getText());
    }
    messageText.invalidate();
}

void MenuTimeSavedView::onDismiss()
{
    application().gotoMenuAlertsScreenNoTransition();
}
