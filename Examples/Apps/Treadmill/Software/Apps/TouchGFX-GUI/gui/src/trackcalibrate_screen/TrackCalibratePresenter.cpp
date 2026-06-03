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
    model->trackCalibrate(meters);
}

void TrackCalibratePresenter::skipCalibration()
{
    model->trackCalibrate(0.0f);
}
