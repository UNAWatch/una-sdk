/**
 ******************************************************************************
 * @file    AppConfigFields.hpp
 * @brief   The app's copy of the configuration contract in its config.json.
 * @details config.json never reaches the watch, so the app repeats the id,
 *          type, default and bounds of every field here. CI compares the two
 *          (validate_app_config.py --check-bounds) and fails the build if they
 *          disagree, which is what makes it safe for SDK::AppConfig to clamp a
 *          value it should never have received.
 *
 *          Keep the table in one file, one entry per line, with plain literals:
 *          that is the form the checker parses.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef APP_CONFIG_FIELDS_HPP
#define APP_CONFIG_FIELDS_HPP

#include "SDK/AppConfig/AppConfig.hpp"

#include <cstddef>

namespace WaypointConfig {

/// The values file the companion app writes, as declared by "configFile".
constexpr const char *kFileName = "app_config.json";

/// Longest waypoint name in bytes (the field's maxLength), plus a terminator.
constexpr size_t kNameBytes = 16 + 1;

/// The declaration, mirroring config.json's "configFields".
extern const SDK::AppConfig::Field kFields[];
extern const size_t kFieldCount;

} // namespace WaypointConfig

#endif // APP_CONFIG_FIELDS_HPP
