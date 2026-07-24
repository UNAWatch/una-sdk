#include <gui/trackdiscarded_screen/TrackDiscardedView.hpp>

static constexpr uint16_t kDismissTicks = SDK::Utils::secToTicks(2, App::Config::kFrameRate);

TrackDiscardedView::TrackDiscardedView()
    : mDismissCb(this, &TrackDiscardedView::onDismiss)
{
}

void TrackDiscardedView::setupScreen()
{
    TrackDiscardedViewBase::setupScreen();

    if (const char* variant = application().getModel().variantTitle()) {
        title.set(variant);
    } else {
        title.set(T_TEXT_APP_NAME_UC);
    }

    mDismissTimer.setDuration(kDismissTicks);
    mDismissTimer.setCallback(mDismissCb);
    mDismissTimer.start();
}

void TrackDiscardedView::tearDownScreen()
{
    mDismissTimer.stop();
    TrackDiscardedViewBase::tearDownScreen();
}

void TrackDiscardedView::onDismiss()
{
    presenter->exitApp();
}
