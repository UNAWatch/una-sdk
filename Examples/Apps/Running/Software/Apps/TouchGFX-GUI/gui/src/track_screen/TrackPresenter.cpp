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

    // Set the mode before the face position so that setPositionId() can fall
    // back to the first face the run has, and the scroll indicator count fits.
    const Track::Data& trackData = model->getTrackData();
    updateMode(trackData);

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
    view.setTimeFormat(model->is12HourFormat());

    onTrackData(model->getTrackData());
    view.setWorkoutStep(model->getWorkoutStep(false));
    view.setWorkoutNext(model->getWorkoutStep(true));
    view.setWorkoutFace(model->getWorkoutFace());

    uint8_t hour;
    uint8_t minute;
    uint8_t sec;
    model->getTime(hour, minute, sec);
    view.setTime(hour, minute);

    view.setBatteryLevel(model->getBatteryLevel());
    view.setGpsFix(model->hasGpsFix());
    view.setAccessoryStatus(model->getAccessoryState());

    // A step that started while another screen showed still gets its card.
    if (model->takeStepCard()) {
        model->application().gotoTrackWorkoutStepScreenNoTransition();
    }
}

void TrackPresenter::deactivate()
{
    model->menu().track.set(view.getPositionId());
}

void TrackPresenter::updateMode(const Track::Data& data)
{
    const TrackView::Mode mode = data.workoutMode    ? TrackView::Mode::Workout
                               : data.intervalsMode  ? TrackView::Mode::Intervals
                                                     : TrackView::Mode::Free;
    if (static_cast<int>(mode) != mMode) {
        mMode = static_cast<int>(mode);
        view.setMode(mode);
        view.setPositionId(view.getPositionId());
    }
}

void TrackPresenter::onTrackData(const Track::Data& data)
{
    // A workout that completes or is ended drops to a free run.
    updateMode(data);
    view.setTrackData(data);
}

void TrackPresenter::onWorkoutData(const Model::WorkoutFace& face)
{
    view.setWorkoutFace(face);
}

void TrackPresenter::onWorkoutStep(bool next)
{
    if (next) {
        view.setWorkoutNext(model->getWorkoutStep(true));
        return;
    }
    view.setWorkoutStep(model->getWorkoutStep(false));
    if (model->takeStepCard()) {
        model->application().gotoTrackWorkoutStepScreenNoTransition();
    }
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

void TrackPresenter::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    view.setAccessoryStatus(state);
}

void TrackPresenter::saveLap()
{
    model->saveLap();
}

void TrackPresenter::intervalsNextPhase()
{
    model->intervalsNextPhase();
}
