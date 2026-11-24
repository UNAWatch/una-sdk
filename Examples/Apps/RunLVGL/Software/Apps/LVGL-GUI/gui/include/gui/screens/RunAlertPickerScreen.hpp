/**
 ******************************************************************************
 * @file    RunAlertPickerScreen.hpp
 * @brief   The Run Alerts min:sec picker for the field being edited
 *          (Model::runAlertField()): Run Time, Walk Time, Too Fast or Too Slow.
 *
 * Bounds and the opening value come from RunAlerts::pickerBounds() and
 * pickerSeed(), so 0:00 and an inverted pace range can't be picked. As the
 * Intervals pickers: L1/L2 change the active component, R1 moves to the
 * seconds and then saves, R2 steps back and then leaves.
 ******************************************************************************
 */

#ifndef RUN_ALERT_PICKER_SCREEN_HPP
#define RUN_ALERT_PICKER_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/PickerLogic.hpp"
#include "gui/widgets/Widgets.hpp"

class RunAlertPickerScreen : public Screen
{
public:
    explicit RunAlertPickerScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    /// Idle on a menu screen leaves the app, as the TouchGFX presenter does.
    void onIdleTimeout() override { mModel.exitApp(); }

protected:
    void build() override;

private:
    void render();

    PickerLogic::BoundedTime                mTime;
    std::unique_ptr<Widgets::TwoTonePicker> mPicker;
};

#endif // RUN_ALERT_PICKER_SCREEN_HPP
