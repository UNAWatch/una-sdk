#include <gui/trackaction_screen/TrackActionView.hpp>
#include <gui/trackaction_screen/TrackActionPresenter.hpp>

TrackActionPresenter::TrackActionPresenter(TrackActionView& v)
    : view(v)
{
}

void TrackActionPresenter::activate()
{
    view.setUnitsImperial(model->isUnitsImperial());
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

void TrackActionPresenter::resumeTrack()
{
    model->trackResume();
}

void TrackActionPresenter::saveRequested()
{
    // Stop recording. The Service builds the summary at the estimated distance
    // and waits for the Calibrate & Save decision.
    model->saveTrack();

    // Always offer Calibrate & Save so the user can correct the recorded
    // distance (and the avg speed derived from it) on any run. The intro screen
    // explains the step, then advances to the distance picker; the user can skip
    // to keep the estimate. Whether the entered distance also updates the delta
    // LUT is gated Service-side (outdoor-calibrated tier + a minimum distance).
    model->application().gotoTrackCalibrateIntroScreenNoTransition();
}
