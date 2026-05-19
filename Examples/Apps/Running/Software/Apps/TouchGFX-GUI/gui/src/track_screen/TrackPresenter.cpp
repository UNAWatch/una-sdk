#include <gui/track_screen/TrackView.hpp>
#include <gui/track_screen/TrackPresenter.hpp>

TrackPresenter::TrackPresenter(TrackView& v)
    : view(v)
{

}

void TrackPresenter::activate()
{
    // Reset nested action menu position
    model->menu().track.action.reset();

    // Propagate intervals mode before setting the face position so that
    // setPositionId() can clamp to the valid range and set the scroll indicator count.
    // If intervalsMode is false and the stored position is ID_INTERVALS (default),
    // setPositionId() automatically advances to the first valid face (ID_TRACK1).
    const Track::Data& trackData = model->getTrackData();
    view.setIntervalsMode(trackData.intervalsMode);

    // If returning from a COOL_DOWN alert (auto-advance), force TrackFaceIntervals
    // so the user sees the cool-down phase regardless of which face was previously shown.
    const bool forceFaceIntervals =
        trackData.intervalsMode &&
        model->getPendingAlertIntervals().phase == Track::IntervalsPhase::COOL_DOWN;

    const uint16_t faceId = forceFaceIntervals
        ? static_cast<uint16_t>(App::MenuNav::TrackView::ID_INTERVALS)
        : model->menu().track.get();

    view.setPositionId(faceId);

    view.setConfig(model->isUnitsImperial(), model->getHrThresholds(), model->getHrThresholdsCount());

    onTrackData(model->getTrackData());

    uint8_t hour;
    uint8_t minute;
    uint8_t sec;
    model->getTime(hour, minute, sec);
    view.setTime(hour, minute);

    view.setBatteryLevel(model->getBatteryLevel());
    view.setGpsFix(model->hasGpsFix());
}

void TrackPresenter::deactivate()
{
    model->menu().track.set(view.getPositionId());
}

void TrackPresenter::onTrackData(const Track::Data& data)
{
    view.setTrackData(data);
}

void TrackPresenter::onBatteryLevel(uint8_t lvl)
{
    view.setBatteryLevel(lvl);
}

void TrackPresenter::onTime(uint8_t hour, uint8_t minute, uint8_t sec)
{
    view.setTime(hour, minute);
}

void TrackPresenter::onLapChanged(uint8_t lapEnd)
{
    model->application().gotoTrackLapScreenNoTransition();
}

void TrackPresenter::onIntervalsPhaseAlert()
{
    model->application().gotoTrackIntervalsAlertScreenNoTransition();
}

void TrackPresenter::onIntervalsWorkoutCompleted()
{
    model->application().gotoTrackIntervalsWorkoutCompletedScreenNoTransition();
}

void TrackPresenter::onGpsFix(bool acquired)
{
    view.setGpsFix(acquired);
}

void TrackPresenter::saveLap()
{
    model->saveLap();
}

void TrackPresenter::intervalsNextPhase()
{
    model->intervalsNextPhase();
}
