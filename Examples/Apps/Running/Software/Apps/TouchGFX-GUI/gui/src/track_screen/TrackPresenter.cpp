#include <gui/track_screen/TrackView.hpp>
#include <gui/track_screen/TrackPresenter.hpp>

TrackPresenter::TrackPresenter(TrackView& v)
    : view(v)
{

}

void TrackPresenter::activate()
{
    mLeaving = false;

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

    // A step card or pop-up that came while another screen showed opens
    // before this screen (FrontendApplication::gotoTrackScreenNoTransition()).
    // This catches one that came after that choice was made, in the same tick.
    if (model->hasStepCard() || model->waitingPopup() != Model::Popup::None) {
        showPopup();
    }
}

void TrackPresenter::showPopup()
{
    // The step card or pop-up replaces this screen at the next tick. Until
    // then the faces are left as they are, so a new step does not show on
    // them before its card.
    mLeaving = true;
    model->application().gotoWaitingPopup();
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
    if (mLeaving) {
        return;
    }
    // A workout that completes or is ended drops to a free run.
    updateMode(data);
    view.setTrackData(data);
}

void TrackPresenter::onWorkoutData(const Model::WorkoutFace& face)
{
    if (mLeaving) {
        return;
    }
    view.setWorkoutFace(face);
}

void TrackPresenter::onWorkoutStep(bool next)
{
    if (mLeaving) {
        return;
    }
    if (next) {
        view.setWorkoutNext(model->getWorkoutStep(true));
        return;
    }
    // activate() shows the new step when the card hands back.
    if (model->hasStepCard()) {
        showPopup();
        return;
    }
    view.setWorkoutStep(model->getWorkoutStep(false));
}

void TrackPresenter::onBatteryLevel(uint8_t lvl)
{
    view.setBatteryLevel(lvl);
}

void TrackPresenter::onTime(uint8_t hour, uint8_t minute, uint8_t sec)
{
    view.setTime(hour, minute);
}

// The model has put each pop-up on its waiting list (Model::waitingPopup());
// one that comes while another pop-up shows opens when that one is done.

void TrackPresenter::onLapChanged(uint8_t lapEnd)
{
    showPopup();
}

void TrackPresenter::onIntervalsPhaseAlert()
{
    showPopup();
}

void TrackPresenter::onRunAlert()
{
    showPopup();
}

void TrackPresenter::onIntervalsWorkoutCompleted()
{
    showPopup();
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
