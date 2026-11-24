#ifndef MENURUNALERTSAVEDPRESENTER_HPP
#define MENURUNALERTSAVEDPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MenuRunAlertSavedView;

class MenuRunAlertSavedPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MenuRunAlertSavedPresenter(MenuRunAlertSavedView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~MenuRunAlertSavedPresenter() {}

private:
    MenuRunAlertSavedPresenter();

    MenuRunAlertSavedView& view;
};

#endif // MENURUNALERTSAVEDPRESENTER_HPP
