#ifndef RUNALERTSUI_HPP
#define RUNALERTSUI_HPP

#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/hal/Types.hpp>

#include "RunAlertsPickers.hpp"

/**
 * Settings > Run Alerts: the screen titles for the field being edited, and
 * the run alert pop-up's title and unit, as the TouchGFX texts. The values
 * and hints come from RunAlertsPickers.hpp, shared with RunLVGL.
 */
namespace RunAlertsUi {

inline touchgfx::TypedTextId title(RunAlerts::Field field)
{
    switch (field) {
    case RunAlerts::Field::RunTime:  return T_TEXT_RUN_TIME_UC;
    case RunAlerts::Field::WalkTime: return T_TEXT_WALK_TIME_UC;
    case RunAlerts::Field::PaceFast: return T_TEXT_TOO_FAST_UC;
    case RunAlerts::Field::PaceSlow: return T_TEXT_TOO_SLOW_UC;
    case RunAlerts::Field::HrHigh:   return T_TEXT_HR_HIGH_UC;
    case RunAlerts::Field::HrLow:    return T_TEXT_HR_LOW_UC;
    }
    return T_TEXT_RUN_ALERTS_UC;
}

/// The pop-up's title, as RunAlerts::alertTitle().
inline touchgfx::TypedTextId alertTitle(Track::RunAlertKind kind)
{
    switch (kind) {
    case Track::RunAlertKind::PACE_FAST: return T_TEXT_ALERT_TOO_FAST;
    case Track::RunAlertKind::PACE_SLOW: return T_TEXT_ALERT_TOO_SLOW;
    case Track::RunAlertKind::HR_HIGH:   return T_TEXT_ALERT_HR_HIGH;
    case Track::RunAlertKind::HR_LOW:    return T_TEXT_ALERT_HR_LOW;
    }
    return T_TEXT_RUN_ALERTS;
}

/// The unit under the pop-up's value, as RunAlerts::alertUnit().
inline touchgfx::TypedTextId alertUnit(Track::RunAlertKind kind, bool imperial)
{
    if (RunAlerts::alertIsPace(kind)) {
        return imperial ? T_TEXT_PER_MI : T_TEXT_PER_KM;
    }
    return T_TEXT_BPM;
}

} // namespace RunAlertsUi

#endif // RUNALERTSUI_HPP
