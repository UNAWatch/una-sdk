#include <gui/trackintervalsworkoutcompleted_screen/TrackIntervalsWorkoutCompletedView.hpp>
#include <gui/trackintervalsworkoutcompleted_screen/TrackIntervalsWorkoutCompletedPresenter.hpp>

TrackIntervalsWorkoutCompletedPresenter::TrackIntervalsWorkoutCompletedPresenter(TrackIntervalsWorkoutCompletedView& v)
    : view(v)
{

}

void TrackIntervalsWorkoutCompletedPresenter::activate()
{
    model->saveTrack();
}

void TrackIntervalsWorkoutCompletedPresenter::deactivate()
{

}
