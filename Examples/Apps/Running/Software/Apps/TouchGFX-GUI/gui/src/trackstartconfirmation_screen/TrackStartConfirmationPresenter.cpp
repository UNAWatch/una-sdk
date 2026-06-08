#include <gui/trackstartconfirmation_screen/TrackStartConfirmationView.hpp>
#include <gui/trackstartconfirmation_screen/TrackStartConfirmationPresenter.hpp>

TrackStartConfirmationPresenter::TrackStartConfirmationPresenter(TrackStartConfirmationView& v)
    : view(v)
{

}

void TrackStartConfirmationPresenter::activate()
{
    // Reset idle timer
    model->resetIdleTimer();
}

void TrackStartConfirmationPresenter::deactivate()
{

}

void TrackStartConfirmationPresenter::onIdleTimeout()
{
    model->application().gotoMainScreenNoTransition();
}

void TrackStartConfirmationPresenter::startTrack()
{
    if (model->isPendingIntervalsMode()) {
        // The user already configured intervals in the menu and chose Start
        // without a GPS fix; confirming proceeds straight to the countdown.
        model->application().gotoTrackIntervalsCountdownScreenNoTransition();
    } else {
        model->trackStart(false);
        model->application().gotoTrackScreenNoTransition();
    }
}