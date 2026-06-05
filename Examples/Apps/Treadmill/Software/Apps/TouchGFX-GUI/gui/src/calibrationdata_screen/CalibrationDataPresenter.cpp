#include <gui/calibrationdata_screen/CalibrationDataView.hpp>
#include <gui/calibrationdata_screen/CalibrationDataPresenter.hpp>

CalibrationDataPresenter::CalibrationDataPresenter(CalibrationDataView& v)
    : view(v)
{
}

void CalibrationDataPresenter::activate()
{
    model->resetIdleTimer();
    // Paint immediately from the last snapshot, then ask the Service to refresh.
    view.setData(model->getCalibrationView());
    model->requestCalibrationData();
}

void CalibrationDataPresenter::deactivate()
{
}

void CalibrationDataPresenter::onCalibrationData(const Model::CalibrationView& data)
{
    view.setData(data);
}
