#ifndef MENUINTERVALSRESTTIMEPRESENTER_HPP
#define MENUINTERVALSRESTTIMEPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuIntervalsRestTimeView;

class MenuIntervalsRestTimePresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuIntervalsRestTimePresenter(MenuIntervalsRestTimeView& v);

    virtual void activate();
    virtual void deactivate();

    void saveRestTime(uint32_t totalSeconds);

    virtual ~MenuIntervalsRestTimePresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }

private:
    MenuIntervalsRestTimePresenter();

    MenuIntervalsRestTimeView& view;
};

#endif // MENUINTERVALSRESTTIMEPRESENTER_HPP
