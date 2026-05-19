#ifndef MENUINTERVALSRUNDISTANCEPRESENTER_HPP
#define MENUINTERVALSRUNDISTANCEPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuIntervalsRunDistanceView;

class MenuIntervalsRunDistancePresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuIntervalsRunDistancePresenter(MenuIntervalsRunDistanceView& v);

    virtual void activate();
    virtual void deactivate();

    void saveRunDistance(float meters);

    virtual ~MenuIntervalsRunDistancePresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }

private:
    MenuIntervalsRunDistancePresenter();

    MenuIntervalsRunDistanceView& view;
};

#endif // MENUINTERVALSRUNDISTANCEPRESENTER_HPP
