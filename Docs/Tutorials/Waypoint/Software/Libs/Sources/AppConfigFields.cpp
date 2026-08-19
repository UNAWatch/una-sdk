/**
 ******************************************************************************
 * @file    AppConfigFields.cpp
 * @brief   The app's copy of the configuration contract in its app-manifest.json.
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "AppConfigFields.hpp"

namespace WaypointConfig {

using SDK::AppConfig;

// Every value here must match Output/app-manifest.json exactly. CI checks it with
// validate_app_config.py --check <app-manifest.json> --check-bounds <this file>.
const AppConfig::Field kFields[] = {
    AppConfig::stringField("waypointName", "Waypoint", 1, 16),
    AppConfig::floatField("targetLatitude", 51.5072f, -90.0f, 90.0f),
    AppConfig::floatField("targetLongitude", -0.1276f, -180.0f, 180.0f),
    AppConfig::intField("arrivalRadiusM", 25, 5, 500),
    AppConfig::boolField("vibrateOnArrival", true),
};

const size_t kFieldCount = sizeof(kFields) / sizeof(kFields[0]);

} // namespace WaypointConfig
