#ifndef MENUDISTANCESAVEDPRESENTER_HPP
#define MENUDISTANCESAVEDPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuDistanceSavedView;

class MenuDistanceSavedPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuDistanceSavedPresenter(MenuDistanceSavedView& v);

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

    virtual ~MenuDistanceSavedPresenter() {}

private:
    MenuDistanceSavedPresenter();

    MenuDistanceSavedView& view;
};

#endif // MENUDISTANCESAVEDPRESENTER_HPP
