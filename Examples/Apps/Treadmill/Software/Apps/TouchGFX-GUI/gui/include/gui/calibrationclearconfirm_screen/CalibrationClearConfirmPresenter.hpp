#ifndef CALIBRATIONCLEARCONFIRMPRESENTER_HPP
#define CALIBRATIONCLEARCONFIRMPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class CalibrationClearConfirmView;

class CalibrationClearConfirmPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    CalibrationClearConfirmPresenter(CalibrationClearConfirmView& v);

    virtual void activate();
    virtual void deactivate();
    virtual void onIdleTimeout() override { model->exitApp(); }

    /// Back up and clear the calibration stores.
    void clearCalibration() { model->clearCalibrationData(); }

    virtual ~CalibrationClearConfirmPresenter() {}

private:
    CalibrationClearConfirmPresenter();

    CalibrationClearConfirmView& view;
};

#endif // CALIBRATIONCLEARCONFIRMPRESENTER_HPP
