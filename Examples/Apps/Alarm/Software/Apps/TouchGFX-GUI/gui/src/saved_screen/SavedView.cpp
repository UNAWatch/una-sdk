#include <gui/saved_screen/SavedView.hpp>
#include <texts/TextKeysAndLanguages.hpp>

static constexpr uint16_t kDismissTicks = SDK::Utils::secToTicks(1, App::Config::kFrameRate);

SavedView::SavedView()
    : mDismissCb(this, &SavedView::onDismiss)
{

}

void SavedView::setupScreen()
{
    SavedViewBase::setupScreen();

    title.set(T_TEXT_ALARM_UC);

    mDismissTimer.setDuration(kDismissTicks);
    mDismissTimer.setCallback(mDismissCb);
    mDismissTimer.start();
}

void SavedView::tearDownScreen()
{
    mDismissTimer.stop();
    SavedViewBase::tearDownScreen();
}

void SavedView::setAlarmId(size_t id)
{
    Unicode::snprintf(messageTextBuffer, MESSAGETEXT_SIZE, "%s %d", touchgfx::TypedText(T_TEXT_ALARM).getText(), id + 1);
    messageText.invalidate();
}

void SavedView::onDismiss()
{
    presenter->exitScreen();
}