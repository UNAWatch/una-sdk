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

#include "SDK/TrackMap/TrackMapScreen.hpp"

/**
 * @struct LapSummary
 * @brief Summary data for a single lap.
 */
struct LapSummary {
    time_t duration;  ///< Lap duration in seconds
    float  distance;  ///< Lap distance in m
    float  speed;     ///< Average speed in m/s
};

/**
 * @struct ActivitySummary
 * @brief Represents a common summary track information.
 */
struct ActivitySummary {
    time_t utc;         ///< Last activity UTC time
    time_t time;        ///< Total track time in seconds
    float distance;     ///< Total track distance in m
    float speedAvg;     ///< Average speed in m/s
    float elevation;    ///< Elevation in m
    float paceAvg;      ///< Average pace in s/m
    float hrMax;        ///< Maximum Heart Rate in bpm
    float hrAvg;        ///< Average Heart Rate in bpm
    SDK::TrackMapScreen map; ///< Track map

    std::vector<LapSummary> laps; ///< Per-lap summary data
};

#endif // ACTIVITY_SUMMARY_HPP
