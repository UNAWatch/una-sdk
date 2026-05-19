#include <gui/deleted_screen/DeletedView.hpp>
#include <texts/TextKeysAndLanguages.hpp>

static constexpr uint16_t kDismissTicks = SDK::Utils::secToTicks(1, App::Config::kFrameRate);

DeletedView::DeletedView()
    : mDismissCb(this, &DeletedView::onDismiss)
{

}

void DeletedView::setupScreen()
{
    DeletedViewBase::setupScreen();

    title.set(T_TEXT_ALARM_UC);

    mDismissTimer.setDuration(kDismissTicks);
    mDismissTimer.setCallback(mDismissCb);
    mDismissTimer.start();
}

void DeletedView::tearDownScreen()
{
    mDismissTimer.stop();
    DeletedViewBase::tearDownScreen();
}

void DeletedView::setAlarmId(size_t id)
{
    Unicode::snprintf(messageTextBuffer, MESSAGETEXT_SIZE, "%s %d", touchgfx::TypedText(T_TEXT_ALARM).getText(), id + 1);
    messageText.invalidate();
}

void DeletedView::onDismiss()
{
    presenter->exitScreen();
}