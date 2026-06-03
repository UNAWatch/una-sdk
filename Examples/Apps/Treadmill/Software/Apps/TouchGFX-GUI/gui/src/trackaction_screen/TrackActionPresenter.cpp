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
    view.setAvgPace(data.avgPace);
    view.setDistance(data.distance);
    view.setAvgHR(data.avgHR);
    view.setElevation(data.elevation);
}

void TrackActionPresenter::resumeTrack()
{
    model->trackResume();
}

void TrackActionPresenter::saveRequested()
{
    // Stop recording. The Service builds the summary at the estimated distance
    // and, for eligible sessions, waits for the Calibrate & Save decision.
    model->saveTrack();

    // Offer Calibrate & Save only for sessions of at least 2 km (§5.1); shorter
    // ones are finalised immediately by the Service with the estimate.
    if (model->getTrackData().distance >= 2000.0f) {
        model->application().gotoTrackCalibrateScreenNoTransition();
    } else {
        model->application().gotoTrackSavedScreenNoTransition();
    }
}
