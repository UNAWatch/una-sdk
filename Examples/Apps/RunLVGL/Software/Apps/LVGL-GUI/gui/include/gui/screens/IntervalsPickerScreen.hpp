/**
 ******************************************************************************
 * @file    IntervalsPickerScreen.hpp
 * @brief   Two-stage picker for a phase's time (mm:ss) or distance (km/mi).
 *
 * One class for the Run app's four MenuIntervals{Run,Rest}{Time,Distance}
 * screens. L1/L2 change the active component, R1 advances to the second
 * component and then saves, R2 steps back and then leaves.
 ******************************************************************************
 */

#ifndef INTERVALS_PICKER_SCREEN_HPP
#define INTERVALS_PICKER_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/screens/MenuIntervalsMetricScreen.hpp"
#include "gui/widgets/PickerLogic.hpp"
#include "gui/widgets/Widgets.hpp"

class IntervalsPickerScreen : public Screen
{
public:
    using Phase = MenuIntervalsMetricScreen::Phase;
    enum class Metric : uint8_t { Time, Distance };

    IntervalsPickerScreen(Model& model, Phase phase, Metric metric);

    void onShow() override;
    void onKey(uint8_t code) override;
    /// Idle on a menu screen leaves the app, as the TouchGFX presenter does.
    void onIdleTimeout() override { mModel.exitApp(); }

protected:
    void build() override;

private:
    void render();
    void save();
    ScreenId backScreen() const;

    Phase  mPhase;
    Metric mMetric;
    PickerLogic::Time<App::MenuNav::Root::Intervals::TimePicker>         mTime;
    PickerLogic::Distance<App::MenuNav::Root::Intervals::DistancePicker> mDistance;
    std::unique_ptr<Widgets::TwoTonePicker> mPicker;
};

#endif // INTERVALS_PICKER_SCREEN_HPP
