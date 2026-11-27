/*******************************************************************************
 * @file   TickLog.hpp
 * @brief  Run: what each second of the run looked like, kept long enough for
 *         a split that settles minutes after it ended.
 *
 *         One entry per active 1 Hz tick: its time on the sensor clock, heart
 *         rate, live speed, the altitude written to the record and the run's
 *         effective ascent and descent so far (counted only off the track).
 *         A split's stats are taken over the ticks inside it, so they follow
 *         the split when its ends move. Paused ticks are not logged.
 *
 *         Ascent and descent are kept in decimetres modulo 65536: a split's
 *         figure is right up to 6.5 km of climbing.
 *
 *         An entry is packed into 14 bytes (not 16) with nothing lost:
 *         each keeps the low 24 bits of its time, and the full time is the
 *         newest tick's less the difference. That is exact while the ticks
 *         kept span less than 2^24 ms (4.6 hours, pauses included).
 *         A split's stats reach back over the split itself and the settle
 *         window: about 610 s for a road kilometre at 5:00/km, and up to
 *         840 s in the host sweep (slow road kilometres). 900 ticks are
 *         kept: that longest reach plus a minute. A longer split (a 5000 m
 *         lap, auto-lap Off) is whole all the same: from the start of the
 *         oldest split still held (anchor()), ticks that leave the log are
 *         kept as sums, maxima and the ascent and descent before it, and
 *         count while the oldest tick kept is not after the split's end.
 *
 *         Plain C++, no SDK headers, no heap.
 ******************************************************************************/

#ifndef RUN_TICK_LOG_HPP
#define RUN_TICK_LOG_HPP

#include <cstdint>

namespace Run {

class TickLog {
public:
    static constexpr uint16_t kSize = 900; ///< 15 minutes of ticks

    struct Stats {
        uint32_t ticks    = 0;    ///< Ticks inside the split
        float    hrAvg    = 0.0f; ///< Over ticks with a heart rate; 0 if none
        float    hrMax    = 0.0f;
        float    speedMax = 0.0f; ///< m/s
        float    ascentM  = 0.0f;
        float    descentM = 0.0f;
        bool     partial  = false; ///< The split began before the oldest tick kept
    };

    void reset() noexcept
    {
        mHead = 0; mCount = 0; mNewestT = 0; mLeftT = 0; mLeftAny = false; mLeftAscDm = 0; mLeftDescDm = 0;
        mCarry = Carry();
    }

    /** The oldest split still held starts at fromMs (the run's start, then
     *  the end of each split handed out): the ticks from there that leave
     *  the log still count in its stats. */
    void anchor(uint32_t fromMs) noexcept;

    /** An active tick. hrBpm 0 means no heart rate. */
    void add(uint32_t tMs, float hrBpm, float speedMps, float altM, float ascentM, float descentM) noexcept;

    /** Stats over the ticks in [startMs, endMs). */
    Stats stats(uint32_t startMs, uint32_t endMs) const noexcept;

    /** The ascent and descent gained since tMs (from the last tick at or
     *  before it, else the oldest tick kept, to the newest), and the altitude
     *  at that tick. False if the log is empty. */
    bool since(uint32_t tMs, float& ascentM, float& descentM, float& altM) const noexcept;

    /** From tMs on, ascent and descent stay what they were at tMs, and the
     *  altitude is altM: a track joined some seconds back holds its
     *  elevation back to where the runner joined it. */
    void flattenFrom(uint32_t tMs, float altM) noexcept;


    uint16_t count() const noexcept { return mCount; }

private:
    struct Entry {
        uint16_t tLo;      ///< Time, bits 0 to 15
        uint8_t  tHi;      ///< Time, bits 16 to 23 (the rest from the newest tick)
        uint8_t  hr;       ///< bpm, 0 none
        uint16_t speedCms; ///< cm/s
        uint16_t ascDm;    ///< Effective ascent so far, dm, modulo 65536
        uint16_t descDm;
        uint16_t alt[2];   ///< The altitude's float, bit for bit
    };
    static_assert(sizeof(Entry) == 14, "a tick is 14 bytes");

    /** Ticks from fromMs that have left the log, and the ascent and descent
     *  before fromMs. */
    struct Carry {
        uint32_t fromMs   = 0;
        uint32_t hrSum    = 0;
        uint32_t hrN      = 0;
        uint32_t ticks    = 0;
        uint16_t speedCms = 0; ///< The highest
        uint16_t ascDm    = 0; ///< Before fromMs (if base)
        uint16_t descDm   = 0;
        uint8_t  hrMax    = 0;
        bool     lost     = false; ///< Ticks from fromMs had left before it was set
        bool     set      = false;
        bool     base     = false;
    };

    const Entry& back(uint16_t b) const noexcept { return mE[(mHead + kSize - 1u - b) % kSize]; }
    Entry&       back(uint16_t b) noexcept { return mE[(mHead + kSize - 1u - b) % kSize]; }
    /** The entry's full time: the newest tick's, less the 24-bit difference. */
    uint32_t timeOf(const Entry& e) const noexcept;
    static float altOf(const Entry& e) noexcept;
    static void  setAlt(Entry& e, float altM) noexcept;

    Entry    mE[kSize] = {};
    uint32_t mNewestT = 0; ///< The newest tick's full time
    uint16_t mHead  = 0;
    uint16_t mCount = 0;
    uint32_t mLeftT   = 0;     ///< The newest tick that has left the log
    bool     mLeftAny = false;
    uint16_t mLeftAscDm  = 0;  ///< Its ascent and descent so far
    uint16_t mLeftDescDm = 0;
    Carry    mCarry;
};

} // namespace Run

#endif // RUN_TICK_LOG_HPP
