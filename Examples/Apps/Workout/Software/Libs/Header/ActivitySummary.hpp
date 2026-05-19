/**
 ******************************************************************************
 * @file    ActivitySummary.hpp
 * @date    08-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Represents a common summary track information.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef ACTIVITY_SUMMARY_HPP
#define ACTIVITY_SUMMARY_HPP

#include <cstdint>
#include <ctime>
#include <vector>

/**
 * @struct LapSummary
 * @brief Summary data for a single lap.
 */
struct LapSummary {
    time_t duration; ///< Lap duration in seconds
    float  hrAvg;    ///< Lap average heart rate (bpm)
    float  hrMax;    ///< Lap maximum heart rate (bpm)
};

/**
 * @struct ActivitySummary
 * @brief Represents a common summary track information.
 */
struct ActivitySummary {
    time_t utc;         ///< Last activity UTC time
    time_t time;        ///< Total track time in seconds
    float hrMax;        ///< Maximum Heart Rate in bpm
    float hrAvg;        ///< Average Heart Rate in bpm
    /// Total energy (kcal): zone-weighted MET model including MET 1.0 when HR is out of zone.
    float calories = 0.0f;
    /// Basal (BMR) component at MET 1.0 every active second (matches FIT session metabolic_calories).
    float restingCalories = 0.0f;
    /// Calories above basal from the zone MET model: @c calories - @c restingCalories (clamped >= 0).
    float activeCalories = 0.0f;

    /// Cumulative seconds spent in HR zones 1..5 (0-indexed).
    /// Active seconds with HR below the zone-1 threshold are NOT included here; the
    /// HR-zones summary screen lumps them into the zone-1 row for display only.
    std::time_t zoneTimeSec[5] = {0, 0, 0, 0, 0};

    std::vector<LapSummary> laps; ///< Per-lap summary data
};

#endif // ACTIVITY_SUMMARY_HPP
