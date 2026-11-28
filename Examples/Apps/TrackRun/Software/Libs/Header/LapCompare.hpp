/**
 ******************************************************************************
 * @file    LapCompare.hpp
 * @brief   A lap against the lap before it, by pace, for the lap pop-up's
 *          delta chip and the summary's lap pace chart. Plain C++, no SDK.
 *
 * Laps can differ in length (an auto-lap, a press of Lap, the last part lap),
 * so a lap is compared by pace: the lap before's pace over this lap's
 * distance, against this lap's time. The difference is in whole seconds, as
 * lap times are: under half a second either way is even.
 *
 * The same file is in Run (Running), RunLVGL and Track Run, kept identical.
 ******************************************************************************
 */

#ifndef LAP_COMPARE_HPP
#define LAP_COMPARE_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace LapCompare {

enum class Verdict : uint8_t { None, Faster, Slower, Even };

struct Delta {
    Verdict verdict = Verdict::None;
    int32_t s       = 0;   ///< This lap less the lap before (scaled), s; negative is faster
};

/** Laps shorter than this are left off the pace chart: a few metres give
 *  a pace that means nothing. */
constexpr float kMinChartM = 100.0f;

/** A wider gap than this (s) is no comparison; it also keeps the rounded
 *  gap inside int32_t. */
constexpr double kMaxDiffS = 86400.0;

/** timeS over m against prevTimeS over prevM, by pace. None if either lap
 *  has no time or no distance, or if the two differ by a day or more (a lap
 *  of a few millimetres before a long one), which says nothing. */
inline Delta compare(uint32_t timeS, float m, uint32_t prevTimeS, float prevM)
{
    Delta d;
    if (timeS == 0 || prevTimeS == 0 || !(m > 0.0f) || !(prevM > 0.0f)) {
        return d;
    }
    const double expected = static_cast<double>(prevTimeS) * static_cast<double>(m) / static_cast<double>(prevM);
    const double diff     = static_cast<double>(timeS) - expected;
    if (!(diff > -kMaxDiffS && diff < kMaxDiffS)) {
        return d;
    }
    d.s       = static_cast<int32_t>(diff < 0.0 ? diff - 0.5 : diff + 0.5);
    d.verdict = d.s < 0 ? Verdict::Faster : d.s > 0 ? Verdict::Slower : Verdict::Even;
    return d;
}

/** The chip's text: "3 s faster", "1:02 slower", "Even";
 *  empty with nothing to compare. */
inline void line(char* buf, size_t n, const Delta& d)
{
    if (n == 0) {
        return;
    }
    const uint32_t a = static_cast<uint32_t>(d.s < 0 ? -static_cast<int64_t>(d.s) : d.s);
    const char*    w = d.verdict == Verdict::Faster ? "faster" : "slower";
    switch (d.verdict) {
    case Verdict::Faster:
    case Verdict::Slower:
        if (a >= 60u) {
            std::snprintf(buf, n, "%u:%02u %s", static_cast<unsigned>(a / 60u), static_cast<unsigned>(a % 60u), w);
        } else {
            std::snprintf(buf, n, "%u s %s", static_cast<unsigned>(a), w);
        }
        break;
    case Verdict::Even: std::snprintf(buf, n, "Even"); break;
    default:            buf[0] = '\0'; break;
    }
}

/** The laps as they end: each against the one before it. The same lap may
 *  come again with later figures (taken at the press, then as the service
 *  ends it): it is still compared with the lap before. A lap whose number
 *  does not follow on from the last one seen (a lap gone by unseen, a GUI
 *  that started mid-run) has nothing to compare with. */
class Tracker {
public:
    void reset() { mHaveCur = false; mHavePrev = false; mLast = Delta(); }

    /** Lap number num (0-based, as Track::Data::lapNum) ended after timeS
     *  over m. Returns it against the lap before; last() keeps it. */
    Delta add(uint32_t num, uint32_t timeS, float m)
    {
        if (!(mHaveCur && num == mCur.num)) {
            mHavePrev = mHaveCur && num == mCur.num + 1u;
            mPrev     = mCur;
        }
        mCur     = Lap{num, timeS, m};
        mHaveCur = true;
        mLast    = mHavePrev ? compare(timeS, m, mPrev.timeS, mPrev.m) : Delta();
        return mLast;
    }

    const Delta& last() const { return mLast; }

private:
    struct Lap {
        uint32_t num   = 0;
        uint32_t timeS = 0;
        float    m     = 0.0f;
    };
    Lap   mCur;
    Lap   mPrev;
    bool  mHaveCur  = false;
    bool  mHavePrev = false;
    Delta mLast;
};

/** The pace chart's fastest and slowest laps (indices into the list), of
 *  those kMinChartM or longer with a time; -1 if there are fewer than two. */
struct Extremes {
    int fastest = -1;
    int slowest = -1;
};

/** paceAt(i) is lap i's pace in s/m, distanceAt(i) its distance; count laps. */
template <typename Pace, typename Dist>
Extremes extremes(size_t count, Pace paceAt, Dist distanceAt)
{
    Extremes e;
    int charted = 0;
    for (size_t i = 0; i < count; ++i) {
        const float p = paceAt(i);
        if (!(distanceAt(i) >= kMinChartM) || !(p > 0.0f)) {
            continue;
        }
        ++charted;
        if (e.fastest < 0 || p < paceAt(static_cast<size_t>(e.fastest))) {
            e.fastest = static_cast<int>(i);
        }
        if (e.slowest < 0 || p > paceAt(static_cast<size_t>(e.slowest))) {
            e.slowest = static_cast<int>(i);
        }
    }
    if (charted < 2) {
        e = Extremes();
    }
    return e;
}

} // namespace LapCompare

#endif // LAP_COMPARE_HPP
