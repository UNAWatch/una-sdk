#include <gui/trackintervalsalert_screen/TrackIntervalsAlertView.hpp>
#include <gui/trackintervalsalert_screen/TrackIntervalsAlertPresenter.hpp>

TrackIntervalsAlertPresenter::TrackIntervalsAlertPresenter(TrackIntervalsAlertView& v)
    : view(v)
{
}

void TrackIntervalsAlertPresenter::activate()
{
    // Use the snapshot carried by the alert message, not getTrackData().intervals --
    // the latter may not be updated yet when this screen activates.
    const Track::IntervalsData& iv = model->getPendingAlertIntervals();

    view.setPhase(iv.phase, iv.repeat, iv.totalRepeats);

    if (iv.metric == Track::IntervalsMetric::DISTANCE) {
        const bool imperial     = model->isUnitsImperial();
        const float distMetres  = iv.distRemaining;
        const float distDisplay = imperial ? distMetres / 1609.344f : distMetres / 1000.0f;
        view.setPhaseDistance(distDisplay, imperial);
    } else {
        view.setPhaseTime(iv.phaseTimerSec);
    }
}

void TrackIntervalsAlertPresenter::deactivate()
{
}
