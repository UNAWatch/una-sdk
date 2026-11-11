#include <gui/trackaction_screen/TrackActionView.hpp>
#include <gui/trackaction_screen/TrackActionPresenter.hpp>

TrackActionPresenter::TrackActionPresenter(TrackActionView& v)
    : view(v)
{
}

void TrackActionPresenter::activate()
{
    view.setUnitsImperial(model->isUnitsImperial());
    // Lap is for a free run, and only once per pause.
    view.setLapShown(!model->getTrackData().intervalsMode && !model->isPausedLapTaken());
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
    view.setAvgPace(data.avgSpeed);
    view.setDistance(data.distance);
    view.setAvgHR(data.avgHR);
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

void TrackActionPresenter::saveRequested()
{
    // Do NOT stop the activity yet: keep it paused through the Calibrate & Save
    // intro + distance picker, so R2 (back) on the picker can return to this
    // pause menu with Resume/Discard still available. The stop + finalise happens
    // only when the user confirms a distance on the picker
    // (TrackCalibratePresenter::applyCalibration). Confirming the seeded estimate
    // keeps the estimate; whether the entered distance also updates the delta LUT
    // is gated Service-side (outdoor-calibrated tier + a minimum distance).
    model->application().gotoTrackCalibrateIntroScreenNoTransition();
}
