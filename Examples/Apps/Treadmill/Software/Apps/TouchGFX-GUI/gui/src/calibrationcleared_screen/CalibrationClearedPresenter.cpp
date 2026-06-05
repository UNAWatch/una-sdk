#include <gui/calibrationcleared_screen/CalibrationClearedView.hpp>
#include <gui/calibrationcleared_screen/CalibrationClearedPresenter.hpp>

CalibrationClearedPresenter::CalibrationClearedPresenter(CalibrationClearedView& v)
    : view(v)
{
}

void CalibrationClearedPresenter::activate()
{
    model->resetIdleTimer();
}

void CalibrationClearedPresenter::deactivate()
{
}
