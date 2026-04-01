/**
 ******************************************************************************
 * @file    SampleRateAdapter.hpp
 * @date    18-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Sample rate adapter
 *
 ******************************************************************************
 */

#ifndef __SENSOR_SAMPLE_RATE_ADAPTER_HPP
#define __SENSOR_SAMPLE_RATE_ADAPTER_HPP

#include <cstdint>

namespace Sensor
{
class SampleRateAdapter
{
public:
    /**
     * @brief Constructor
     * @param expectedPeriod Desired period between samples (in milliseconds)
     */
    explicit SampleRateAdapter(uint64_t expectedPeriod = 1000);

    /**
     * @brief Set the expected sampling period.
     * @param periodMs Period in milliseconds
     */
    void setExpectedPeriod(uint64_t periodMs);

    /**
     * @brief Reset the internal state (e.g., after reconnect)
     */
    void reset();

    /**
     * @brief Check whether the sample should be forwarded to listener
     * @return true if period since last accepted sample >= expected
     */
    bool shouldEmit();

    bool shouldEmit(uint64_t ts);

private:
    uint64_t mExpectedPeriod;       ///< Desired period in ms
    uint64_t mLastEmitedTimestamp;  ///< Timestamp of last emitted sample
    uint64_t mLastSampleTimestamp;  ///< Timestamp of last input sample
};

} // namespace Sensor

#endif // __SENSOR_SAMPLE_RATE_ADAPTER_HPP
