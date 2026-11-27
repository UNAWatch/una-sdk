/*******************************************************************************
 * @file   PaceWindow.hpp
 * @brief  Run: live speed from the run's total distance over a rolling window,
 *         never the GPS speed field, so pace agrees with the distance
 *         counted.
 *
 *         One entry per GPS reading: its sensor time and the run's total
 *         distance after it. Speed is the distance gained over the last
 *         windowMs (about 12 s). The same entries give timeAt(): when the
 *         total reached a figure, interpolated between readings, which is how
 *         a rep or distance split that ends on the road is timed.
 *
 *         The total can jump up when a correction lands (a known track
 *         joined back to where the runner came on, the way off a track
 *         credited).
 *         rebase() shifts the older entries up by the jump, so the window
 *         sees only running, and pace never spikes.
 *
 *         Plain C++, no SDK headers, no heap.
 ******************************************************************************/

#ifndef RUN_PACE_WINDOW_HPP
#define RUN_PACE_WINDOW_HPP

#include <cstdint>

namespace Run {

class PaceWindow {
public:
    static constexpr uint8_t  kSize            = 40;    ///< Readings kept (about 40 s)
    static constexpr uint32_t kWindowMs        = 12000; ///< The live speed window
    static constexpr uint32_t kMinSpanMs       = 4000;  ///< Less than this: no speed yet
    static constexpr float    kMaxRunMps       = 12.0f; ///< Faster than any runner

    void reset() noexcept { mCount = 0; mHead = 0; }

    /** A reading: sensor time and the run's total after it. Readings must
     *  come in time order; an older or equal time is ignored. */
    void add(uint32_t tMs, float totalM) noexcept;

    /** Moves every entry up by jumpM (a correction, not running). */
    void rebase(float jumpM) noexcept;


    /** Speed over the last kWindowMs, m/s. 0 with too little data. */
    float speedMps() const noexcept;

    /** The speed before the newest reading (for sizing a jump). */
    float speedBeforeNewestMps() const noexcept;

    /** When the total reached totalM, interpolated. False if it is outside
     *  the readings kept. */
    bool timeAt(float totalM, uint32_t& tMs) const noexcept;

    /** The total at tMs, interpolated; the newest total if tMs is later than
     *  the newest reading. False if tMs is older than the readings kept. */
    bool totalAt(uint32_t tMs, float& totalM) const noexcept;

    uint8_t  count() const noexcept { return mCount; }
    bool     newest(uint32_t& tMs, float& totalM) const noexcept;

private:
    struct Entry {
        uint32_t tMs;
        float    totalM;
    };
    const Entry& at(uint8_t back) const noexcept; ///< 0 = newest
    float speedOver(uint8_t newestBack) const noexcept;

    Entry   mE[kSize] = {};
    uint8_t mHead  = 0; ///< Next slot to write
    uint8_t mCount = 0;
};

/** How much of a rise in the total over dtMs is a correction rather than
 *  running. What the recent speed explains (25% and 2 m of slack; up to
 *  kMaxRunMps when there is no speed yet) is running; the rest is a jump.
 *  Unless forced (a correction event came with this reading), a rise that a
 *  runner could make (kMaxRunMps x dt) is all running. Never below 0. */
float correctionJumpM(float riseM, uint32_t dtMs, float recentMps, bool forced) noexcept;

} // namespace Run

#endif // RUN_PACE_WINDOW_HPP
