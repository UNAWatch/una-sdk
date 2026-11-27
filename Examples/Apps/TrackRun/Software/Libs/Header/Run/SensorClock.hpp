/*******************************************************************************
 * @file   SensorClock.hpp
 * @brief  Run: "now" on the GPS sensor clock, which the Oval code and every
 *         split time use.
 *
 *         GPS readings carry their own millisecond timestamps. The service's
 *         only clock is the system millisecond tick, and on the watch the
 *         sensor clock's epoch is not documented, so the offset between the
 *         two is learnt from the readings: offset = sensor time - system time
 *         at arrival. Each reading arrives some latency after its timestamp,
 *         so the reading with the smallest latency gives the largest offset,
 *         and that is the one kept. A smaller one (the clocks drifting apart)
 *         is followed only slowly, so jitter never moves "now" backwards.
 *         now() never decreases.
 *
 *         Before the first reading the offset is 0 (the system clock). The
 *         first reading sets it outright; onSample() returns that jump (the
 *         service times no run before the first reading, so it has nothing
 *         to move).
 *
 *         Plain C++, no SDK headers, no heap.
 ******************************************************************************/

#ifndef RUN_SENSOR_CLOCK_HPP
#define RUN_SENSOR_CLOCK_HPP

#include <cstdint>

namespace Run {

class SensorClock {
public:
    /** How fast a smaller offset is followed: ms per second of system time. */
    static constexpr uint32_t kSlewMsPerS = 1;

    void reset() noexcept { *this = SensorClock(); }

    /** A reading stamped sensorMs arrived at system time sysMs. Returns the
     *  jump in the offset when this is the first reading (0 otherwise). */
    int32_t onSample(uint32_t sensorMs, uint32_t sysMs) noexcept;

    /** Now on the sensor clock. Never decreases. */
    uint32_t now(uint32_t sysMs) noexcept;

    bool    synced() const noexcept { return mSynced; }
    int32_t offsetMs() const noexcept { return mOffset; }

private:
    bool     mSynced  = false;
    int32_t  mOffset  = 0;     ///< sensor - system
    uint32_t mSlewSys = 0;     ///< System time the slew was last applied
    bool     mHaveOut = false;
    uint32_t mLastOut = 0;
};

} // namespace Run

#endif // RUN_SENSOR_CLOCK_HPP
