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
    if (model->startMode(model->isPendingIntervalsMode()) != RunAlerts::StartMode::Free) {
        // Intervals were configured, a workout armed, or Run/Walk is on, and
        // Start chosen without a GPS fix; confirming proceeds straight to the
        // countdown.
        model->application().gotoTrackIntervalsCountdownScreenNoTransition();
    } else {
        model->trackStart(false);
        model->application().gotoTrackScreenNoTransition();
    }
}