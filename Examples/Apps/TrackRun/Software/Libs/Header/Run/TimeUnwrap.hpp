/*******************************************************************************
 * @file   TimeUnwrap.hpp
 * @brief  Run: a millisecond clock that wraps modulo 2^32.
 *
 *         The watch's system millisecond clock counts its 1024 Hz tick in
 *         milliseconds, so it runs 0 to 4,194,303,999 and then starts again
 *         at 0 (about every 48.5 days of uptime). It does not reach 2^32.
 *         Every time difference in Track Run is taken modulo 2^32, which
 *         would read that wrap as a step of about 28 hours forward.
 *
 *         Fed each reading of such a clock, in order, this returns a clock
 *         that carries on modulo 2^32 instead. A clock that does run to 2^32
 *         (the simulator's) passes through unchanged. A reading from before
 *         a wrap that arrives after it is placed before it too.
 *
 *         Plain C++, no SDK headers, no heap.
 ******************************************************************************/

#ifndef RUN_TIME_UNWRAP_HPP
#define RUN_TIME_UNWRAP_HPP

#include <cstdint>

namespace Run {

class TimeUnwrap {
public:
    /** Where the watch's millisecond clock starts again. */
    static constexpr uint32_t kWatchWrapMs = 4194304000u;

    uint32_t operator()(uint32_t raw) noexcept
    {
        if (!mHave) {
            mHave = true;
            mLast = raw;
            return raw;
        }
        constexpr uint32_t kHalf = 0x80000000u;
        if (raw < mLast && mLast - raw > kHalf) {
            // The clock started again. Below kWatchWrapMs it is the watch's
            // clock; above it, one that wraps at 2^32 by itself.
            mStep = mLast >= kWatchWrapMs ? 0u : kWatchWrapMs;
            mAdd += mStep;
            mLast = raw;
        } else if (raw > mLast && raw - mLast > kHalf) {
            return raw + mAdd - mStep; // from before the latest wrap
        } else if (raw > mLast) {
            mLast = raw;
        }
        return raw + mAdd;
    }

private:
    bool     mHave = false;
    uint32_t mLast = 0;
    uint32_t mAdd  = 0; ///< Added to every reading, modulo 2^32
    uint32_t mStep = 0; ///< The latest wrap's share of mAdd
};

} // namespace Run

#endif // RUN_TIME_UNWRAP_HPP
