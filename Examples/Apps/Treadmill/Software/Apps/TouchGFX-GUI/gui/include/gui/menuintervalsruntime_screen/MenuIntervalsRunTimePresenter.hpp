#ifndef MENUINTERVALSRUNTIMEPRESENTER_HPP
#define MENUINTERVALSRUNTIMEPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuIntervalsRunTimeView;

class MenuIntervalsRunTimePresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuIntervalsRunTimePresenter(MenuIntervalsRunTimeView& v);

    virtual void activate();
    virtual void deactivate();

    void saveRunTime(uint32_t totalSeconds);

    virtual ~MenuIntervalsRunTimePresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }

private:
    MenuIntervalsRunTimePresenter();

    MenuIntervalsRunTimeView& view;
};

#endif // MENUINTERVALSRUNTIMEPRESENTER_HPP
