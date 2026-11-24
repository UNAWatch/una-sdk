/**
 ******************************************************************************
 * @file    RunAlertsPickers.hpp
 * @brief   Settings > Run Alerts: the pickers' rules, shared by both GUIs.
 *
 * Which values each Run Alerts picker offers, where it opens, how a pick is
 * stored, and the menu hints; also the run alert pop-up's texts. Header-only,
 * because the GUI images compile no Libs sources. Pace is kept in s/km
 * (Settings::RunAlerts) and picked in min:sec per km or per mile, following
 * the units setting.
 ******************************************************************************
 */

#ifndef RUN_ALERTS_PICKERS_HPP
#define RUN_ALERTS_PICKERS_HPP

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "Settings.hpp"
#include "Track.hpp"

namespace RunAlerts {

/// A setting the Run Alerts pickers edit.
enum class Field : uint8_t {
    RunTime,   ///< Run/Walk run time, s
    WalkTime,  ///< Run/Walk walk time, s
    PaceFast,  ///< Too Fast, min:sec per km or mi
    PaceSlow,  ///< Too Slow, min:sec per km or mi
    HrHigh,    ///< HR High, bpm
    HrLow,     ///< HR Low, bpm
};

inline bool isPace(Field f) { return f == Field::PaceFast || f == Field::PaceSlow; }
inline bool isHr(Field f)   { return f == Field::HrHigh || f == Field::HrLow; }

constexpr float    kKmPerMile          = 1.609344f;
constexpr uint32_t kStepSec            = 5;     ///< The time and pace pickers' seconds step
constexpr uint32_t kPaceMinDisplaySec  = 120;   ///< 2:00 per km or mi
constexpr uint32_t kPaceMaxDisplaySec  = 1200;  ///< 20:00 per km or mi
constexpr uint32_t kPaceFastSuggestSec = 270;   ///< 4:30, where Too Fast opens when Off
constexpr uint32_t kPaceSlowSuggestSec = 360;   ///< 6:00, where Too Slow opens when Off
constexpr uint32_t kHrListMinBpm       = 40;
constexpr uint32_t kHrListMaxBpm       = 220;

namespace Detail {
inline uint32_t floorStep(uint32_t v)   { return v / kStepSec * kStepSec; }
inline uint32_t ceilStep(uint32_t v)    { return (v + kStepSec - 1u) / kStepSec * kStepSec; }
inline uint32_t nearestStep(uint32_t v) { return (v + kStepSec / 2u) / kStepSec * kStepSec; }
} // namespace Detail

/// A pace kept in s/km, as min:sec per km or mi to the nearest second. 0 (Off) stays 0.
inline uint32_t paceToDisplay(float secPerKm, bool imperial)
{
    if (!(secPerKm > 0.0f)) {
        return 0;
    }
    const float s = imperial ? secPerKm * kKmPerMile : secPerKm;
    return static_cast<uint32_t>(std::lround(s));
}

/// A pace picked per km or mi, in s/km. Not rounded, so a pace per mile
/// shows again exactly as it was picked.
inline float paceFromDisplay(uint32_t secPerUnit, bool imperial)
{
    const float s = static_cast<float>(secPerUnit);
    return imperial ? s / kKmPerMile : s;
}

/// What a picker may offer: seconds for the times, seconds per km or mi for
/// a pace, bpm for heart rate. Each limit is narrowed by the other one, so an
/// inverted or equal range can't be picked. Empty (min > max) when nothing
/// can be picked: Too Fast with Too Slow at 2:00, say.
struct Bounds {
    uint32_t min;
    uint32_t max;
    bool empty() const { return min > max; }
};

inline Bounds pickerBounds(Field field, const Settings::RunAlerts& cfg, bool imperial)
{
    using RA = Settings::RunAlerts;

    switch (field) {
    case Field::RunTime:
    case Field::WalkTime:
        return Bounds{Detail::ceilStep(RA::kRunWalkMinSec), Detail::floorStep(RA::kRunWalkMaxSec)};

    case Field::PaceFast: {
        // At least a step quicker than Too Slow.
        Bounds b{kPaceMinDisplaySec, kPaceMaxDisplaySec};
        const uint32_t slow = paceToDisplay(cfg.paceSlowSecPerKm, imperial);
        if (slow > 0) {
            const uint32_t lim = slow > kStepSec ? Detail::floorStep(slow - kStepSec) : 0u;
            if (lim < b.max) {
                b.max = lim;
            }
        }
        return b;
    }
    case Field::PaceSlow: {
        // At least a step slower than Too Fast.
        Bounds b{kPaceMinDisplaySec, kPaceMaxDisplaySec};
        const uint32_t fast = paceToDisplay(cfg.paceFastSecPerKm, imperial);
        if (fast > 0) {
            const uint32_t lim = Detail::ceilStep(fast + kStepSec);
            if (lim > b.min) {
                b.min = lim;
            }
        }
        return b;
    }
    case Field::HrHigh: {
        // Above HR Low.
        Bounds b{kHrListMinBpm, kHrListMaxBpm};
        if (cfg.hrLowBpm > 0 && cfg.hrLowBpm + 1u > b.min) {
            b.min = cfg.hrLowBpm + 1u;
        }
        return b;
    }
    case Field::HrLow: {
        // Below HR High.
        Bounds b{kHrListMinBpm, kHrListMaxBpm};
        if (cfg.hrHighBpm > 0 && cfg.hrHighBpm - 1u < b.max) {
            b.max = cfg.hrHighBpm - 1u;
        }
        return b;
    }
    }
    return Bounds{0, 0};
}

/// Where a picker opens: the current value, or when it is Off a suggestion
/// (Too Fast 4:30, Too Slow 6:00; HR High at the start of zone 5 and HR Low
/// at the start of zone 3, from the watch's thresholds), on the picker's step
/// and within its bounds.
inline uint32_t pickerSeed(Field field, const Settings::RunAlerts& cfg, bool imperial,
                           const uint8_t* hrThresholds, uint8_t hrThresholdsCount)
{
    // Zone n starts at threshold n-1 (Service::hrBand()).
    const auto zoneStart = [&](uint8_t zone) -> uint32_t {
        return (hrThresholds != nullptr && zone >= 1 && zone <= hrThresholdsCount)
            ? hrThresholds[zone - 1]
            : 0u;
    };

    uint32_t v = 0;
    switch (field) {
    case Field::RunTime:
        v = Detail::nearestStep(cfg.runWalkRunSec);
        break;
    case Field::WalkTime:
        v = Detail::nearestStep(cfg.runWalkWalkSec);
        break;
    case Field::PaceFast:
        v = paceToDisplay(cfg.paceFastSecPerKm, imperial);
        v = v > 0 ? Detail::nearestStep(v) : kPaceFastSuggestSec;
        break;
    case Field::PaceSlow:
        v = paceToDisplay(cfg.paceSlowSecPerKm, imperial);
        v = v > 0 ? Detail::nearestStep(v) : kPaceSlowSuggestSec;
        break;
    case Field::HrHigh:
        v = cfg.hrHighBpm > 0 ? cfg.hrHighBpm : zoneStart(5);
        break;
    case Field::HrLow:
        v = cfg.hrLowBpm > 0 ? cfg.hrLowBpm : zoneStart(3);
        break;
    }
    const Bounds b = pickerBounds(field, cfg, imperial);
    if (b.empty() || v < b.min) {
        return b.min;
    }
    return v > b.max ? b.max : v;
}

/// Store a picked @p value (as pickerBounds() gives it). 0 turns a pace or
/// heart-rate limit Off.
inline void setField(Field field, uint32_t value, bool imperial, Settings::RunAlerts& cfg)
{
    switch (field) {
    case Field::RunTime:
        cfg.runWalkRunSec = value;
        break;
    case Field::WalkTime:
        cfg.runWalkWalkSec = value;
        break;
    case Field::PaceFast:
        cfg.paceFastSecPerKm = value > 0 ? paceFromDisplay(value, imperial) : 0.0f;
        break;
    case Field::PaceSlow:
        cfg.paceSlowSecPerKm = value > 0 ? paceFromDisplay(value, imperial) : 0.0f;
        break;
    case Field::HrHigh:
        cfg.hrHighBpm = static_cast<uint8_t>(value > 255u ? 255u : value);
        break;
    case Field::HrLow:
        cfg.hrLowBpm = static_cast<uint8_t>(value > 255u ? 255u : value);
        break;
    }
}

/// The field's value as the pickers show it: seconds, display seconds per km
/// or mi, or bpm; 0 for a limit that is Off.
inline uint32_t fieldValue(Field field, const Settings::RunAlerts& cfg, bool imperial)
{
    switch (field) {
    case Field::RunTime:  return cfg.runWalkRunSec;
    case Field::WalkTime: return cfg.runWalkWalkSec;
    case Field::PaceFast: return paceToDisplay(cfg.paceFastSecPerKm, imperial);
    case Field::PaceSlow: return paceToDisplay(cfg.paceSlowSecPerKm, imperial);
    case Field::HrHigh:   return cfg.hrHighBpm;
    case Field::HrLow:    return cfg.hrLowBpm;
    }
    return 0;
}

/// "4:30" for a pace in display seconds, "--:--" for 0. A pace slower than
/// 99:59 (a runner all but standing) shows as 99:59, so the text is never
/// longer than 5 characters.
inline void paceText(uint32_t displaySec, char* out, size_t size)
{
    if (displaySec == 0) {
        std::snprintf(out, size, "--:--");
    } else {
        const uint32_t sec = displaySec < 6000u ? displaySec : 5999u;
        std::snprintf(out, size, "%u:%02u", static_cast<unsigned>(sec / 60u), static_cast<unsigned>(sec % 60u));
    }
}

/// The Pace row's hint: "4:30-6:00", "Fast 4:30", "Slow 6:00", or "" when both are Off.
inline void paceRangeText(const Settings::RunAlerts& cfg, bool imperial, char* out, size_t size)
{
    const uint32_t fast = paceToDisplay(cfg.paceFastSecPerKm, imperial);
    const uint32_t slow = paceToDisplay(cfg.paceSlowSecPerKm, imperial);
    char f[8];
    char s[8];
    paceText(fast, f, sizeof(f));
    paceText(slow, s, sizeof(s));
    if (fast > 0 && slow > 0) {
        std::snprintf(out, size, "%s-%s", f, s);
    } else if (fast > 0) {
        std::snprintf(out, size, "Fast %s", f);
    } else if (slow > 0) {
        std::snprintf(out, size, "Slow %s", s);
    } else if (size > 0) {
        out[0] = '\0';
    }
}

/// The Heart Rate row's hint: "120-165", "High 165", "Low 120", or "" when both are Off.
inline void hrRangeText(const Settings::RunAlerts& cfg, char* out, size_t size)
{
    const unsigned high = cfg.hrHighBpm;
    const unsigned low  = cfg.hrLowBpm;
    if (high > 0 && low > 0) {
        std::snprintf(out, size, "%u-%u", low, high);
    } else if (high > 0) {
        std::snprintf(out, size, "High %u", high);
    } else if (low > 0) {
        std::snprintf(out, size, "Low %u", low);
    } else if (size > 0) {
        out[0] = '\0';
    }
}

/// A field's value as a menu hint or the Saved screen shows it: "04:00 min",
/// "4:30 /km", "165 bpm", or "OFF" for a limit that is Off.
inline void fieldText(Field field, const Settings::RunAlerts& cfg, bool imperial, char* out, size_t size)
{
    const uint32_t v = fieldValue(field, cfg, imperial);
    if (field == Field::RunTime || field == Field::WalkTime) {
        std::snprintf(out, size, "%02u:%02u min", static_cast<unsigned>(v / 60u), static_cast<unsigned>(v % 60u));
    } else if (v == 0) {
        std::snprintf(out, size, "OFF");
    } else if (isPace(field)) {
        char p[8];
        paceText(v, p, sizeof(p));
        std::snprintf(out, size, "%s /%s", p, imperial ? "mi" : "km");
    } else {
        std::snprintf(out, size, "%u bpm", static_cast<unsigned>(v));
    }
}

/// The screen title for a field: "RUN TIME", "WALK TIME", "TOO FAST",
/// "TOO SLOW", "HR HIGH" or "HR LOW".
inline const char* fieldTitle(Field field)
{
    switch (field) {
    case Field::RunTime:  return "RUN TIME";
    case Field::WalkTime: return "WALK TIME";
    case Field::PaceFast: return "TOO FAST";
    case Field::PaceSlow: return "TOO SLOW";
    case Field::HrHigh:   return "HR HIGH";
    case Field::HrLow:    return "HR LOW";
    }
    return "";
}

// =============================================================================
// The run alert pop-up
// =============================================================================

inline bool alertIsPace(Track::RunAlertKind kind)
{
    return kind == Track::RunAlertKind::PACE_FAST || kind == Track::RunAlertKind::PACE_SLOW;
}

/// The pop-up's title: "Too fast", "Too slow", "HR high" or "HR low".
inline const char* alertTitle(Track::RunAlertKind kind)
{
    switch (kind) {
    case Track::RunAlertKind::PACE_FAST: return "Too fast";
    case Track::RunAlertKind::PACE_SLOW: return "Too slow";
    case Track::RunAlertKind::HR_HIGH:   return "HR high";
    case Track::RunAlertKind::HR_LOW:    return "HR low";
    }
    return "";
}

/// The pop-up's value. @p value is as CustomMessage::RunAlert carries it: a
/// pace in s/m, shown per km or mi ("5:42", "--:--" with no pace), or the
/// heart rate in bpm ("165").
inline void alertValueText(Track::RunAlertKind kind, float value, bool imperial, char* out, size_t size)
{
    if (alertIsPace(kind)) {
        paceText(paceToDisplay(value * 1000.0f, imperial), out, size);
    } else {
        const long bpm = value > 0.0f ? std::lround(value) : 0;
        std::snprintf(out, size, "%ld", bpm);
    }
}

/// The unit under the pop-up's value: "/km", "/mi" or "bpm".
inline const char* alertUnit(Track::RunAlertKind kind, bool imperial)
{
    if (alertIsPace(kind)) {
        return imperial ? "/mi" : "/km";
    }
    return "bpm";
}

} // namespace RunAlerts

#endif // RUN_ALERTS_PICKERS_HPP
