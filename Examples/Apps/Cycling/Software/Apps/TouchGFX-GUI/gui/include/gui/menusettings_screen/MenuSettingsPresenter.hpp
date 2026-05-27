#ifndef MENUSETTINGSPRESENTER_HPP
#define MENUSETTINGSPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuSettingsView;

class MenuSettingsPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuSettingsPresenter(MenuSettingsView& v);

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

    virtual ~MenuSettingsPresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }
    virtual void onGpsFix(bool acquired) override;

    void savePhoneNotif(bool state);

private:
    MenuSettingsPresenter();

    MenuSettingsView& view;
};

#endif // MENUSETTINGSPRESENTER_HPP
