/**
 ******************************************************************************
 * @file    PickerLogic.hpp
 * @brief   State and rendering of the two-stage value pickers, shared by the
 *          interval time and distance screens.
 *
 * Ported from the Run app's PickerLogic. Each screen owns one of these plus a
 * TwoTonePicker, wires the buttons to inc()/dec()/stage changes and calls
 * render() after every change. Ranges and steps come from the AppMenu
 * descriptor (Root::Intervals::TimePicker / DistancePicker).
 ******************************************************************************
 */

#ifndef PICKER_LOGIC_HPP
#define PICKER_LOGIC_HPP

#include <cstdint>
#include <cstdio>

#include "SDK/Utils/Utils.hpp"
#include "gui/widgets/Widgets.hpp"

namespace PickerLogic
{

// -----------------------------------------------------------------------------
// Distance: whole.fraction, shown in km or mi. The fraction step comes from the
// descriptor (0.05 for intervals) and the displayed hundredths derive from it.
// -----------------------------------------------------------------------------
template<typename Menu>
struct Distance {
    enum Stage { WHOLE = 0, FRAC };
    Stage    stage    = WHOLE;
    bool     imperial = false;
    uint16_t whole    = 0;
    uint16_t fracIdx  = 0;

    static int hundStep() { return static_cast<int>(Menu::kFracStep * 100.0f + 0.5f); }
    uint16_t maxWhole() const { return imperial ? Menu::kMaxWholeMi : Menu::kMaxWholeKm; }

    void seed(float meters, bool isImperial)
    {
        imperial = isImperial;
        float units = meters / 1000.0f;
        if (imperial) units = SDK::Utils::kmToMiles(units);
        whole = static_cast<uint16_t>(units);
        if (whole > maxWhole()) whole = maxWhole();
        const float frac = units - whole;
        uint16_t idx = static_cast<uint16_t>(frac / Menu::kFracStep + 0.5f);
        if (idx >= Menu::kCountFrac) idx = Menu::kCountFrac - 1;
        fracIdx = idx;
        stage = WHOLE;
    }
    void dec()
    {
        if (stage == WHOLE) { if (whole > 0) --whole; }
        else                { if (fracIdx > 0) --fracIdx; }
    }
    void inc()
    {
        if (stage == WHOLE) { if (whole < maxWhole()) ++whole; }
        else                { if (fracIdx + 1u < Menu::kCountFrac) ++fracIdx; }
    }
    bool atFrac() const { return stage == FRAC; }
    void toFrac()  { stage = FRAC; }
    void toWhole() { stage = WHOLE; }

    /// Chosen value in metres.
    float meters() const
    {
        const float units = whole + fracIdx * Menu::kFracStep;
        const float km    = imperial ? SDK::Utils::milesToKm(units) : units;
        return km * 1000.0f;
    }

    void render(Widgets::TwoTonePicker& p) const
    {
        const bool leftActive = (stage == WHOLE);
        char left[8], right[8], up1[8] = "", up2[8] = "";
        snprintf(left, sizeof left, "%02u", whole);
        snprintf(right, sizeof right, "%02u", static_cast<unsigned>(fracIdx * hundStep()));
        if (leftActive) {
            if (whole + 1u <= maxWhole()) snprintf(up1, sizeof up1, "%02u", whole + 1u);
            if (whole + 2u <= maxWhole()) snprintf(up2, sizeof up2, "%02u", whole + 2u);
        } else {
            if (fracIdx + 1u < Menu::kCountFrac)
                snprintf(up1, sizeof up1, "%02u", static_cast<unsigned>((fracIdx + 1u) * hundStep()));
            if (fracIdx + 2u < Menu::kCountFrac)
                snprintf(up2, sizeof up2, "%02u", static_cast<unsigned>((fracIdx + 2u) * hundStep()));
        }
        p.renderSubtitleSingle(imperial ? "Miles" : "Kilometers");
        p.renderValue(leftActive, left, right, ".", up1, up2);
    }
};

// -----------------------------------------------------------------------------
// Time: minutes:seconds. Ranges and steps come from the descriptor.
// -----------------------------------------------------------------------------
template<typename Menu>
struct Time {
    enum Stage { MIN = 0, SEC };
    Stage    stage   = MIN;
    uint16_t minutes = 0;
    uint16_t seconds = 0;

    void seed(uint32_t totalSeconds)
    {
        uint16_t m = static_cast<uint16_t>(totalSeconds / 60u);
        uint16_t s = static_cast<uint16_t>(totalSeconds % 60u);
        if (m > Menu::kMaxMin) m = Menu::kMaxMin;
        s = static_cast<uint16_t>(((s + Menu::kStepSec / 2u) / Menu::kStepSec) * Menu::kStepSec);
        if (s > Menu::kMaxSec) s = Menu::kMaxSec;
        minutes = m;
        seconds = s;
        stage = MIN;
    }
    void dec()
    {
        if (stage == MIN) { if (minutes >= Menu::kStepMin) minutes -= Menu::kStepMin; }
        else              { if (seconds >= Menu::kStepSec) seconds -= Menu::kStepSec; }
    }
    void inc()
    {
        if (stage == MIN) { if (minutes + Menu::kStepMin <= Menu::kMaxMin) minutes += Menu::kStepMin; }
        else              { if (seconds + Menu::kStepSec <= Menu::kMaxSec) seconds += Menu::kStepSec; }
    }
    bool atSec() const { return stage == SEC; }
    void toSec() { stage = SEC; }
    void toMin() { stage = MIN; }
    uint32_t totalSeconds() const { return minutes * 60u + seconds; }

    void render(Widgets::TwoTonePicker& p) const
    {
        const bool leftActive = (stage == MIN);
        char left[8], right[8], up1[8] = "", up2[8] = "";
        snprintf(left, sizeof left, "%02u", minutes);
        snprintf(right, sizeof right, "%02u", seconds);
        if (leftActive) {
            if (minutes + Menu::kStepMin <= Menu::kMaxMin)
                snprintf(up1, sizeof up1, "%02u", minutes + Menu::kStepMin);
            if (minutes + 2u * Menu::kStepMin <= Menu::kMaxMin)
                snprintf(up2, sizeof up2, "%02u", minutes + 2u * Menu::kStepMin);
        } else {
            if (seconds + Menu::kStepSec <= Menu::kMaxSec)
                snprintf(up1, sizeof up1, "%02u", seconds + Menu::kStepSec);
            if (seconds + 2u * Menu::kStepSec <= Menu::kMaxSec)
                snprintf(up2, sizeof up2, "%02u", seconds + 2u * Menu::kStepSec);
        }
        p.renderSubtitleDual("Mins.", "Secs.", leftActive);
        p.renderValue(leftActive, left, right, ":", up1, up2);
    }
};

} // namespace PickerLogic

#endif // PICKER_LOGIC_HPP
