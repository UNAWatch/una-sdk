#ifndef MENUALERTSPRESENTER_HPP
#define MENUALERTSPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuAlertsView;

class MenuAlertsPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuAlertsPresenter(MenuAlertsView& v);

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

    virtual ~MenuAlertsPresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }

private:
    MenuAlertsPresenter();

    MenuAlertsView& view;
};

#endif // MENUALERTSPRESENTER_HPP
