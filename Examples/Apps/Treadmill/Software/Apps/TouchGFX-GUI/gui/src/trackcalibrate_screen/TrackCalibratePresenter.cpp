#include <gui/trackcalibrate_screen/TrackCalibrateView.hpp>
#include <gui/trackcalibrate_screen/TrackCalibratePresenter.hpp>

TrackCalibratePresenter::TrackCalibratePresenter(TrackCalibrateView& v)
    : view(v)
{

}

void TrackCalibratePresenter::activate()
{
    // Seed the entry with the estimated distance so the user adjusts from it.
    view.setDistance(model->getTrackData().distance, model->isUnitsImperial());
    model->resetIdleTimer();
}

void TrackCalibratePresenter::deactivate()
{

}

void TrackCalibratePresenter::applyCalibration(float meters)
{
    // Confirm: stop the activity now (the Service snapshots the session and
    // builds the summary), then fold in the entered distance. Confirming the
    // seeded estimate is equivalent to saving without correction (ΔD = 0).
    model->saveTrack();
    model->trackCalibrate(meters);
}
