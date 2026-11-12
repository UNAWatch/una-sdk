#include <gui/trackaction_screen/TrackActionView.hpp>
#include <gui/trackaction_screen/TrackActionPresenter.hpp>

TrackActionPresenter::TrackActionPresenter(TrackActionView& v)
    : view(v)
{}

void TrackActionPresenter::activate()
{
    view.setUnitsImperial(model->isUnitsImperial());
    // Lap is taken only once per pause.
    view.setLapShown(!model->isPausedLapTaken());
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
    view.setSteps(data.steps);
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
