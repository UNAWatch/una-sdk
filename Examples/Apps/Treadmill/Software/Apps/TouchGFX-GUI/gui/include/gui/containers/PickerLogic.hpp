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
// the Menu descriptor (0.05 for the intervals pickers); the displayed hundredths
// are derived from it. Calibrate & Save uses DistanceFine below instead.
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
// DistanceFine: whole.tenth.hundredth, displayed in km or mi (Calibrate & Save).
//
// Where Distance above has two stages over a fixed fraction step, this keeps the
// value as one integer count of 0.01 units and lets each stage pick only the step
// size. Two consequences, both deliberate: a press at the top of a place carries
// into the place above it (5.99 +0.01 -> 6.00, one press) and the whole range
// wraps (99.99 +0.01 -> 0.00), so any treadmill-console reading is a few presses
// away in either direction.
// -----------------------------------------------------------------------------
template<typename Menu>
struct DistanceFine {
    enum Stage { WHOLE = 0, TENTH, HUNDREDTH };

    /// Number of 0.01-unit steps in the range; the value wraps modulo this.
    static constexpr uint16_t kRange = Menu::kCountHund;

    // Widening the range past 0..99.99 means widening the value type too: the
    // descriptor computes kCountHund in uint16_t, so it would silently truncate.
    static_assert(static_cast<uint32_t>(Menu::kMaxWhole + 1) * 100u <= UINT16_MAX,
                  "Picker range no longer fits uint16_t -- widen kCountHund and hund together");
    static_assert(kRange % Menu::kStepWhole == 0,
                  "The whole-unit step must divide the range, else wrapping skips values");

    Stage    stage    = WHOLE;
    bool     imperial = false;
    uint16_t hund     = 0;      ///< Value in hundredths of a display unit.

    void seed(float meters, bool isImperial)
    {
        imperial = isImperial;
        float units = meters / 1000.0f;            // km
        if (imperial) units = SDK::Utils::kmToMiles(units);

        // Ordered so that anything not provably in range -- negative, over-range,
        // or NaN, which compares false both ways -- avoids an out-of-range
        // float->integer cast rather than relying on the comparison to catch it.
        const float counts = units * 100.0f + 0.5f;
        if (!(counts >= 1.0f)) {
            hund = 0;
        } else if (counts >= static_cast<float>(kRange)) {
            hund = static_cast<uint16_t>(kRange - 1u);
        } else {
            hund = static_cast<uint16_t>(counts);
        }
        stage = WHOLE;
    }

    /// Units-of-0.01 moved by one press at the current stage.
    uint16_t step() const
    {
        switch (stage) {
        case WHOLE: return Menu::kStepWhole;
        case TENTH: return Menu::kStepTenth;
        default:    return Menu::kStepHund;
        }
    }

    void inc() { hund = wrapAdd(hund, step()); }
    void dec() { hund = wrapAdd(hund, static_cast<uint16_t>(kRange - step())); }

    /// Move to the next-finer place; false when already at the finest (= confirm).
    bool advance()
    {
        if (stage == HUNDREDTH) return false;
        stage = (stage == WHOLE) ? TENTH : HUNDREDTH;
        return true;
    }

    /// Step back to the next-coarser place; false when already at the coarsest.
    bool retreat()
    {
        if (stage == WHOLE) return false;
        stage = (stage == HUNDREDTH) ? TENTH : WHOLE;
        return true;
    }

    /// Chosen value in metres.
    float meters() const
    {
        const float units = hund / 100.0f;
        const float km    = imperial ? SDK::Utils::milesToKm(units) : units;
        return km * 1000.0f;
    }

    void render(TwoTonePicker& p) const
    {
        char whole[4], fracHi[4], fracLo[4], up1[4], up2[4];
        std::snprintf(whole, sizeof whole, "%02u", static_cast<unsigned>(hund / 100u));
        std::snprintf(fracHi, sizeof fracHi, "%u", static_cast<unsigned>((hund / 10u) % 10u));
        std::snprintf(fracLo, sizeof fracLo, "%u", static_cast<unsigned>(hund % 10u));

        // Wrapping turns the upcoming column into a circular list, so unlike the
        // clamped pickers both slots are always populated.
        activePlace(up1, wrapAdd(hund, step()));
        activePlace(up2, wrapAdd(hund, static_cast<uint16_t>(2u * step())));

        p.renderSubtitleSingle(imperial ? T_TEXT_MILES_SUB : T_TEXT_KILOMETERS);
        p.renderValue3(segment(), whole, fracHi, fracLo, up1, up2);
    }

private:
    static uint16_t wrapAdd(uint16_t value, uint16_t delta)
    {
        return static_cast<uint16_t>((value + delta) % kRange);
    }

    TwoTonePicker::Segment segment() const
    {
        switch (stage) {
        case WHOLE: return TwoTonePicker::SEG_WHOLE;
        case TENTH: return TwoTonePicker::SEG_FRAC_HI;
        default:    return TwoTonePicker::SEG_FRAC_LO;
        }
    }

    /// Format only the place being edited -- that is all the upcoming column shows.
    void activePlace(char (&out)[4], uint16_t value) const
    {
        switch (stage) {
        case WHOLE: std::snprintf(out, sizeof out, "%02u", static_cast<unsigned>(value / 100u)); break;
        case TENTH: std::snprintf(out, sizeof out, "%u", static_cast<unsigned>((value / 10u) % 10u)); break;
        default:    std::snprintf(out, sizeof out, "%u", static_cast<unsigned>(value % 10u)); break;
        }
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
