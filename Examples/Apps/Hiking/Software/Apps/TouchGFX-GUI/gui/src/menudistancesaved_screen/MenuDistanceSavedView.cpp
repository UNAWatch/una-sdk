#include <gui/menudistancesaved_screen/MenuDistanceSavedView.hpp>

static constexpr uint16_t kDismissTicks = SDK::Utils::secToTicks(1, App::Config::kFrameRate);

MenuDistanceSavedView::MenuDistanceSavedView()
    : mDismissCb(this, &MenuDistanceSavedView::onDismiss)
{
}

void MenuDistanceSavedView::setupScreen()
{
    MenuDistanceSavedViewBase::setupScreen();

    title.set(T_TEXT_DISTANCE_UC);

    mDismissTimer.setDuration(kDismissTicks);
    mDismissTimer.setCallback(mDismissCb);
    mDismissTimer.start();
}

void MenuDistanceSavedView::tearDownScreen()
{
    mDismissTimer.stop();
    MenuDistanceSavedViewBase::tearDownScreen();
}

void MenuDistanceSavedView::setDistanceUnits(Menu::Id id, bool isImperial)
{
    if (id == Menu::ID_OFF) {
        Unicode::snprintf(messageTextBuffer, MESSAGETEXT_SIZE, "%s", touchgfx::TypedText(T_TEXT_OFF_UC).getText());
    } else {
        uint16_t val = Menu::kValues[id];
        if (isImperial) {
            touchgfx::TypedTextId txt = val > 1 ? T_TEXT_MILES : T_TEXT_MILE;
            Unicode::snprintf(messageTextBuffer, MESSAGETEXT_SIZE, "%d %s", val, touchgfx::TypedText(txt).getText());
        } else {
            Unicode::snprintf(messageTextBuffer, MESSAGETEXT_SIZE, "%d %s", val, touchgfx::TypedText(T_TEXT_KM).getText());
        }
    }
    messageText.invalidate();
}

void MenuDistanceSavedView::onDismiss()
{
    application().gotoMenuAlertsScreenNoTransition();
}
