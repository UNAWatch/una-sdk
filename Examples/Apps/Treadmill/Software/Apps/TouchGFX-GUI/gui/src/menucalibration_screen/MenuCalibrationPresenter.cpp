#include <gui/menucalibration_screen/MenuCalibrationView.hpp>
#include <gui/menucalibration_screen/MenuCalibrationPresenter.hpp>

MenuCalibrationPresenter::MenuCalibrationPresenter(MenuCalibrationView& v)
    : view(v)
{
}

void MenuCalibrationPresenter::activate()
{
    view.setPositionId(model->menu().settings.calibration.get());
    model->resetIdleTimer();
}

void MenuCalibrationPresenter::deactivate()
{
    model->menu().settings.calibration.set(view.getPositionId());
}
