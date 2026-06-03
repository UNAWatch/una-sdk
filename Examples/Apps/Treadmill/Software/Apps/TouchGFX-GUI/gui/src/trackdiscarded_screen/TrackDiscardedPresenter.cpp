#include <gui/trackdiscarded_screen/TrackDiscardedView.hpp>
#include <gui/trackdiscarded_screen/TrackDiscardedPresenter.hpp>

TrackDiscardedPresenter::TrackDiscardedPresenter(TrackDiscardedView& v)
    : view(v)
{

}

void TrackDiscardedPresenter::activate()
{
    // Reset idle timer
    model->resetIdleTimer();

    model->discardTrack();
}

void TrackDiscardedPresenter::deactivate()
{

}

void TrackDiscardedPresenter::exitApp()
{
    model->exitApp();
}