#ifndef MENUINTERVALSRUNPRESENTER_HPP
#define MENUINTERVALSRUNPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuIntervalsRunView;

class MenuIntervalsRunPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuIntervalsRunPresenter(MenuIntervalsRunView& v);

    virtual void activate();
    virtual void deactivate();

    void saveOpenRun();

    virtual ~MenuIntervalsRunPresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }

private:
    MenuIntervalsRunPresenter();

    MenuIntervalsRunView& view;
};

#endif // MENUINTERVALSRUNPRESENTER_HPP
