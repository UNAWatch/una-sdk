/**
 * @file Utils.hpp
 * @date 26-04-2026
 * @author Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief General-purpose utility functions shared across all SDK applications.
 *
 * Unit conversions, time decomposition, and generic array algorithms.
 */

#ifndef SDK_UTILS_UTILS_HPP
#define SDK_UTILS_UTILS_HPP

#include <cstdint>
#include <cstddef>
#include <ctime>
#include <cmath>

namespace SDK::Utils
{

// =============================================================================
// Frame-rate timing
// =============================================================================

/**
 * @brief Convert milliseconds to display ticks at the given frame rate.
 * @param ms   Duration in milliseconds.
 * @param fps  Frame rate in frames per second.
 */
inline constexpr uint32_t msToTicks(uint32_t ms, uint32_t fps)
{
    return ms * fps / 1000;
}

/**
 * @brief Convert seconds to display ticks at the given frame rate.
 * @param sec  Duration in seconds.
 * @param fps  Frame rate in frames per second.
 */
inline constexpr uint32_t secToTicks(uint32_t sec, uint32_t fps)
{
    return sec * fps;
}

// =============================================================================
// Unit conversions
// =============================================================================

/** @brief Convert kilometres to miles. */
inline constexpr float kmToMiles(float km) { return km * 0.621371f; }

/** @brief Convert miles to kilometres. */
inline constexpr float milesToKm(float mi) { return mi / 0.621371f; }

/** @brief Convert metres to feet. */
inline constexpr float metersToFeet(float m) { return m * 3.28084f; }

/** @brief Convert feet to metres. */
inline constexpr float feetToMeters(float ft) { return ft / 3.28084f; }

// =============================================================================
// Time decomposition
// =============================================================================

/** @brief Decomposed hours / minutes / seconds. */
struct HMS {
    uint16_t h = 0;  ///< Hours (not wrapped at 24)
    uint8_t  m = 0;  ///< Minutes (0-59)
    uint8_t  s = 0;  ///< Seconds (0-59)
};

/**
 * @brief Decompose a total-seconds value into hours, minutes and seconds.
 * @param sec  Total seconds (non-negative).
 * @return     HMS struct with h, m, s fields.
 */
inline constexpr HMS toHMS(std::time_t sec)
{
    HMS result{};
    result.h = static_cast<uint16_t>(sec / 3600);
    sec %= 3600;
    result.m = static_cast<uint8_t>(sec / 60);
    result.s = static_cast<uint8_t>(sec % 60);
    return result;
}

// =============================================================================
// Array algorithms
// =============================================================================

/**
 * @brief Return the value in @p arr closest to @p target.
 *
 * @param arr    Pointer to array of values.
 * @param len    Number of elements.
 * @param target Target value to snap.
 * @return       Nearest element value.
 */
template<typename T>
inline T nearestValueIn(const T* arr, size_t len, float target)
{
    T nearest = arr[0];
    float minDiff = std::abs(static_cast<float>(arr[0]) - target);

    for (size_t i = 1; i < len; ++i) {
        float diff = std::abs(static_cast<float>(arr[i]) - target);
        if (diff < minDiff) {
            minDiff = diff;
            nearest = arr[i];
        }
    }
    return nearest;
}

/** @brief Array overload: size deduced automatically. */
template<typename T, size_t N>
inline T nearestValueIn(const T (&arr)[N], float target)
{
    return nearestValueIn(arr, N, target);
}

/**
 * @brief Return the index of the value in @p arr closest to @p target.
 *
 * @param arr    Pointer to array of values.
 * @param len    Number of elements.
 * @param target Target value to snap.
 * @return       Index of the nearest element.
 */
template<typename T>
inline size_t nearestIndexIn(const T* arr, size_t len, float target)
{
    size_t nearestIdx = 0;
    float minDiff = std::abs(static_cast<float>(arr[0]) - target);

    for (size_t i = 1; i < len; ++i) {
        float diff = std::abs(static_cast<float>(arr[i]) - target);
        if (diff < minDiff) {
            minDiff = diff;
            nearestIdx = i;
        }
    }
    return nearestIdx;
}

/** @brief Array overload: size deduced automatically. */
template<typename T, size_t N>
inline size_t nearestIndexIn(const T (&arr)[N], float target)
{
    return nearestIndexIn(arr, N, target);
}

} // namespace SDK::Utils

#endif // SDK_UTILS_UTILS_HPP
