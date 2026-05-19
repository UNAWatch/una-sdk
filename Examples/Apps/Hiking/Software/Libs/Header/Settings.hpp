/**
 ******************************************************************************
 * @file    Settings.hpp
 * @date    08-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Application settings structure and alert sub-types.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <cstdint>

/**
 * @brief Application settings persisted to storage.
 */
struct Settings {
    /// Settings version.
    static constexpr uint8_t kVersion = 1;

    struct Alerts {
        struct Distance {
            enum Id : uint8_t {
                ID_OFF = 0, ID_KM_MILL_1, ID_KM_MILL_2, ID_KM_MILL_3,
                ID_KM_MILL_4, ID_KM_MILL_5, ID_KM_MILL_10,
                ID_COUNT, ID_DEFAULT = ID_KM_MILL_1
            };
            static constexpr uint16_t kValues[ID_COUNT] = { 0, 1, 2, 3, 4, 5, 10 };

            static float toMeters(Id id, bool isImperial) {
                return kValues[id] * (isImperial ? 1609.34f : 1000.0f);
            }
        };

        struct Time {
            enum Id : uint8_t {
                ID_OFF = 0, ID_MIN_10, ID_MIN_20, ID_MIN_30, ID_MIN_60,
                ID_COUNT, ID_DEFAULT = ID_OFF
            };
            static constexpr uint16_t kValues[ID_COUNT] = { 0, 10, 20, 30, 60 };

            static constexpr uint32_t toSeconds(Id id) {
                return kValues[id] * 60u;
            }
        };
    };

    // Fields
    uint32_t version = kVersion;    ///< Settings version.

    bool     autoPauseEn  = false;  ///< Flag to enable auto pause during activity track.
    bool     phoneNotifEn = true;   ///< Flag to enable receiving phone notification when app is run.
    Alerts::Distance::Id alertDistanceId = Alerts::Distance::ID_OFF;    ///< Distance alert option.
    Alerts::Time::Id     alertTimeId     = Alerts::Time::ID_OFF;        ///< Time alert option.
};

#endif // SETTINGS_HPP
