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
    float  paceAvg;   ///< Average pace in s/m
};

/**
 * @struct TrackRunSummary
 * @brief  What the summary adds for Track Run: the track page.
 */
struct TrackRunSummary {
    uint8_t  trackNumber = 0;     ///< The stored track used, 0 if none
    uint8_t  savedNumber = 0;     ///< A new track saved as this number, 0 if none
    bool     onTrack     = false; ///< Any of the run was on a track
    bool     notStandard = false; ///< "Not a standard track" (counting only)
    bool     calLocked   = false; ///< Calibration locked in this run (a new track, or a visit blended in)
    uint8_t  laneMin     = 0;     ///< Lanes run on the track (laneMin == laneMax: one lane)
    uint8_t  laneMax     = 0;
    uint8_t  laneLast    = 0;     ///< The lane at the end
    float    roadM       = 0.0f;  ///< Distance off the track
    float    trackM      = 0.0f;  ///< Distance on the track
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
    float elevation;    ///< Accumulated elevation gain (ascent) in m
    float paceAvg;      ///< Average pace in s/m
    float hrMax;        ///< Maximum Heart Rate in bpm
    float hrAvg;        ///< Average Heart Rate in bpm
    SDK::TrackMapScreen map; ///< Track map

    std::vector<LapSummary> laps; ///< Per-lap summary data

    TrackRunSummary trackRun {};
};

#endif // ACTIVITY_SUMMARY_HPP
