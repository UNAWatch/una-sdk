#ifndef MENUDISTANCESAVEDVIEW_HPP
#define MENUDISTANCESAVEDVIEW_HPP

#include <gui_generated/menudistancesaved_screen/MenuDistanceSavedViewBase.hpp>
#include <gui/menudistancesaved_screen/MenuDistanceSavedPresenter.hpp>
#include <gui/containers/CountdownTimer.hpp>
#include <touchgfx/Callback.hpp>

class MenuDistanceSavedView : public MenuDistanceSavedViewBase
{
public:
    MenuDistanceSavedView();
    virtual ~MenuDistanceSavedView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setDistanceUnits(Settings::Alerts::Distance::Id id, bool isImperial);

private:
    using Menu = App::MenuNav::Root::Settings::Alerts::Distance;

    void onDismiss();

    CountdownTimer                              mDismissTimer;
    touchgfx::Callback<MenuDistanceSavedView>   mDismissCb;
};

#endif // MENUDISTANCESAVEDVIEW_HPP
