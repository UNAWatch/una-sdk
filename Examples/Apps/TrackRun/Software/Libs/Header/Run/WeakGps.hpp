/*******************************************************************************
 * @file   WeakGps.hpp
 * @brief  Run: the weak GPS flag for the run screen (its weak
 *         GPS icon). Weak once there has been no good reading (a fix with HDOP
 *         at most maxHdop) for more than kWeakAfterMs. Through such a spell
 *         the track total can hold still (docs/OVAL.md); this
 *         flag only says so, it changes no distance.
 *
 *         Plain C++, no SDK headers, no heap.
 ******************************************************************************/

#ifndef RUN_WEAK_GPS_HPP
#define RUN_WEAK_GPS_HPP

#include <cstdint>

namespace Run {

class WeakGps {
public:
    static constexpr uint32_t kWeakAfterMs = 10000;

    void reset() noexcept { *this = WeakGps(); }

    /** A reading. hdop < 0 means unknown, which counts as good. */
    void add(uint32_t tMs, bool fix, float hdop, float maxHdop) noexcept;

    /** Weak at nowMs (sensor clock): no good reading for over kWeakAfterMs.
     *  False until the first reading after reset(). */
    bool weak(uint32_t nowMs) const noexcept;

private:
    bool     mHaveAny  = false;
    bool     mHaveGood = false;
    uint32_t mFirstT   = 0;
    uint32_t mGoodT    = 0;
};

} // namespace Run

#endif // RUN_WEAK_GPS_HPP
