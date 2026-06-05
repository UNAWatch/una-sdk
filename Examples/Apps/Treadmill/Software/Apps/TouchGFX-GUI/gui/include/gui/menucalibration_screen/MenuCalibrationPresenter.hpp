#ifndef MENUCALIBRATIONPRESENTER_HPP
#define MENUCALIBRATIONPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuCalibrationView;

class MenuCalibrationPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuCalibrationPresenter(MenuCalibrationView& v);

    virtual void activate();
    virtual void deactivate();
    virtual void onIdleTimeout() override { model->exitApp(); }

    virtual ~MenuCalibrationPresenter() {}

private:
    MenuCalibrationPresenter();

    MenuCalibrationView& view;
};

#endif // MENUCALIBRATIONPRESENTER_HPP
