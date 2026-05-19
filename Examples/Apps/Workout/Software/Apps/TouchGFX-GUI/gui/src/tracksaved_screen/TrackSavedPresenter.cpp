#include <gui/tracksaved_screen/TrackSavedView.hpp>
#include <gui/tracksaved_screen/TrackSavedPresenter.hpp>

TrackSavedPresenter::TrackSavedPresenter(TrackSavedView& v)
    : view(v)
{

}

void TrackSavedPresenter::activate()
{
    model->saveTrack();
}

void TrackSavedPresenter::deactivate()
{

}
