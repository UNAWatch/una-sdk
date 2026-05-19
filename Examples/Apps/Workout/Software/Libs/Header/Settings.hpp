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
        struct Time {
            enum Id : uint8_t {
                ID_OFF = 0, ID_MIN_1, ID_MIN_5, ID_MIN_10,
                ID_MIN_15, ID_MIN_20, ID_MIN_30,
                ID_COUNT, ID_DEFAULT = ID_OFF
            };
            static constexpr uint16_t kValues[ID_COUNT] = { 0, 1, 5, 10, 15, 20, 30 };

            static constexpr uint32_t toSeconds(Id id) {
                return kValues[id] * 60u;
            }
        };
    };


    // Fields
    uint32_t version = kVersion;    ///< Settings version.

    bool     phoneNotifEn  = true;  ///< Flag to enable receiving phone notification when app is run.
    Alerts::Time::Id     alertTimeId     = Alerts::Time::ID_OFF;     ///< Time alert option.
};

#endif // SETTINGS_HPP
