#ifndef MENUTIMESAVEDVIEW_HPP
#define MENUTIMESAVEDVIEW_HPP

#include <gui_generated/menutimesaved_screen/MenuTimeSavedViewBase.hpp>
#include <gui/menutimesaved_screen/MenuTimeSavedPresenter.hpp>
#include <gui/containers/CountdownTimer.hpp>
#include <touchgfx/Callback.hpp>

class MenuTimeSavedView : public MenuTimeSavedViewBase
{
public:
    MenuTimeSavedView();
    virtual ~MenuTimeSavedView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setTime(Settings::Alerts::Time::Id id);

private:
    using Menu = App::MenuNav::Root::Settings::Alerts::Time;

    void onDismiss();

    CountdownTimer                          mDismissTimer;
    touchgfx::Callback<MenuTimeSavedView>   mDismissCb;
};

#endif // MENUTIMESAVEDVIEW_HPP
