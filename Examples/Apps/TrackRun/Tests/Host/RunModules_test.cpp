// The service-side helpers with no SDK (Software/Libs/*/Run): the
// sensor clock, the pace window, the weak GPS flag, the tick log, the split
// buffer and the GPS trace lines.

#include "PortableRandom.hpp"
#include "TraceCsv.hpp"

#include "Run/PaceWindow.hpp"
#include "Run/SensorClock.hpp"
#include "Run/TimeUnwrap.hpp"
#include "Run/TickLog.hpp"
#include "Run/TraceFormat.hpp"
#include "Run/WeakGps.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstring>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// SensorClock
// ---------------------------------------------------------------------------

TEST(SensorClock, NeverGoesBackAndStaysWithinTheLatency)
{
    // The sensor clock runs 123456 ms ahead of the system clock. Readings
    // arrive 20 to 300 ms after their timestamp; now() is asked every 100 ms.
    std::mt19937 rng(PortableRandom::seeded(5));
    PortableRandom::UniformInt lat(20, 300);
    Run::SensorClock c;
    const uint32_t ahead = 123456u;
    uint32_t last = 0;
    bool     have = false;
    int32_t  worstLag = 0, worstLead = 0;
    for (uint32_t sys = 1000u; sys < 1000u + 3600u * 1000u; sys += 100u) {
        if (sys % 1000u == 0u) {
            const uint32_t stamp = sys + ahead - static_cast<uint32_t>(lat(rng));
            const int32_t  jump  = c.onSample(stamp, sys);
            if (sys > 1000u) {
                EXPECT_EQ(jump, 0);
            }
        }
        const uint32_t n = c.now(sys);
        if (have) {
            ASSERT_GE(static_cast<int32_t>(n - last), 0) << "now() went back at " << sys;
        }
        have = true;
        last = n;
        if (c.synced()) {
            const int32_t err = static_cast<int32_t>(n - (sys + ahead));
            worstLag  = std::min(worstLag, err);
            worstLead = std::max(worstLead, err);
        }
    }
    // Never ahead of the truth, and never further behind than the latency.
    EXPECT_LE(worstLead, 0);
    EXPECT_GE(worstLag, -300);
}

TEST(TimeUnwrap, TheWatchClocksWrapCarriesOnModulo2To32)
{
    Run::TimeUnwrap u;
    const uint32_t w = Run::TimeUnwrap::kWatchWrapMs;
    EXPECT_EQ(u(w - 2000u), w - 2000u);
    EXPECT_EQ(u(w - 1u), w - 1u);
    EXPECT_EQ(u(0u), w) << "the step after the wrap is 1 ms, not 28 hours";
    EXPECT_EQ(u(1500u), w + 1500u);
    EXPECT_EQ(static_cast<int32_t>(u(1500u) - u(w - 1000u)), 2500) << "a straggler from before the wrap";
    // The next wrap, 48.5 days on, carries on past 2^32.
    for (uint64_t t = 1000000000u; t < w; t += 1000000000u) { // 64-bit: 5e9 would wrap
        EXPECT_EQ(u(static_cast<uint32_t>(t)), w + static_cast<uint32_t>(t));
    }
    EXPECT_EQ(u(w - 1u), w + w - 1u);
    EXPECT_EQ(u(5u), w + w + 5u);
}

TEST(TimeUnwrap, AClockThatWrapsAt2To32PassesThrough)
{
    Run::TimeUnwrap u;
    EXPECT_EQ(u(0xFFFFFF00u), 0xFFFFFF00u);
    EXPECT_EQ(u(0x10u), 0x10u);
    EXPECT_EQ(u(0x20u), 0x20u);
    EXPECT_EQ(u(0x18u), 0x18u) << "slightly out of order: unchanged";
}

TEST(SensorClock, FirstReadingSetsTheOffsetAndReportsTheJump)
{
    Run::SensorClock c;
    EXPECT_EQ(c.now(5000u), 5000u); // the system clock until the first reading
    const int32_t jump = c.onSample(90000u, 5100u);
    EXPECT_EQ(jump, 90000 - 5100);
    EXPECT_EQ(c.now(5100u), 90000u);
}

TEST(SensorClock, FollowsALargerLatencySlowlyAndASmallerOneAtOnce)
{
    Run::SensorClock c;
    c.onSample(10000u, 0u);           // offset 10000
    // The clocks drift: every reading now arrives with 500 ms more latency.
    uint32_t prev = c.now(0u);
    for (uint32_t s = 1u; s <= 100u; ++s) {
        c.onSample(10000u + s * 1000u - 500u, s * 1000u);
        const uint32_t n = c.now(s * 1000u);
        EXPECT_GE(static_cast<int32_t>(n - prev), 0);
        prev = n;
    }
    // 1 ms a second: 100 ms followed after 100 s, not 500.
    EXPECT_EQ(c.offsetMs(), 10000 - 100);
    // A reading with less latency again: taken at once.
    c.onSample(10000u + 101u * 1000u, 101u * 1000u);
    EXPECT_EQ(c.offsetMs(), 10000);
}

// ---------------------------------------------------------------------------
// PaceWindow
// ---------------------------------------------------------------------------

TEST(PaceWindow, SpeedOverTheWindowAndTimeAt)
{
    Run::PaceWindow p;
    EXPECT_EQ(p.speedMps(), 0.0f);
    for (uint32_t i = 0; i <= 30; ++i) {
        p.add(1000u * i, 4.0f * static_cast<float>(i));
    }
    EXPECT_NEAR(p.speedMps(), 4.0f, 1e-4f);
    uint32_t t = 0;
    ASSERT_TRUE(p.timeAt(110.0f, t));
    EXPECT_EQ(t, 27500u);
    float tot = 0.0f;
    ASSERT_TRUE(p.totalAt(27500u, tot));
    EXPECT_NEAR(tot, 110.0f, 1e-3f);
    EXPECT_FALSE(p.timeAt(500.0f, t)) << "not reached yet";
}

TEST(PaceWindow, ARebasedJumpDoesNotSpikeTheSpeed)
{
    Run::PaceWindow p;
    float total = 0.0f;
    for (uint32_t i = 0; i <= 20; ++i) {
        p.add(1000u * i, total);
        total += 4.0f;
    }
    // A correction of 150 m lands with one second of running.
    const float before = p.speedMps();
    const float rise   = 4.0f + 150.0f;
    const float jump   = Run::correctionJumpM(rise, 1000u, before, true);
    EXPECT_NEAR(jump, 150.0f - 1.0f - 2.0f, 1e-3f); // 25% and 2 m of slack stay as running
    total += 150.0f;
    p.add(21000u, total);
    p.rebase(jump);
    EXPECT_LT(p.speedMps(), 4.0f * 1.3f);
}

TEST(PaceWindow, RunningIsNotAJumpUnlessForced)
{
    EXPECT_EQ(Run::correctionJumpM(6.0f, 1000u, 4.0f, false), 0.0f);
    EXPECT_EQ(Run::correctionJumpM(11.9f, 1000u, 4.0f, false), 0.0f);
    EXPECT_GT(Run::correctionJumpM(40.0f, 1000u, 4.0f, false), 30.0f);
    EXPECT_EQ(Run::correctionJumpM(-3.0f, 1000u, 4.0f, true), 0.0f);
}

// ---------------------------------------------------------------------------
// WeakGps
// ---------------------------------------------------------------------------

TEST(WeakGps, WeakAfterTenSecondsWithoutAGoodReading)
{
    Run::WeakGps w;
    EXPECT_FALSE(w.weak(0u));
    w.add(1000u, true, 1.0f, 4.0f);
    for (uint32_t t = 2000u; t <= 11000u; t += 1000u) {
        w.add(t, true, 5.0f, 4.0f); // HDOP over the limit
        EXPECT_FALSE(w.weak(t)) << t;
    }
    w.add(12000u, true, 5.0f, 4.0f);
    EXPECT_TRUE(w.weak(12000u));
    w.add(13000u, true, -1.0f, 4.0f); // no HDOP counts as good
    EXPECT_FALSE(w.weak(13000u));
    EXPECT_TRUE(w.weak(23001u)) << "no reading at all for over 10 s";
    w.reset();
    w.add(0u, false, 1.0f, 4.0f);
    EXPECT_TRUE(w.weak(10001u)) << "no fix";
}

// ---------------------------------------------------------------------------
// TickLog
// ---------------------------------------------------------------------------

TEST(TickLog, StatsOverASplitAndAFlattenedLateJoin)
{
    Run::TickLog log;
    for (uint32_t i = 0; i < 100; ++i) {
        // Climbing 0.5 m a second, HR 140 + i % 10, speed i / 10.
        log.add(1000u * i, 140.0f + static_cast<float>(i % 10), 0.1f * static_cast<float>(i),
                100.0f + 0.5f * static_cast<float>(i), 0.5f * static_cast<float>(i), 0.0f);
    }
    const Run::TickLog::Stats st = log.stats(20000u, 40000u); // ticks 20 to 39
    EXPECT_EQ(st.ticks, 20u);
    EXPECT_NEAR(st.hrAvg, 144.5f, 1e-3f);
    EXPECT_EQ(st.hrMax, 149.0f);
    EXPECT_NEAR(st.speedMax, 3.9f, 0.011f);
    EXPECT_NEAR(st.ascentM, 10.0f, 0.051f); // from tick 19 to tick 39
    EXPECT_FALSE(st.partial);

    float asc = 0.0f, desc = 0.0f, alt = 0.0f;
    ASSERT_TRUE(log.since(60000u, asc, desc, alt));
    EXPECT_NEAR(asc, 19.5f, 0.051f);
    EXPECT_NEAR(alt, 130.0f, 1e-3f);
    log.flattenFrom(60000u, alt);
    const Run::TickLog::Stats after = log.stats(61000u, 100000u); // from tick 60, the join
    EXPECT_NEAR(after.ascentM, 0.0f, 1e-3f) << "the track found late is flat";
}

namespace {

// The unpacked TickLog (16-byte entries with full times), kept as the
// reference for the packed one: same rounding, same answers.
class UnpackedTickLog {
public:
    static constexpr uint16_t kSize = Run::TickLog::kSize;

    void add(uint32_t tMs, float hrBpm, float speedMps, float altM, float ascentM, float descentM)
    {
        if (mCount > 0 && diff(tMs, back(0).tMs) <= 0) {
            return;
        }
        Entry& e   = mE[mHead];
        e.tMs      = tMs;
        e.altM     = altM;
        const float cms = speedMps > 0.0f ? speedMps * 100.0f + 0.5f : 0.0f;
        e.speedCms = static_cast<uint16_t>(cms < 65535.0f ? cms : 65535.0f);
        const float hr = hrBpm > 0.0f ? hrBpm + 0.5f : 0.0f;
        e.hr       = static_cast<uint8_t>(hr < 255.0f ? hr : 255.0f);
        e.ascDm    = toDm(ascentM);
        e.descDm   = toDm(descentM);
        mHead      = static_cast<uint16_t>((mHead + 1u) % kSize);
        if (mCount < kSize) {
            ++mCount;
        }
    }

    Run::TickLog::Stats stats(uint32_t startMs, uint32_t endMs) const
    {
        Run::TickLog::Stats st;
        if (mCount == 0) {
            return st;
        }
        st.partial = diff(back(static_cast<uint16_t>(mCount - 1u)).tMs, startMs) > 0;
        uint32_t hrSum = 0;
        uint32_t hrN   = 0;
        int32_t  first = -1;
        int32_t  last  = -1;
        for (uint16_t b = 0; b < mCount; ++b) {
            const Entry& e = back(b);
            if (diff(e.tMs, endMs) >= 0) {
                continue;
            }
            if (diff(e.tMs, startMs) < 0) {
                break;
            }
            if (last < 0) {
                last = b;
            }
            first = b;
            ++st.ticks;
            if (e.hr > 0) {
                hrSum += e.hr;
                ++hrN;
                if (e.hr > st.hrMax) {
                    st.hrMax = e.hr;
                }
            }
            const float v = 0.01f * static_cast<float>(e.speedCms);
            if (v > st.speedMax) {
                st.speedMax = v;
            }
        }
        if (hrN > 0) {
            st.hrAvg = static_cast<float>(hrSum) / static_cast<float>(hrN);
        }
        if (first >= 0) {
            const uint16_t fromB = static_cast<uint16_t>(first + 1 < mCount ? first + 1 : first);
            st.ascentM  = dmSpan(back(fromB).ascDm, back(static_cast<uint16_t>(last)).ascDm);
            st.descentM = dmSpan(back(fromB).descDm, back(static_cast<uint16_t>(last)).descDm);
        }
        return st;
    }

    bool since(uint32_t tMs, float& ascentM, float& descentM, float& altM) const
    {
        if (mCount == 0) {
            return false;
        }
        uint16_t pick = static_cast<uint16_t>(mCount - 1u);
        for (uint16_t b = 0; b < mCount; ++b) {
            if (diff(back(b).tMs, tMs) <= 0) {
                pick = b;
                break;
            }
        }
        const Entry& e = back(pick);
        ascentM  = dmSpan(e.ascDm, back(0).ascDm);
        descentM = dmSpan(e.descDm, back(0).descDm);
        altM     = e.altM;
        return true;
    }

    void flattenFrom(uint32_t tMs, float altM)
    {
        uint16_t asc = 0, desc = 0;
        bool     have = false;
        for (uint16_t b = 0; b < mCount; ++b) {
            if (diff(back(b).tMs, tMs) <= 0) {
                asc  = back(b).ascDm;
                desc = back(b).descDm;
                have = true;
                break;
            }
        }
        if (!have && mCount > 0) {
            asc  = back(static_cast<uint16_t>(mCount - 1u)).ascDm;
            desc = back(static_cast<uint16_t>(mCount - 1u)).descDm;
            have = true;
        }
        for (uint16_t b = 0; b < mCount; ++b) {
            Entry& e = back(b);
            if (diff(e.tMs, tMs) <= 0) {
                break;
            }
            if (have) {
                e.ascDm  = asc;
                e.descDm = desc;
            }
            e.altM = altM;
        }
    }

    uint16_t count() const { return mCount; }
    uint32_t timeBack(uint16_t b) const { return back(b).tMs; }

private:
    struct Entry {
        uint32_t tMs;
        float    altM;
        uint16_t speedCms;
        uint8_t  hr;
        uint16_t ascDm;
        uint16_t descDm;
    };
    static int32_t diff(uint32_t later, uint32_t earlier) { return static_cast<int32_t>(later - earlier); }
    static uint16_t toDm(float m)
    {
        const double dm = std::floor(static_cast<double>(m) * 10.0 + 0.5);
        return static_cast<uint16_t>(static_cast<int64_t>(dm) & 0xFFFF);
    }
    static float dmSpan(uint16_t from, uint16_t to) { return 0.1f * static_cast<float>(static_cast<uint16_t>(to - from)); }
    const Entry& back(uint16_t b) const { return mE[(mHead + kSize - 1u - b) % kSize]; }
    Entry&       back(uint16_t b) { return mE[(mHead + kSize - 1u - b) % kSize]; }

    Entry    mE[kSize] = {};
    uint16_t mHead  = 0;
    uint16_t mCount = 0;
};

bool sameBits(float a, float b)
{
    uint32_t x = 0, y = 0;
    std::memcpy(&x, &a, sizeof(x));
    std::memcpy(&y, &b, sizeof(y));
    return x == y;
}

::testing::AssertionResult sameStats(const Run::TickLog::Stats& a, const Run::TickLog::Stats& b)
{
    if (a.ticks == b.ticks && sameBits(a.hrAvg, b.hrAvg) && sameBits(a.hrMax, b.hrMax)
        && sameBits(a.speedMax, b.speedMax) && sameBits(a.ascentM, b.ascentM) && sameBits(a.descentM, b.descentM)
        && a.partial == b.partial) {
        return ::testing::AssertionSuccess();
    }
    return ::testing::AssertionFailure() << "ticks " << a.ticks << "/" << b.ticks << ", HR " << a.hrAvg << "/"
                                         << b.hrAvg << ", max " << a.hrMax << "/" << b.hrMax << ", speed "
                                         << a.speedMax << "/" << b.speedMax << ", asc " << a.ascentM << "/"
                                         << b.ascentM << ", desc " << a.descentM << "/" << b.descentM;
}

} // namespace

TEST(TickLog, PackedMatchesUnpacked)
{
    // The packed entries are 14 bytes, not 16 (times in 24 bits off the
    // newest tick). Against the unpacked reference, on the same random run: pauses
    // up to 20 minutes, the sensor clock near its wrap, altitudes
    // missing (NaN), HR none to past 255, speeds past 655 m/s, climbs across
    // the 6.5 km modulo, and a late track's flattening.
    static Run::TickLog packed;
    static UnpackedTickLog old;
    std::mt19937 rng(PortableRandom::seeded(6003u));
    PortableRandom::Uniform u01(0.0, 1.0);
    uint32_t t   = 0xFFFFFFFFu - 400000u; // the 32-bit wrap comes 400 s in
    float    asc = 6400.0f, desc = 0.0f, alt = 120.0f;
    size_t   checks = 0;
    for (int i = 0; i < 5000; ++i) {
        const double r = u01(rng);
        if (r < 0.004) {
            t += static_cast<uint32_t>(60000.0 + 1140000.0 * u01(rng)); // a pause, 1 to 20 minutes
        } else if (r < 0.01) {
            t += 1u; // a tick 1 ms after the last
        } else if (r < 0.012) {
            t -= 500u; // out of order: both drop it
        } else {
            t += static_cast<uint32_t>(900.0 + 200.0 * u01(rng));
        }
        const double h = u01(rng);
        const float  hr = h < 0.1 ? 0.0f : h < 0.12 ? 300.0f : static_cast<float>(60.0 + 140.0 * u01(rng));
        const double v = u01(rng);
        const float  speed = v < 0.05 ? -1.0f : v < 0.06 ? 700.0f : static_cast<float>(12.0 * u01(rng));
        asc += static_cast<float>(2.0 * u01(rng));
        desc += static_cast<float>(1.5 * u01(rng));
        alt += static_cast<float>(u01(rng) - 0.5);
        const float a = u01(rng) < 0.05 ? std::nanf("") : alt;
        packed.add(t, hr, speed, a, asc, desc);
        old.add(t, hr, speed, a, asc, desc);
        ASSERT_EQ(packed.count(), old.count());
        ASSERT_LT(old.timeBack(0) - old.timeBack(static_cast<uint16_t>(old.count() - 1u)), 1u << 24)
            << "the ticks kept span less than 2^24 ms, where the packed times are exact";

        if (i % 37 == 0 && old.count() > 1) {
            // Windows from before the oldest tick to past the newest.
            for (int k = 0; k < 6; ++k) {
                const uint16_t n  = old.count();
                const uint16_t b1 = static_cast<uint16_t>(u01(rng) * n);
                const uint16_t b0 = static_cast<uint16_t>(b1 + u01(rng) * (n - b1));
                const uint32_t start = old.timeBack(b0 < n ? b0 : n - 1u) - static_cast<uint32_t>(1500.0 * u01(rng));
                const uint32_t end   = old.timeBack(b1) + static_cast<uint32_t>(1500.0 * u01(rng));
                EXPECT_TRUE(sameStats(packed.stats(start, end), old.stats(start, end))) << "tick " << i;
                ++checks;
            }
            float pa = 0, pd = 0, pl = 0, oa = 0, od = 0, ol = 0;
            const uint32_t at = old.timeBack(static_cast<uint16_t>(u01(rng) * old.count())) + 300u;
            ASSERT_EQ(packed.since(at, pa, pd, pl), old.since(at, oa, od, ol));
            EXPECT_TRUE(sameBits(pa, oa) && sameBits(pd, od) && sameBits(pl, ol)) << "since, tick " << i;
        }
        if (i % 900 == 450) {
            const uint32_t at = old.timeBack(static_cast<uint16_t>(u01(rng) * old.count()));
            packed.flattenFrom(at, alt);
            old.flattenFrom(at, alt);
        }
    }
    EXPECT_GT(checks, 700u);
    EXPECT_LE(sizeof(Run::TickLog), 900u * 14u + 16u + 28u) << "900 ticks of 14 bytes, and the carry";
}

TEST(TickLog, ASplitLongerThanTheLogKeepsItsWholeStats)
{
    // A 40-minute split from the anchor: HR 140 for the first 10 minutes,
    // with a 9 m/s burst and a 50 m climb there, then HR 160 at 3 m/s on
    // the level. Only the last 15 minutes stay in the log.
    Run::TickLog log;
    log.anchor(1000u);
    float asc = 0.0f;
    for (uint32_t s = 1; s <= 2400; ++s) {
        const bool early = s <= 600;
        if (early && s <= 500) {
            asc += 0.1f;
        }
        log.add(1000u + s * 1000u, early ? 140.0f : 160.0f, s == 100 ? 9.0f : 3.0f, 20.0f, asc, 0.0f);
    }
    const Run::TickLog::Stats st = log.stats(1000u, 1000u + 2401u * 1000u);
    EXPECT_FALSE(st.partial);
    EXPECT_EQ(st.ticks, 2400u);
    EXPECT_NEAR(st.hrAvg, (600.0f * 140.0f + 1800.0f * 160.0f) / 2400.0f, 0.01f);
    EXPECT_EQ(st.hrMax, 160.0f);
    EXPECT_NEAR(st.speedMax, 9.0f, 0.01f) << "the burst left the log, and still counts";
    EXPECT_NEAR(st.ascentM, 50.0f, 0.15f);

    // The next split, from the anchor at its start, is not carried from
    // the first one's ticks.
    log.anchor(1000u + 2401u * 1000u);
    for (uint32_t s = 2401; s <= 2460; ++s) {
        log.add(1000u + s * 1000u, 150.0f, 4.0f, 20.0f, asc, 0.0f);
    }
    const Run::TickLog::Stats st2 = log.stats(1000u + 2401u * 1000u, 1000u + 2461u * 1000u);
    EXPECT_FALSE(st2.partial);
    EXPECT_EQ(st2.ticks, 60u);
    EXPECT_EQ(st2.hrMax, 150.0f);
    EXPECT_NEAR(st2.speedMax, 4.0f, 0.01f);
}

TEST(TickLog, ASplitWhoseTicksHaveAllLeftKeepsItsClimb)
{
    // A 100 s split climbing 10 m, still held when every tick of it has left
    // the log, and the oldest tick kept is at its end: its climb stands.
    Run::TickLog log;
    log.anchor(0u);
    for (uint32_t s = 1; s <= 1000; ++s) {
        const float asc = s <= 101 ? 0.1f * static_cast<float>(s) : 10.1f;
        log.add(s * 1000u, 150.0f, 4.0f, 20.0f, asc, 0.0f);
    }
    const Run::TickLog::Stats st = log.stats(0u, 101000u);
    EXPECT_FALSE(st.partial);
    EXPECT_EQ(st.ticks, 100u);
    EXPECT_NEAR(st.ascentM, 10.0f, 0.15f) << "from the first tick's 0.1 m to the end's 10.1 m";
}

TEST(TickLog, ASplitOver18HoursCountsEveryTick)
{
    // 20 hours of 1 Hz ticks in one split (auto-lap Off): 72,000 ticks, more
    // than 16 bits hold.
    Run::TickLog log;
    log.anchor(0u);
    const uint32_t n = 72000u;
    for (uint32_t s = 1; s <= n; ++s) {
        log.add(s * 1000u, s <= n / 2 ? 120.0f : 160.0f, 3.0f, 20.0f, 0.0f, 0.0f);
    }
    const Run::TickLog::Stats st = log.stats(0u, (n + 1u) * 1000u);
    EXPECT_FALSE(st.partial);
    EXPECT_EQ(st.ticks, n);
    EXPECT_NEAR(st.hrAvg, 140.0f, 0.01f);
}

TEST(TickLog, TwoHeldSplitsPoppedLateAreBothPartial)
{
    // Split A [0, 300 s) and split B [300, 1100 s), both held until 1300 s
    // of 1 Hz ticks: the log keeps 401 to 1300 s. A is partial; B, anchored
    // only when A is handed out, lost its ticks 300 to 400 s before that,
    // and must say so.
    Run::TickLog log;
    log.anchor(0u);
    for (uint32_t s = 1; s <= 1300; ++s) {
        log.add(s * 1000u, 150.0f, 4.0f, 20.0f, 0.0f, 0.0f);
    }
    const Run::TickLog::Stats a = log.stats(0u, 300000u);
    EXPECT_TRUE(a.partial);
    log.anchor(300000u);
    const Run::TickLog::Stats b = log.stats(300000u, 1100000u);
    EXPECT_TRUE(b.partial) << "its first 100 s are gone";
    EXPECT_EQ(b.ticks, 699u) << "401 to 1099 s";
}

TEST(TickLog, AHeldSplitOlderThanTheLogStaysPartial)
{
    // The split from the anchor ends after 1 minute, but is still held when
    // its ticks and 15 minutes of later ones have left the log (a start that
    // is not decided keeps every split held). The carry then has the later
    // splits' ticks: the first split must not take them as its own.
    Run::TickLog log;
    log.anchor(1000u);
    for (uint32_t s = 1; s <= 1000; ++s) {
        const bool first = s <= 60;
        log.add(1000u + s * 1000u, first ? 120.0f : 170.0f, first ? 3.0f : 6.0f, 20.0f, 0.0f, 0.0f);
    }
    const Run::TickLog::Stats st = log.stats(1000u, 1000u + 61u * 1000u);
    EXPECT_TRUE(st.partial) << "its ticks are gone, with later ones";
    EXPECT_EQ(st.hrMax, 0.0f) << "no later split's heart rate";
    EXPECT_EQ(st.speedMax, 0.0f) << "no later split's speed";
}

TEST(TraceFormat, FixedDecimals)
{
    char b[32];
    auto f = [&](double v, int d) { const size_t n = Run::TraceFormat::fixed(b, sizeof b, v, d); return std::string(b, n); };
    EXPECT_EQ(f(55.9496536, 7), "55.9496536");
    EXPECT_EQ(f(-3.18996914, 7), "-3.1899691");
    EXPECT_EQ(f(0.004, 2), "0.00");
    EXPECT_EQ(f(-0.004, 2), "0.00");
    EXPECT_EQ(f(1234.5, 0), "1235");
    EXPECT_EQ(f(9.9951, 2), "10.00");
    EXPECT_EQ(Run::TraceFormat::fixed(b, sizeof b, std::nan(""), 2), 0u);
}

TEST(TraceFormat, WatchLinesReadBackWithTheHostReader)
{
    std::string text;
    char line[Run::TraceFormat::kMaxLineBytes];
    auto put = [&](size_t n) { ASSERT_GT(n, 0u); text.append(line, n); };
    put(Run::TraceFormat::versionLine(line, sizeof line));
    put(Run::TraceFormat::metaLine(line, sizeof line, "start_ms", 1005500u));
    put(Run::TraceFormat::metaLine(line, sizeof line, "lane", 2u));
    put(Run::TraceFormat::metaLine(line, sizeof line, "store", "track v=1 n=1 lat=55.9 crc=0"));
    put(Run::TraceFormat::columnsLine(line, sizeof line));
    Run::TraceFormat::Row r;
    r.tMs = 1004000u; r.fix = true; r.lat = 55.9496536f; r.lon = -3.1899691f; r.altM = 50.1f; r.hdop = 0.8f;
    r.mode = "pre"; r.lane = 2;
    put(Run::TraceFormat::rowLine(line, sizeof line, r));
    r.tMs = 1006000u; r.gpsDistM = 1234.5f; r.trackDistM = 0.0f; r.lap = 0; r.mode = "count"; r.segment = "track";
    put(Run::TraceFormat::rowLine(line, sizeof line, r));
    put(Run::TraceFormat::actionLine(line, sizeof line, "lap_press", 1006500u));
    r.tMs = 1007000u; r.fix = false; r.hdop = -1.0f; r.altM = std::nanf(""); r.trackDistM = 3.25f; r.lap = 1;
    r.mode = "cal"; r.segment = "track";
    put(Run::TraceFormat::rowLine(line, sizeof line, r));

    std::istringstream in(text);
    TraceCsv::File f;
    std::string err;
    ASSERT_TRUE(TraceCsv::parse(in, f, err)) << err;
    EXPECT_EQ(f.version, 2);
    EXPECT_EQ(f.metaValue("start_ms"), "1005500");
    EXPECT_EQ(f.metaValue("store"), "track v=1 n=1 lat=55.9 crc=0");
    ASSERT_EQ(f.rows.size(), 3u);
    EXPECT_EQ(f.rows[0].s.tMs, 1004000u);
    EXPECT_NEAR(f.rows[0].s.lat, 55.9496536, 1e-7);
    EXPECT_NEAR(f.rows[0].s.hdop, 0.8f, 1e-6f);
    EXPECT_EQ(f.rows[0].mode, "pre");
    EXPECT_FALSE(std::isfinite(f.rows[0].s.gpsDistM));
    EXPECT_NEAR(f.rows[1].s.gpsDistM, 1234.5f, 1e-3f);
    EXPECT_EQ(f.rows[1].lap, 0);
    EXPECT_FALSE(f.rows[2].s.fix);
    EXPECT_LT(f.rows[2].s.hdop, 0.0f);
    EXPECT_NEAR(f.rows[2].trackDistM, 3.25f, 1e-4f);
    EXPECT_EQ(f.rows[2].lane, 2);
}
