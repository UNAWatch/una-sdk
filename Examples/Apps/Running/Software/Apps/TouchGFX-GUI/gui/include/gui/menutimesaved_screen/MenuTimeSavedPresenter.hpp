#ifndef MENUTIMESAVEDPRESENTER_HPP
#define MENUTIMESAVEDPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuTimeSavedView;

class MenuTimeSavedPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuTimeSavedPresenter(MenuTimeSavedView& v);

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

    virtual ~MenuTimeSavedPresenter() {}

private:
    MenuTimeSavedPresenter();

    MenuTimeSavedView& view;
};

#endif // MENUTIMESAVEDPRESENTER_HPP
