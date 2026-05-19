#include <gui/trackdiscardconfirmation_screen/TrackDiscardConfirmationView.hpp>
#include <gui/trackdiscardconfirmation_screen/TrackDiscardConfirmationPresenter.hpp>

TrackDiscardConfirmationPresenter::TrackDiscardConfirmationPresenter(TrackDiscardConfirmationView& v)
    : view(v)
{

}

void TrackDiscardConfirmationPresenter::activate()
{
    // Reset idle timer
    model->resetIdleTimer();
}

void TrackDiscardConfirmationPresenter::deactivate()
{

}
