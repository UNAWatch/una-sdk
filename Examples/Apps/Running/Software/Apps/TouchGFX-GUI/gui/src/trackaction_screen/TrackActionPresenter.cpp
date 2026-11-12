#include <gui/trackaction_screen/TrackActionView.hpp>
#include <gui/trackaction_screen/TrackActionPresenter.hpp>

TrackActionPresenter::TrackActionPresenter(TrackActionView& v)
    : view(v)
{
}

void TrackActionPresenter::activate()
{
    view.setUnitsImperial(model->isUnitsImperial());
    const Track::Data& data = model->getTrackData();
    // Lap is for a free run, and only once per pause.
    view.setMode(!data.intervalsMode && !data.workoutMode && !model->isPausedLapTaken(),
                 data.workoutMode);
    onTrackData(model->getTrackData());

    // Reset idle timer
    model->resetIdleTimer();

    view.setPositionId(model->menu().track.action.get());

    model->trackPause();
}

void TrackActionPresenter::deactivate()
{
    model->menu().track.action.set(view.getPositionId());
}

void TrackActionPresenter::onTrackData(const Track::Data& data)
{
    view.setTimer(data.totalTime);
    view.setAvgPace(data.avgPace);
    view.setDistance(data.distance);
    view.setAvgHR(data.avgHR);
    view.setElevation(data.elevation);
}

void TrackActionPresenter::onLapChanged(uint8_t /*lapEnd*/)
{
    model->setLapFromPauseMenu(true);
    model->application().gotoTrackLapScreenNoTransition();
}

void TrackActionPresenter::resumeTrack()
{
    model->trackResume();
}

void TrackActionPresenter::saveLap()
{
    model->saveLap();
}
