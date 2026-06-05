#include <gui/calibrationclearconfirm_screen/CalibrationClearConfirmView.hpp>
#include <gui/calibrationclearconfirm_screen/CalibrationClearConfirmPresenter.hpp>

CalibrationClearConfirmPresenter::CalibrationClearConfirmPresenter(CalibrationClearConfirmView& v)
    : view(v)
{
}

void CalibrationClearConfirmPresenter::activate()
{
    model->resetIdleTimer();
}

void CalibrationClearConfirmPresenter::deactivate()
{
}
