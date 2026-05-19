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
    model->trackStart();
    model->application().gotoTrackScreenNoTransition();
}