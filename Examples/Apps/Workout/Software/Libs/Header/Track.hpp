/**
 ******************************************************************************
 * @file    Track.hpp
 * @date    08-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Track namespace: state and real-time metrics.
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
 */
struct Data {
    // Time, s
    std::time_t totalTime = 0;
    std::time_t lapTime   = 0;

    uint32_t lapNum = 0;

    // Heart rate, bpm
    float hr            = 0.0f;
    float hrTrustLevel  = 0.0f;
    float avgHR         = 0.0f;
    float maxHR         = 0.0f;
    float avgLapHR      = 0.0f;
    float maxLapHR      = 0.0f;
    uint8_t hrZone         = 0;   ///< 0 = none, 1..5 = active zone
    float totalCalories    = 0.0f;
    float lapCalories      = 0.0f;
    /// Basal (BMR) calories accumulated at MET 1.0 every active second, regardless of HR.
    /// Reported in the FIT file as session "metabolic_calories" / lap "resting_calories".
    float restingCaloriesTotal = 0.0f;
    float restingCaloriesLap   = 0.0f;
    std::time_t zoneTimeSec[5] = {0, 0, 0, 0, 0}; ///< cumulative time in zones 1..5
};

} // namespace Track

#endif // TRACK_HPP
