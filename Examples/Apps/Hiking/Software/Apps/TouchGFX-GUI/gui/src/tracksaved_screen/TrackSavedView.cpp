#include <gui/tracksaved_screen/TrackSavedView.hpp>

static constexpr uint16_t kDismissTicks = SDK::Utils::secToTicks(2, App::Config::kFrameRate);

TrackSavedView::TrackSavedView()
    : mDismissCb(this, &TrackSavedView::onDismiss)
{
}

void TrackSavedView::setupScreen()
{
    TrackSavedViewBase::setupScreen();

    title.set(T_TEXT_APP_NAME_UC);

    mDismissTimer.setDuration(kDismissTicks);
    mDismissTimer.setCallback(mDismissCb);
    mDismissTimer.start();
}

void TrackSavedView::tearDownScreen()
{
    mDismissTimer.stop();
    TrackSavedViewBase::tearDownScreen();
}

void TrackSavedView::onDismiss()
{
    application().gotoTrackSummaryScreenNoTransition();
}
