#include <gui/tracksaved_screen/TrackSavedView.hpp>
#include <gui/tracksaved_screen/TrackSavedPresenter.hpp>

TrackSavedPresenter::TrackSavedPresenter(TrackSavedView& v)
    : view(v)
{

}

void TrackSavedPresenter::activate()
{
    // The stop (and any Calibrate & Save) was already triggered from the track
    // action / calibrate screens; this screen only shows the confirmation.
}

void TrackSavedPresenter::deactivate()
{

}
