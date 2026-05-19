#ifndef MENUTIMEPRESENTER_HPP
#define MENUTIMEPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuTimeView;

class MenuTimePresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuTimePresenter(MenuTimeView& v);

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

    virtual ~MenuTimePresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }

    void saveTime(Settings::Alerts::Time::Id id);

private:
    MenuTimePresenter();

    MenuTimeView& view;
};

#endif // MENUTIMEPRESENTER_HPP
