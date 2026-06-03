#include <gui/trackintervalscountdown_screen/TrackIntervalsCountdownView.hpp>
#include <gui/trackintervalscountdown_screen/TrackIntervalsCountdownPresenter.hpp>

TrackIntervalsCountdownPresenter::TrackIntervalsCountdownPresenter(TrackIntervalsCountdownView& v)
    : view(v)
{

}

void TrackIntervalsCountdownPresenter::activate()
{
    model->resetIdleTimer();

    view.setIntervals(model->getSettings().intervals, model->isUnitsImperial());
}

void TrackIntervalsCountdownPresenter::deactivate()
{

}

void TrackIntervalsCountdownPresenter::startTrack()
{
    model->trackStart(true);

    if (model->getSettings().intervals.warmUp) {
        model->application().gotoTrackScreenNoTransition();
    } else {
        model->application().gotoTrackIntervalsAlertScreenNoTransition();
    }
}