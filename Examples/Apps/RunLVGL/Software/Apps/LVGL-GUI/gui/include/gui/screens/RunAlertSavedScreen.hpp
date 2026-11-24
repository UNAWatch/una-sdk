/**
 ******************************************************************************
 * @file    RunAlertSavedScreen.hpp
 * @brief   "Saved" for 1 s after a Run Alerts picker or list saves, with the
 *          field's name and new value, then back to its submenu. The Lap
 *          Alerts Saved screen's layout.
 ******************************************************************************
 */

#ifndef RUN_ALERT_SAVED_SCREEN_HPP
#define RUN_ALERT_SAVED_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class RunAlertSavedScreen : public Screen
{
public:
    explicit RunAlertSavedScreen(Model& model);
    ~RunAlertSavedScreen() override;

    void onShow() override;

protected:
    void build() override;

private:
    static void dismissCb(lv_timer_t* t);

    lv_timer_t* mDismiss = nullptr;
    lv_obj_t*   mMessage = nullptr;
    std::unique_ptr<Widgets::Title> mTitle;
};

#endif // RUN_ALERT_SAVED_SCREEN_HPP
