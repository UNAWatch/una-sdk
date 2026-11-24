#ifndef MENURUNALERTSAVEDVIEW_HPP
#define MENURUNALERTSAVEDVIEW_HPP

#include <gui_generated/menurunalertsaved_screen/MenuRunAlertSavedViewBase.hpp>
#include <gui/menurunalertsaved_screen/MenuRunAlertSavedPresenter.hpp>
#include <touchgfx/Callback.hpp>
#include <SDK/GUI/CountdownTimer.hpp>

/**
 * "Saved" for 1 s after a Run Alerts picker or list saves, with the field's
 * name and new value, then back to its submenu. The Lap Alerts Saved
 * screen's layout, with a wider value ("04:00 min").
 */
class MenuRunAlertSavedView : public MenuRunAlertSavedViewBase
{
public:
    MenuRunAlertSavedView();
    virtual ~MenuRunAlertSavedView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setSaved(RunAlerts::Field field, const char* value);

private:
    void onDismiss();

    RunAlerts::Field                          mField = RunAlerts::Field::RunTime;
    SDK::GUI::CountdownTimer                  mDismissTimer;
    touchgfx::Callback<MenuRunAlertSavedView> mDismissCb;
};

#endif // MENURUNALERTSAVEDVIEW_HPP
