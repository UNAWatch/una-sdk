#include <gui/deleted_screen/DeletedView.hpp>
#include <texts/TextKeysAndLanguages.hpp>

// How long the confirmation stays up before returning to Main.
static const uint16_t kHoldTicks = SDK::Utils::secToTicks(2, App::Config::kFrameRate);

DeletedView::DeletedView()
    : mDismissCb(this, &DeletedView::onDismiss)
{
}

void DeletedView::setupScreen()
{
    DeletedViewBase::setupScreen();

    title.set(T_TEXT_TIMER_UC);

    mDismissTimer.setDuration(kHoldTicks);
    mDismissTimer.setCallback(mDismissCb);
    mDismissTimer.start();
}

void DeletedView::tearDownScreen()
{
    mDismissTimer.stop();
    DeletedViewBase::tearDownScreen();
}

void DeletedView::onDismiss()
{
    application().gotoMainScreenNoTransition();
}
