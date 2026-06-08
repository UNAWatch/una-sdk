#include <gui/trackintervalsworkoutcompleted_screen/TrackIntervalsWorkoutCompletedView.hpp>
#include <gui/trackintervalsworkoutcompleted_screen/TrackIntervalsWorkoutCompletedPresenter.hpp>

TrackIntervalsWorkoutCompletedPresenter::TrackIntervalsWorkoutCompletedPresenter(TrackIntervalsWorkoutCompletedView& v)
    : view(v)
{

}

void TrackIntervalsWorkoutCompletedPresenter::activate()
{
    // This screen is only a notification that the programmed workout finished.
    // The session keeps running (the Service dropped out of intervals mode and
    // started a fresh lap); the view returns to the track screen so the user can
    // record additional laps and end the session manually. Do NOT save/stop here.
}

void TrackIntervalsWorkoutCompletedPresenter::deactivate()
{

}
