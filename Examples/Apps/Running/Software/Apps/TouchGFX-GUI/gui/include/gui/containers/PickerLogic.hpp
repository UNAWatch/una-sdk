#ifndef PICKERLOGIC_HPP
#define PICKERLOGIC_HPP

#include <cstdint>
#include <cstdio>
#include <gui/containers/TwoTonePicker.hpp>
#include <gui/model/AppMenu.hpp>
#include <SDK/Utils/Utils.hpp>
#include <texts/TextKeysAndLanguages.hpp>

/**
 * Headless state + rendering logic shared by the Treadmill two-stage pickers, so
 * each View stays a thin shell that only wires keys to a save action. The View
 * owns one of these structs plus the generated `picker` (a TwoTonePicker) and
 * calls render(picker) on every change.
 */
namespace PickerLogic {

// -----------------------------------------------------------------------------
// Distance: whole.fraction, displayed in km or mi. The fraction step comes from
// the Menu descriptor (0.05 for intervals, 0.01 for calibrate); the displayed
// hundredths are derived from it so one template covers both.
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
        float units = meters / 1000.0f;            // km
        if (imperial) units = SDK::Utils::kmToMiles(units);

        whole = static_cast<uint16_t>(units);
        if (whole > maxWhole()) whole = maxWhole();

        float frac = units - whole;
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

    void render(TwoTonePicker& p) const
    {
        const bool leftActive = (stage == WHOLE);
        char left[8], right[8], up1[8] = "", up2[8] = "";
        std::snprintf(left, sizeof left, "%02u", whole);
        std::snprintf(right, sizeof right, "%02u", static_cast<unsigned>(fracIdx * hundStep()));

        if (leftActive) {
            if (whole + 1u <= maxWhole()) std::snprintf(up1, sizeof up1, "%02u", whole + 1u);
            if (whole + 2u <= maxWhole()) std::snprintf(up2, sizeof up2, "%02u", whole + 2u);
        } else {
            if (fracIdx + 1u < Menu::kCountFrac)
                std::snprintf(up1, sizeof up1, "%02u", static_cast<unsigned>((fracIdx + 1u) * hundStep()));
            if (fracIdx + 2u < Menu::kCountFrac)
                std::snprintf(up2, sizeof up2, "%02u", static_cast<unsigned>((fracIdx + 2u) * hundStep()));
        }

        p.renderSubtitleSingle(imperial ? T_TEXT_MILES_SUB : T_TEXT_KILOMETERS);
        p.renderValue(leftActive, left, right, ".", up1, up2);
    }
};

// -----------------------------------------------------------------------------
// Time: minutes:seconds. Minute/second ranges + steps come from the Menu.
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
        s = ((s + Menu::kStepSec / 2u) / Menu::kStepSec) * Menu::kStepSec;
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

    void render(TwoTonePicker& p) const
    {
        const bool leftActive = (stage == MIN);
        char left[8], right[8], up1[8] = "", up2[8] = "";
        std::snprintf(left, sizeof left, "%02u", minutes);
        std::snprintf(right, sizeof right, "%02u", seconds);

        if (leftActive) {
            if (minutes + Menu::kStepMin <= Menu::kMaxMin)
                std::snprintf(up1, sizeof up1, "%02u", minutes + Menu::kStepMin);
            if (minutes + 2u * Menu::kStepMin <= Menu::kMaxMin)
                std::snprintf(up2, sizeof up2, "%02u", minutes + 2u * Menu::kStepMin);
        } else {
            if (seconds + Menu::kStepSec <= Menu::kMaxSec)
                std::snprintf(up1, sizeof up1, "%02u", seconds + Menu::kStepSec);
            if (seconds + 2u * Menu::kStepSec <= Menu::kMaxSec)
                std::snprintf(up2, sizeof up2, "%02u", seconds + 2u * Menu::kStepSec);
        }

        p.renderSubtitleDual(T_TEXT_MINS, T_TEXT_SECS, leftActive);
        p.renderValue(leftActive, left, right, ":", up1, up2);
    }
};

} // namespace PickerLogic

#endif // PICKERLOGIC_HPP
