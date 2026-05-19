/**
 ******************************************************************************
 * @file    TrackInfo.hpp
 * @date    08-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Represents current track information.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef TRACK_HPP
#define TRACK_HPP

#include <cstdint>
#include <ctime>

namespace Track
{

/**
 * @brief Activity tracking state.
 */
enum class State {
    INACTIVE = 0,
    ACTIVE,
    PAUSED
};

/**
 * @brief Real-time metrics snapshot; updated every second by the Service.
 *
 * All distances are in metres, speeds in m/s, pace in s/m, times in seconds.
 * The GUI converts to display units as needed.
 */
struct Data {

    // Pace, s/m
    float pace        = 0.0f;
    float avgPace     = 0.0f;
    float lapPace     = 0.0f;

    // Distance, m
    float distance    = 0.0f;
    float lapDistance = 0.0f;

    // Time, s
    std::time_t totalTime = 0;
    std::time_t lapTime   = 0;

    uint32_t lapNum = 0;

    // Heart rate, bpm
    float hr           = 0.0f;
    float hrTrustLevel = 0.0f;
    float avgHR        = 0.0f;
    float maxHR        = 0.0f;
    float avgLapHR     = 0.0f;
    float maxLapHR     = 0.0f;

    // Speed, m/s
    float speed       = 0.0f;
    float avgSpeed    = 0.0f;
    float maxSpeed    = 0.0f;
    float avgLapSpeed = 0.0f;
    float maxLapSpeed = 0.0f;

    // Elevation, m
    float elevation    = 0.0f;
};

} // namespace Track

#endif // TRACK_HPP
