// PortableRandom: the same draws, and so the same traces, on every
// compiler. The pinned values below were printed by an MSVC build and must
// come out bit for bit on GCC and Clang as well. They are fixed inputs, so
// TRACKRUN_SEED_SET does not apply here.

#include "OvalTestUtil.hpp"
#include "PortableRandom.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <limits>
#include <cstring>
#include <random>

namespace {

// FNV-1a over a trace's readings: time, position, HDOP, GPS distance.
uint64_t traceHash(const TraceGen::Trace& tr)
{
    uint64_t h = 1469598103934665603ull;
    auto mix = [&](const void* p, size_t n) {
        const unsigned char* b = static_cast<const unsigned char*>(p);
        for (size_t i = 0; i < n; ++i) {
            h = (h ^ b[i]) * 1099511628211ull;
        }
    };
    for (const auto& s : tr.samples) {
        mix(&s.tMs, sizeof s.tMs);
        mix(&s.lat, sizeof s.lat);
        mix(&s.lon, sizeof s.lon);
        mix(&s.hdop, sizeof s.hdop);
        mix(&s.gpsDistM, sizeof s.gpsDistM);
    }
    return h;
}

// Ten laps with every kind of noise the generator has: white noise, drift,
// realistic HDOP with random spells, bad fixes, a dropout and a pause.
TraceGen::Trace pinnedTrace()
{
    TraceGen::Track track;
    track.headingDeg  = 37.0f;
    track.kerbRadiusM = 36.9f;
    TraceGen::Noise n = OvalTest::noisy(3.5f);
    n.hdopSpellsPerHour = 6.0f;
    n.badFixProb        = 0.02f;
    n.periodJitterMs    = 40;
    TraceGen::Generator g(track, n, 20261023u);
    g.begin(0.25f, 4);
    g.runLaps(4.0, OvalTest::runner(4.5f));
    g.dropout(true);
    g.run(60.0, OvalTest::runner(4.5f));
    g.dropout(false);
    g.pause();
    g.stand(15.0);
    g.resume();
    g.changeLane(6);
    g.runLaps(5.5, OvalTest::runner(5.0f));
    return g.finish();
}

} // namespace

struct Pinned {
    uint32_t seed;
    double   u[3];
    int      k[3];
    double   z[3];
};

// The first three uniform01(), pick(1, 8) and Normal() draws, in that order.
const Pinned kPinned[] = {
    {1u,
     {0x1.ab07d0ffa3c06p-2, 0x1.70ce5f3baf051p-1, 0x1.dfb906661cp-14},
     {3, 8, 2},
     {-0x1.6722aa901a226p+0, -0x1.197854b3b5ccp-1, -0x1.0b8ebe1fb87d5p+0}},
    {20261023u,
     {0x1.53d7bda2a3a59p-1, 0x1.c8b4d4bffe829p-1, 0x1.957904d8157b6p-1},
     {1, 5, 5},
     {0x1.00d885e2497d2p-1, -0x1.ce9bd1232bfeep-1, -0x1.9124c870253d2p-3}},
};

constexpr uint64_t kPinnedTraceHash = 0x911b7ed2e55c037dull;

TEST(PortableRandom, PinnedValues)
{
    for (const Pinned& p : kPinned) {
        std::mt19937 rng(p.seed);
        for (double want : p.u) {
            EXPECT_EQ(PortableRandom::uniform01(rng), want) << "seed " << p.seed;
        }
        for (int want : p.k) {
            EXPECT_EQ(PortableRandom::pick(rng, 1, 8), want) << "seed " << p.seed;
        }
        PortableRandom::Normal norm;
        for (double want : p.z) {
            EXPECT_EQ(norm(rng), want) << "seed " << p.seed;
        }
    }
}

// A whole generated trace, reading for reading, so the generator's own use
// of the draws (and its order of them) is pinned too.
TEST(PortableRandom, PinnedTrace)
{
    if (PortableRandom::seedSet() != 0u) {
        GTEST_SKIP() << "fixed inputs: not run on another seed set";
    }
    const TraceGen::Trace tr = pinnedTrace();
    EXPECT_EQ(traceHash(tr), kPinnedTraceHash) << tr.samples.size() << " readings";
}

// The draws have the right distributions: 10^6 of each.
TEST(PortableRandom, Distributions)
{
    std::mt19937 rng(7u);
    const int n = 1000000;
    double su = 0.0, su2 = 0.0, sz = 0.0, sz2 = 0.0, sz4 = 0.0;
    int    counts[8] = {};
    size_t beyond3 = 0;
    PortableRandom::Normal norm;
    for (int i = 0; i < n; ++i) {
        const double u = PortableRandom::uniform01(rng);
        ASSERT_GE(u, 0.0);
        ASSERT_LT(u, 1.0);
        su += u;
        su2 += u * u;
        ++counts[PortableRandom::pick(rng, 1, 8) - 1];
        const double z = norm(rng);
        sz += z;
        sz2 += z * z;
        sz4 += z * z * z * z;
        beyond3 += std::fabs(z) > 3.0 ? 1 : 0;
    }
    const double mu = su / n, vu = su2 / n - mu * mu;
    const double mz = sz / n, vz = sz2 / n - mz * mz;
    std::printf("[ portable ] uniform mean %.5f var %.5f; normal mean %.5f var %.5f kurtosis %.3f, |z| > 3: %.5f\n", mu,
                vu, mz, vz, sz4 / n, static_cast<double>(beyond3) / n);
    EXPECT_NEAR(mu, 0.5, 0.002);
    EXPECT_NEAR(vu, 1.0 / 12.0, 0.001);
    for (int c : counts) {
        EXPECT_NEAR(c / static_cast<double>(n), 0.125, 0.002);
    }
    EXPECT_NEAR(mz, 0.0, 0.005);
    EXPECT_NEAR(vz, 1.0, 0.005);
    EXPECT_NEAR(sz4 / n, 3.0, 0.05);                              // normal tails, not a sum of uniforms
    EXPECT_NEAR(static_cast<double>(beyond3) / n, 0.0027, 0.0003); // P(|z| > 3) = 0.27%
}

namespace {
// The larger error so far; one that is not a number is the worst there is.
double worse(double worst, double err)
{
    return std::isfinite(err) ? std::max(worst, err) : std::numeric_limits<double>::infinity();
}
} // namespace

// The written-out log agrees with the maths library's to within 4 ulp (2 on
// this PC's; what matters is that it is the same everywhere, not the last bit).
TEST(PortableRandom, LogMatchesTheLibrary)
{
    std::mt19937 rng(11u);
    double worst = 0.0;
    for (int i = 0; i < 200000; ++i) {
        const double u = PortableRandom::uniform01(rng);
        const double x = (i % 2) ? u + 1e-300 : std::ldexp(u + 0.5, i % 60 - 30);
        const double want = std::log(x);
        const double got  = PortableMath::ln(x);
        const double ulp  = std::fabs(std::nextafter(want, 2.0 * want + 1.0) - want);
        worst = worse(worst, want == 0.0 ? std::fabs(got) : std::fabs(got - want) / ulp);
    }
    std::printf("[ portable ] ln against std::log: worst %.2f ulp\n", worst);
    EXPECT_LE(worst, 4.0);
    EXPECT_EQ(PortableMath::ln(1.0), 0.0);
}

// The written-out sin, cos, exp and atan2 agree with the maths library's
// over the ranges the generator uses (angles to a few thousand radians: the
// speed wobble's phase over an hour), to well under a micrometre in a metre.
TEST(PortableMath, MatchesTheLibrary)
{
    std::mt19937 rng(13u);
    double sinErr = 0.0, cosErr = 0.0, expErr = 0.0, atanErr = 0.0;
    for (int i = 0; i < 200000; ++i) {
        const double a = PortableRandom::uniform(rng, -4000.0, 4000.0);
        sinErr = worse(sinErr, std::fabs(PortableMath::sin(a) - std::sin(a)));
        cosErr = worse(cosErr, std::fabs(PortableMath::cos(a) - std::cos(a)));
        const double x = PortableRandom::uniform(rng, -40.0, 40.0);
        expErr = worse(expErr, std::fabs(PortableMath::exp(x) / std::exp(x) - 1.0));
        const double y = PortableRandom::uniform(rng, -100.0, 100.0);
        const double z = PortableRandom::uniform(rng, -100.0, 100.0);
        atanErr = worse(atanErr, std::fabs(PortableMath::atan2(y, z) - std::atan2(y, z)));
    }
    std::printf("[ portable ] against the library: sin %.1e, cos %.1e, exp %.1e (relative), atan2 %.1e\n", sinErr,
                cosErr, expErr, atanErr);
    EXPECT_LT(sinErr, 1e-12);
    EXPECT_LT(cosErr, 1e-12);
    EXPECT_LT(expErr, 1e-14);
    EXPECT_LT(atanErr, 1e-14);
    EXPECT_EQ(PortableMath::sin(0.0), 0.0);
    EXPECT_EQ(PortableMath::cos(0.0), 1.0);
    EXPECT_EQ(PortableMath::exp(0.0), 1.0);
    EXPECT_EQ(PortableMath::atan2(0.0, 1.0), 0.0);
}

// Their bits, pinned: printed by an MSVC build, and the same with GCC on
// glibc (x86-64, and ARM under qemu), MinGW and Clang.
TEST(PortableMath, PinnedValues)
{
    struct Pin {
        double x, sin, cos, exp, atan2; // exp of -|x| / 50, atan2(x, 3)
    };
    const Pin pins[] = {
        {0.5, 0x1.eaee8744b05fp-2, 0x1.c1528065b7d5p-1, 0x1.fae7cfd2b9cfep-1, 0x1.52397843c9adep-3},
        {2.0, 0x1.d18f6ead1b446p-1, -0x1.aa22657537205p-2, 0x1.ebec97e700b8dp-1, 0x1.2d0ead6066396p-1},
        {-7.25, -0x1.a56adb62a27b8p-1, 0x1.22c6f50dc3fbfp-1, 0x1.bae433b671705p-1, -0x1.2daf4debf28bcp+0},
        {123.456, -0x1.9b9dadc41aeb6p-1, -0x1.307e5980a1559p-1, 0x1.5ac3bce5aa721p-4, 0x1.8be77c86d33dbp+0},
        {3000.0, 0x1.c0e6ac593f0c1p-3, -0x1.f38c9e0798e67p-1, 0x1.5ae191a99585ap-87, 0x1.91de2c0e658bcp+0},
    };
    for (const Pin& p : pins) {
        EXPECT_EQ(PortableMath::sin(p.x), p.sin) << p.x;
        EXPECT_EQ(PortableMath::cos(p.x), p.cos) << p.x;
        EXPECT_EQ(PortableMath::exp(-std::fabs(p.x) / 50.0), p.exp) << p.x;
        EXPECT_EQ(PortableMath::atan2(p.x, 3.0), p.atan2) << p.x;
    }
}
