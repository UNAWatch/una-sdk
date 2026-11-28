/**
 ******************************************************************************
 * @file    IntervalsPickerScreen.cpp
 * @brief   Two-stage time / distance picker (see IntervalsPickerScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/IntervalsPickerScreen.hpp"
#include "gui/screens/ScreenManager.hpp"

IntervalsPickerScreen::IntervalsPickerScreen(Model& model, Phase phase, Metric metric)
    : Screen(model)
    , mPhase(phase)
    , mMetric(metric)
{
}

void IntervalsPickerScreen::build()
{
    mPicker = std::make_unique<Widgets::TwoTonePicker>(mRoot);
    mPicker->setTitle(mMetric == Metric::Time ? "TIME" : "DISTANCE");
}

void IntervalsPickerScreen::onShow()
{
    const Settings::Intervals& iv = mModel.getSettings().intervals;
    const bool run = mPhase == Phase::Run;
    if (mMetric == Metric::Time) {
        mTime.seed(run ? iv.runTime : iv.restTime);
    } else {
        mDistance.seed(run ? iv.runDistance : iv.restDistance, mModel.isUnitsImperial());
    }
    render();
    mModel.resetIdleTimer();
}

void IntervalsPickerScreen::render()
{
    if (mMetric == Metric::Time) {
        mTime.render(*mPicker);
    } else {
        mDistance.render(*mPicker);
    }
}

ScreenId IntervalsPickerScreen::backScreen() const
{
    return mPhase == Phase::Run ? ScreenId::MenuIntervalsRun : ScreenId::MenuIntervalsRest;
}

void IntervalsPickerScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    const bool atSecond = mMetric == Metric::Time ? mTime.atSec() : mDistance.atFrac();

    switch (code) {
        case Btn::L1:
            if (mMetric == Metric::Time) mTime.dec(); else mDistance.dec();
            render();
            break;
        case Btn::L2:
            if (mMetric == Metric::Time) mTime.inc(); else mDistance.inc();
            render();
            break;
        case Btn::R1:
            // First press moves to the second component, second press saves.
            if (!atSecond) {
                if (mMetric == Metric::Time) mTime.toSec(); else mDistance.toFrac();
                render();
            } else {
                save();
                ScreenManager::instance().goTo(ScreenId::MenuIntervals);
            }
            break;
        case Btn::R2:
            // Back one component, or out of the picker from the first one.
            if (!atSecond) {
                ScreenManager::instance().goTo(backScreen());
            } else {
                if (mMetric == Metric::Time) mTime.toMin(); else mDistance.toWhole();
                render();
            }
            break;
        default:
            break;
    }
}

void IntervalsPickerScreen::save()
{
    Settings sett = mModel.getSettings();
    Settings::Intervals& iv = sett.intervals;
    const bool run = mPhase == Phase::Run;
    Settings::Intervals::Metric& metric = run ? iv.runMetric : iv.restMetric;

    // A zero value means the phase is open-ended.
    if (mMetric == Metric::Time) {
        const uint32_t total = mTime.totalSeconds();
        uint32_t& field = run ? iv.runTime : iv.restTime;
        metric = total > 0 ? Settings::Intervals::TIME : Settings::Intervals::OPEN;
        field  = total;
    } else {
        const float metres = mDistance.meters();
        float& field = run ? iv.runDistance : iv.restDistance;
        metric = metres > 0.0f ? Settings::Intervals::DISTANCE : Settings::Intervals::OPEN;
        field  = metres > 0.0f ? metres : 0.0f;
    }
    mModel.saveSettings(sett);
}
