/**
 ******************************************************************************
 * @file    AlertSavedScreen.hpp
 * @brief   One-second "Saved" confirmation after choosing an auto-lap value.
 *
 * One class for the Run app's MenuDistanceSavedView and MenuTimeSavedView.
 ******************************************************************************
 */

#ifndef ALERT_SAVED_SCREEN_HPP
#define ALERT_SAVED_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/screens/MenuAlertValueScreen.hpp"
#include "gui/widgets/Widgets.hpp"

class AlertSavedScreen : public Screen
{
public:
    using Kind = MenuAlertValueScreen::Kind;

    AlertSavedScreen(Model& model, Kind kind);
    ~AlertSavedScreen() override;

    void onShow() override;

protected:
    void build() override;

private:
    static void dismissCb(lv_timer_t* t);

    Kind        mKind;
    lv_timer_t* mDismiss = nullptr;
    lv_obj_t*   mMessage = nullptr;
    std::unique_ptr<Widgets::Title> mTitle;
};

#endif // ALERT_SAVED_SCREEN_HPP
