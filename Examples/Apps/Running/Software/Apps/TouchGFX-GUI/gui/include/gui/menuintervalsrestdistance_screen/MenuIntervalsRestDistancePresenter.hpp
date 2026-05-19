#ifndef MENUINTERVALSRESTDISTANCEPRESENTER_HPP
#define MENUINTERVALSRESTDISTANCEPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuIntervalsRestDistanceView;

class MenuIntervalsRestDistancePresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuIntervalsRestDistancePresenter(MenuIntervalsRestDistanceView& v);

    virtual void activate();
    virtual void deactivate();

    void saveRestDistance(float meters);

    virtual ~MenuIntervalsRestDistancePresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }

private:
    MenuIntervalsRestDistancePresenter();

    MenuIntervalsRestDistanceView& view;
};

#endif // MENUINTERVALSRESTDISTANCEPRESENTER_HPP
