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

    /// Lanes on a track; a lane of 0 is "No lane" (the road).
    static constexpr uint8_t kLanes = 8;

    struct Alerts {
        /// Auto-lap by distance, in metres on any watch: a track is
        /// measured in metres. The distance is the runner's, so in an outer
        /// lane a 400 m lap ends a little further round each time.
        struct Distance {
            enum Id : uint8_t {
                ID_OFF = 0, ID_M_200, ID_M_400, ID_M_800, ID_M_1000, ID_M_1600,
                ID_COUNT, ID_DEFAULT = ID_M_400
            };
            static constexpr uint16_t kValues[ID_COUNT] = { 0, 200, 400, 800, 1000, 1600 };

            static float toMeters(Id id, bool /*isImperial*/) {
                return static_cast<float>(kValues[id]);
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
        bool     lastRest      = true;  ///< Include the REST after the final interval; false -> skip straight to COOL_DOWN.
    };


    /**
     * @struct RunAlerts
     * @brief Settings > Run Alerts: the Run/Walk start, and the pace and
     *        heart-rate range alerts. See RunAlerts.hpp.
     *
     * Pace is kept in s/km whatever the units setting. A pace or heart-rate
     * limit of 0 is Off. The ranges below are what the settings file accepts;
     * outside them a limit loads as Off and a Run/Walk time as its default.
     */
    struct RunAlerts {
        static constexpr uint32_t kRunWalkMinSec   = 5;
        static constexpr uint32_t kRunWalkMaxSec   = 3600;
        static constexpr float    kPaceMinSecPerKm = 60.0f;
        static constexpr float    kPaceMaxSecPerKm = 1800.0f;
        static constexpr uint8_t  kHrMinBpm        = 30;
        static constexpr uint8_t  kHrMaxBpm        = 240;

        bool     runWalkEn        = false;  ///< A plain Start runs Run/Walk on the intervals engine.
        uint32_t runWalkRunSec    = 240;    ///< Run phase, s (4:00).
        uint32_t runWalkWalkSec   = 60;     ///< Walk phase, s (1:00).
        float    paceFastSecPerKm = 0.0f;   ///< Too Fast: alert at a quicker pace, s/km. 0 - Off
        float    paceSlowSecPerKm = 0.0f;   ///< Too Slow: alert at a slower pace, s/km. 0 - Off
        uint8_t  hrHighBpm        = 0;      ///< HR High, bpm. 0 - Off
        uint8_t  hrLowBpm         = 0;      ///< HR Low, bpm. 0 - Off
        bool     backInTap        = false;  ///< A vibration tap on coming back in range (pace and HR).
    };


    // Fields
    uint32_t version = kVersion;    ///< Settings version.

    bool autoPauseEn = false; ///< Serialized only; not on settings wheel until auto-pause is implemented.
    bool     phoneNotifEn  = true;  ///< Flag to enable receiving phone notification when app is run.
    bool     calibTraceEn  = false; ///< Debug: write per-tick outdoor-stride calibrator CSV trace.
    Alerts::Distance::Id alertDistanceId = Alerts::Distance::ID_DEFAULT; ///< Distance alert option.
    Alerts::Time::Id     alertTimeId     = Alerts::Time::ID_OFF;     ///< Time alert option.

    Intervals intervals {};  ///< Interval training settings.

    RunAlerts runAlerts {};  ///< Run/Walk, pace and heart-rate alerts.
    uint8_t   lastLane   = 1;     ///< The lane Start uses, 1 to kLanes, or 0: "No lane" (the service owns it)
    bool      laneHoldEn = true;  ///< Hold Lap during a run for the lane picker

};

#endif // SETTINGS_HPP
