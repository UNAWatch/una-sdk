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



    /**
     * @struct Intervals
     * @brief Interval training configuration (WARM_UP-RUN-REST-COOL_DOWN cycles).
     */
    struct Intervals {

        static constexpr uint8_t kRepeatsMax = 20; ///< Maximum number of RUN-REST repetitions.

        /**
         * @brief Metric used to measure the duration of a phase.
         */
        enum Metric {
            OPEN = 0,   ///< Phase has no duration limit.
            TIME,       ///< Phase duration is measured by time.
            DISTANCE,   ///< Phase duration is measured by distance.
        };

        uint8_t  repeatsNum    = 0;     ///< Number of repetitions of RUN-REST cycles. 0 - 'Open'

        Metric   runMetric     = Metric::OPEN; ///< Metric used to measure the RUN phase duration.
        uint32_t runTime       = 0;     ///< Duration of the RUN phase in seconds. 0 - 'Open'
        float    runDistance   = 0.0f;  ///< Duration of the RUN phase in meters. 0 - 'Open'

        Metric   restMetric    = Metric::OPEN; ///< Metric used to measure the REST phase duration.
        uint32_t restTime      = 0;     ///< Duration of the REST phase in seconds. 0 - 'Open'
        float    restDistance  = 0.0f;  ///< Duration of the REST phase in meters. 0 - 'Open'

        bool     warmUp        = true;  ///< Enable WARM_UP phase.
        bool     coolDown      = true;  ///< Enable COOL_DOWN phase.
    };


    // Fields
    uint32_t version = kVersion;    ///< Settings version.

    bool     autoPauseEn   = false; ///< Flag to enable auto pause during activity track.
    bool     phoneNotifEn  = true;  ///< Flag to enable receiving phone notification when app is run.
    Alerts::Distance::Id alertDistanceId = Alerts::Distance::ID_OFF; ///< Distance alert option.
    Alerts::Time::Id     alertTimeId     = Alerts::Time::ID_OFF;     ///< Time alert option.

    Intervals intervals {};  ///< Interval training settings.

};

#endif // SETTINGS_HPP
