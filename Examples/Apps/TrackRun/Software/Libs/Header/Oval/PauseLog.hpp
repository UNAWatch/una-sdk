/*******************************************************************************
 * @file   PauseLog.hpp
 * @brief  Oval: the run's recent pauses, for questions about any span of
 *         time: how much of it was paused, and the active time at a moment
 *         in the past.
 *
 *         The last kKept completed pauses are kept, with the one in force.
 *         Older pauses count in the totals only, so a span that reaches back
 *         past them is taken as unpaused there. A reading interval or a lap's
 *         refinement window is a few seconds, well inside what is kept.
 *
 *         Times are on the sample clock, wrap-safe (msDiff). Plain C++, no
 *         heap.
 ******************************************************************************/

#ifndef OVAL_PAUSE_LOG_HPP
#define OVAL_PAUSE_LOG_HPP

#include "Oval/Sample.hpp"

#include <cstdint>

namespace Oval {

class PauseLog {
public:
    static constexpr uint8_t kKept = 8;

    void reset() noexcept { *this = PauseLog(); }

    void pause(uint32_t tMs) noexcept
    {
        if (!mPaused) {
            mPaused = true;
            mStartT = tMs;
        }
    }

    void resume(uint32_t tMs) noexcept
    {
        if (!mPaused) {
            return;
        }
        mPaused = false;
        const int32_t len = msDiff(tMs, mStartT);
        if (len <= 0) {
            return;
        }
        mTotalMs += static_cast<uint64_t>(len);
        mKeep[mHead] = {mStartT, tMs};
        mHead        = static_cast<uint8_t>((mHead + 1u) % kKept);
        mCount       = static_cast<uint8_t>(mCount < kKept ? mCount + 1u : kKept);
    }

    bool isPaused() const noexcept { return mPaused; }

    /** Paused time within [fromMs, toMs): the pauses kept, and the one in
     *  force (open, to toMs). */
    uint32_t pausedIn(uint32_t fromMs, uint32_t toMs) const noexcept
    {
        if (msDiff(toMs, fromMs) <= 0) {
            return 0;
        }
        int64_t sum = 0;
        for (uint8_t i = 0; i < mCount; ++i) {
            sum += overlap(mKeep[i].startT, mKeep[i].endT, fromMs, toMs);
        }
        if (mPaused) {
            sum += overlap(mStartT, toMs, fromMs, toMs);
        }
        return static_cast<uint32_t>(sum);
    }

    /** Paused before tMs, of the run from startMs: every completed pause,
     *  less the parts of the kept ones at or after tMs, and the part of the
     *  one in force before tMs. */
    uint32_t pausedBefore(uint32_t startMs, uint32_t tMs) const noexcept
    {
        int64_t sum = static_cast<int64_t>(mTotalMs);
        for (uint8_t i = 0; i < mCount; ++i) {
            const int32_t after = msDiff(mKeep[i].endT, later(mKeep[i].startT, tMs));
            sum -= after > 0 ? after : 0;
        }
        if (mPaused) {
            const int32_t in = msDiff(tMs, later(mStartT, startMs));
            sum += in > 0 ? in : 0;
        }
        return sum > 0 ? static_cast<uint32_t>(sum) : 0u;
    }

    /** Within a pause at tMs (kept, or the one in force). */
    bool pausedAt(uint32_t tMs) const noexcept
    {
        if (mPaused && msDiff(tMs, mStartT) >= 0) {
            return true;
        }
        for (uint8_t i = 0; i < mCount; ++i) {
            if (msDiff(tMs, mKeep[i].startT) >= 0 && msDiff(mKeep[i].endT, tMs) > 0) {
                return true;
            }
        }
        return false;
    }

    /** The active share (0 to 1) of [fromMs, toMs): 1 for no span. */
    float activeShare(uint32_t fromMs, uint32_t toMs) const noexcept
    {
        const int32_t span = msDiff(toMs, fromMs);
        if (span <= 0) {
            return 1.0f;
        }
        const uint32_t p = pausedIn(fromMs, toMs);
        return p >= static_cast<uint32_t>(span) ? 0.0f
                                                : static_cast<float>(static_cast<uint32_t>(span) - p) / static_cast<float>(span);
    }

private:
    struct Span {
        uint32_t startT = 0;
        uint32_t endT   = 0;
    };

    static uint32_t later(uint32_t a, uint32_t b) noexcept { return msDiff(a, b) >= 0 ? a : b; }

    /** The length of [s, e) within [from, to). */
    static int32_t overlap(uint32_t s, uint32_t e, uint32_t from, uint32_t to) noexcept
    {
        const uint32_t a = later(s, from);
        const uint32_t b = msDiff(e, to) <= 0 ? e : to;
        const int32_t  d = msDiff(b, a);
        return d > 0 ? d : 0;
    }

    Span     mKeep[kKept] = {};
    uint8_t  mHead    = 0;
    uint8_t  mCount   = 0;
    bool     mPaused  = false;
    uint32_t mStartT  = 0;
    uint64_t mTotalMs = 0;
};

} // namespace Oval

#endif // OVAL_PAUSE_LOG_HPP
