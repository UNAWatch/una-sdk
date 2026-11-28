/*******************************************************************************
 * @file   LapTiming.hpp
 * @brief  Oval: the last few seconds of readings, and the times fitted from
 *         them.
 *
 *         A lap is first reported with the two-reading interpolated time, so
 *         the lap alert is not delayed. A few seconds later a straight line
 *         of position against time is fitted through the readings either
 *         side of the crossing and solved for the crossing time: white noise
 *         then averages out over several readings instead of two.
 *
 *         Each entry keeps the reading's local position (for counting mode,
 *         where the position along the gate direction is fitted) and its raw
 *         track total (for calibrated mode: the credited distance plus the
 *         unsmoothed position along the lane line, continuous through lap
 *         ends). The same fit answers when a given total was reached, for
 *         rep endings.
 *
 *         No heap, no exceptions, no SDK headers.
 ******************************************************************************/

#ifndef OVAL_LAP_TIMING_HPP
#define OVAL_LAP_TIMING_HPP

#include "Oval/LocalFrame.hpp"

#include <cstdint>

namespace Oval {

class LapTiming {
public:
    static constexpr uint8_t kSlots = 24;

    enum Flags : uint8_t {
        kPosition   = 1, ///< p is a good position in the local frame
        kCalibrated = 2, ///< total is a raw calibrated-mode total (fit it)
        kCounting   = 4, ///< total is the counting-mode shown total (interpolate it)
    };

    struct Entry {
        uint32_t tMs   = 0;
        Vec2     p;
        float    total = 0.0f;
        uint8_t  flags = 0;
    };

    void clear() noexcept { mCount = 0; mHead = 0; }
    void push(const Entry& e) noexcept;

    uint8_t      size() const noexcept { return mCount; }
    const Entry& at(uint8_t i) const noexcept; ///< 0 = oldest

    /** Counting mode: the time the position along dir from c crosses zero,
     *  fitted over the readings from beforeMs before to afterMs after tc
     *  (the two-reading time). False if the readings do not allow a fit. */
    bool fitGateCrossing(uint32_t tc, Vec2 c, Vec2 dir, uint32_t beforeMs, uint32_t afterMs,
                         uint32_t& outMs) const noexcept;

    /** Calibrated mode: the time the raw total reaches target, fitted over
     *  the calibrated readings from beforeMs before to afterMs after tc. */
    bool fitTotalCrossing(uint32_t tc, float target, uint32_t beforeMs, uint32_t afterMs,
                          uint32_t& outMs) const noexcept;

    /** When the total first reached target among the kept readings: fitted
     *  on calibrated readings, interpolated between two on counting ones.
     *  False if it has not been reached, or was before the oldest reading. */
    bool timeOfTotal(float target, uint32_t beforeMs, uint32_t afterMs, uint32_t& outMs) const noexcept;

private:
    /** Least squares line y = a + b t through the chosen entries (t in
     *  seconds from tc), solved for y = target. */
    bool fitLine(uint32_t tc, uint32_t beforeMs, uint32_t afterMs, uint8_t flag, Vec2 c, Vec2 dir,
                 float target, uint32_t& outMs) const noexcept;

    Entry   mE[kSlots];
    uint8_t mHead  = 0; ///< Next slot to write
    uint8_t mCount = 0;
};

} // namespace Oval

#endif // OVAL_LAP_TIMING_HPP
