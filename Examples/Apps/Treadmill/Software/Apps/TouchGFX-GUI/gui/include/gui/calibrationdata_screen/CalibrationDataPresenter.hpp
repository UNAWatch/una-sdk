#ifndef CALIBRATIONDATAPRESENTER_HPP
#define CALIBRATIONDATAPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class CalibrationDataView;

class CalibrationDataPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    CalibrationDataPresenter(CalibrationDataView& v);

    virtual void activate();
    virtual void deactivate();
    virtual void onIdleTimeout() override { model->exitApp(); }

    /// Fresh snapshot arrived from the Service.
    virtual void onCalibrationData(const Model::CalibrationView& data) override;

    virtual ~CalibrationDataPresenter() {}

private:
    CalibrationDataPresenter();

    CalibrationDataView& view;
};

#endif // CALIBRATIONDATAPRESENTER_HPP
