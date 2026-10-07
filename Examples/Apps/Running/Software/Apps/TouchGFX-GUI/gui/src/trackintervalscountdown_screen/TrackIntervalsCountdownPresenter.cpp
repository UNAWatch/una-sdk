#include <gui/trackintervalscountdown_screen/TrackIntervalsCountdownView.hpp>
#include <gui/trackintervalscountdown_screen/TrackIntervalsCountdownPresenter.hpp>

TrackIntervalsCountdownPresenter::TrackIntervalsCountdownPresenter(TrackIntervalsCountdownView& v)
    : view(v)
{

}

void TrackIntervalsCountdownPresenter::activate()
{
    model->resetIdleTimer();

    if (isWorkout()) {
        const size_t armed = static_cast<size_t>(model->getArmedWorkout());
        view.setWorkout(model->getWorkoutLibrary()->entry(armed), model->isUnitsImperial());
    } else {
        view.setIntervals(model->getSettings().intervals, model->isUnitsImperial());
    }
}

bool TrackIntervalsCountdownPresenter::isWorkout() const
{
    const SDK::Workout::Library* lib = model->getWorkoutLibrary();
    const int16_t armed = model->getArmedWorkout();
    return lib != nullptr && armed >= 0 && static_cast<size_t>(armed) < lib->size();
}

void TrackIntervalsCountdownPresenter::deactivate()
{

}

void TrackIntervalsCountdownPresenter::startTrack()
{
    if (isWorkout()) {
        // The next-step card for the first step follows from the Track screen.
        model->trackStart(false);
        model->application().gotoTrackScreenNoTransition();
        return;
    }
    model->trackStart(true);

    if (model->getSettings().intervals.warmUp) {
        model->application().gotoTrackScreenNoTransition();
    } else {
        model->application().gotoTrackIntervalsAlertScreenNoTransition();
    }
}