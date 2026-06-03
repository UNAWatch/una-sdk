#ifndef MENUINTERVALSPRESENTER_HPP
#define MENUINTERVALSPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuIntervalsView;

class MenuIntervalsPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuIntervalsPresenter(MenuIntervalsView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~MenuIntervalsPresenter() {}

    virtual void onIdleTimeout() override;

    void saveWarmUp(bool enable);
    void saveCoolDown(bool enable);

private:
    MenuIntervalsPresenter();

    MenuIntervalsView& view;
};

#endif // MENUINTERVALSPRESENTER_HPP
