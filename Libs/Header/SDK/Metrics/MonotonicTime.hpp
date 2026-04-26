/**
 * @file MonotonicTime.hpp
 * @date 07-02-2025
 * @author Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief Monotonic time tracker independent of RTC and timezone changes
 */

#ifndef SDK_METRICS_MONOTONIC_TIME_HPP
#define SDK_METRICS_MONOTONIC_TIME_HPP

#include <cstdint>
#include <ctime>

namespace SDK::Metric {

/**
 * @brief Monotonic time tracker independent of RTC and timezone changes
 *
 * Provides time values based on initial timestamp + elapsed monotonic ticks,
 * guaranteeing monotonically increasing results unaffected by RTC adjustments
 * or timezone changes. Use for activity duration and metric timestamps.
 *
 * The getExpected*() methods return monotonic values derived from the tick counter.
 * getLocalTime() returns the current system local time and is NOT monotonic.
 *
 * @tparam TClock Clock source type. Must provide: @code uint32_t getTimeMs() @endcode
 *
 * @note Must call init() before using any other method.
 */
template<typename TClock>
class MonotonicTime {
public:
    /**
     * @brief Construct with a clock source.
     *
     * @param clock Reference to a clock source that provides getTimeMs().
     *              The reference must remain valid for the lifetime of this object.
     */
    explicit MonotonicTime(TClock& clock);

    ~MonotonicTime() = default;

    /**
     * @brief Initialize starting timestamps for monotonic tracking.
     *
     * Captures current UTC time, local timezone offset, and the current
     * monotonic tick count. Must be called before any other method.
     */
    void init();

    /**
     * @brief Get expected UTC time based on monotonic counter.
     *
     * Calculates UTC using the initial timestamp and elapsed ticks.
     * Result is monotonically increasing and unaffected by RTC changes.
     *
     * @return Expected UTC time (seconds since epoch), or 0 if not initialized
     */
    std::time_t getExpectedUTC();

    /**
     * @brief Get expected local time for the current monotonic time.
     *
     * Uses the timezone offset captured at init(), so the result is
     * unaffected by subsequent timezone changes.
     *
     * @return Expected local time as tm structure
     */
    std::tm getExpectedLocalTime();

    /**
     * @brief Get expected local time for a given UTC value.
     *
     * Converts the given UTC to local time using the timezone offset
     * captured at init(). Unaffected by timezone changes after init().
     *
     * @param utc UTC time to convert
     * @return Corresponding local time as tm structure
     */
    std::tm getExpectedLocalTime(std::time_t utc);

    /**
     * @brief Get current system local time for a given UTC value.
     *
     * Converts UTC using the current system timezone - reflects real-time
     * timezone changes and is therefore NOT monotonic.
     *
     * @param utc UTC time to convert
     * @return Corresponding system local time as tm structure
     */
    std::tm getLocalTime(std::time_t utc);

    /**
     * @brief Get elapsed seconds since init() was called.
     *
     * @return Elapsed time in seconds, or 0 if not initialized
     */
    uint64_t getElapsedSeconds();

    /**
     * @brief Get the UTC timestamp captured during init().
     *
     * @return UTC time at init (seconds since epoch)
     */
    std::time_t getStartUTC() const;

private:
    TClock&     mClock;               /* Clock source providing getTimeMs() */
    std::time_t mStartUTC;            /* UTC time captured at init() */
    std::time_t mStartLocalTimeOffset;/* Local timezone offset captured at init() */
    uint32_t    mStartMono;           /* Monotonic tick count captured at init() (ms) */
    bool        mIsInitialized;       /* True after init() has been called */
};

// Template implementation

template<typename TClock>
MonotonicTime<TClock>::MonotonicTime(TClock& clock)
    : mClock(clock)
    , mStartUTC(0)
    , mStartLocalTimeOffset(0)
    , mStartMono(0)
    , mIsInitialized(false)
{
}

template<typename TClock>
void MonotonicTime<TClock>::init()
{
    mStartUTC = std::time(nullptr);

    std::tm gmt {};
    std::tm loc {};

#if defined(_WIN32) || defined(_WIN64)
    gmtime_s(&gmt, &mStartUTC);
    localtime_s(&loc, &mStartUTC);
#else
    gmtime_r(&mStartUTC, &gmt);
    localtime_r(&mStartUTC, &loc);
#endif

    std::time_t gmtTime = std::mktime(&gmt);
    std::time_t locTime = std::mktime(&loc);
    mStartLocalTimeOffset = static_cast<std::time_t>(std::difftime(locTime, gmtTime));

    mStartMono     = mClock.getTimeMs();
    mIsInitialized = true;
}

template<typename TClock>
std::time_t MonotonicTime<TClock>::getExpectedUTC()
{
    if (!mIsInitialized) {
        return 0;
    }
    uint32_t elapsedMs = mClock.getTimeMs() - mStartMono;
    return mStartUTC + (elapsedMs / 1000);
}

template<typename TClock>
std::tm MonotonicTime<TClock>::getExpectedLocalTime()
{
    return getExpectedLocalTime(getExpectedUTC());
}

template<typename TClock>
std::tm MonotonicTime<TClock>::getExpectedLocalTime(std::time_t utc)
{
    std::tm tmResult {};
    std::time_t localTime = utc + mStartLocalTimeOffset;

#if defined(_WIN32) || defined(_WIN64)
    gmtime_s(&tmResult, &localTime);
#else
    gmtime_r(&localTime, &tmResult);
#endif

    return tmResult;
}

template<typename TClock>
std::tm MonotonicTime<TClock>::getLocalTime(std::time_t utc)
{
    std::tm tmResult {};

#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tmResult, &utc);
#else
    localtime_r(&utc, &tmResult);
#endif

    return tmResult;
}

template<typename TClock>
uint64_t MonotonicTime<TClock>::getElapsedSeconds()
{
    if (!mIsInitialized) {
        return 0;
    }
    uint32_t elapsedMs = mClock.getTimeMs() - mStartMono;
    return elapsedMs / 1000;
}

template<typename TClock>
std::time_t MonotonicTime<TClock>::getStartUTC() const
{
    return mStartUTC;
}

}  // namespace SDK::Metric

#endif // SDK_METRICS_MONOTONIC_TIME_HPP
