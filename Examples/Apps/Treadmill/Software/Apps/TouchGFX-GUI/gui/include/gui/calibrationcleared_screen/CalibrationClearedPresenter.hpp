#ifndef CALIBRATIONCLEAREDPRESENTER_HPP
#define CALIBRATIONCLEAREDPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class CalibrationClearedView;

class CalibrationClearedPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    CalibrationClearedPresenter(CalibrationClearedView& v);

    virtual void activate();
    virtual void deactivate();
    virtual void onIdleTimeout() override { model->exitApp(); }

    virtual ~CalibrationClearedPresenter() {}

private:
    CalibrationClearedPresenter();

    CalibrationClearedView& view;
};

#endif // CALIBRATIONCLEAREDPRESENTER_HPP
