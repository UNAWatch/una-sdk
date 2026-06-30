#include <gui/trackholdconfirmation_screen/TrackHoldConfirmationView.hpp>
#include <gui/trackholdconfirmation_screen/TrackHoldConfirmationPresenter.hpp>

TrackHoldConfirmationPresenter::TrackHoldConfirmationPresenter(TrackHoldConfirmationView& v)
    : view(v)
{
}

void TrackHoldConfirmationPresenter::activate()
{
    // Reset idle timer
    model->resetIdleTimer();
}

void TrackHoldConfirmationPresenter::deactivate()
{
}
