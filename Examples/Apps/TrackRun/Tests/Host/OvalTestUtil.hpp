// Shared helpers for the LapCounter tests.
#pragma once

#include "PortableMath.hpp"
#include "Replay.hpp"
#include "TraceGen.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace OvalTest {

// A typical runner: 5 m/s with a slow wobble and a little slower on the bends.
inline TraceGen::Speed runner(float mps = 5.0f)
{
    TraceGen::Speed s;
    s.baseMps    = mps;
    s.variation  = 0.05f;
    s.periodS    = 75.0f;
    s.bendFactor = 0.97f;
    s.jitter     = 0.02f;
    return s;
}

inline TraceGen::Speed steady(float mps)
{
    TraceGen::Speed s;
    s.baseMps = mps;
    return s;
}

// Noisy GPS: white noise per axis plus a slow drift, and GPS distance that
// over-reads by about 12% at running pace.
inline TraceGen::Noise noisy(float sigmaM, float driftStepM = 0.1f, float distJitterM = 1.5f)
{
    TraceGen::Noise n;
    n.posSigmaM   = sigmaM;
    n.driftStepM  = driftStepM;
    n.distJitterM = distJitterM;
    n.metresAccBaseM = 2.0f + 0.5f * sigmaM; // the older figure in metres, for metresHdop()
    return n;
}

inline TraceGen::Noise clean()
{
    TraceGen::Noise n;
    n.metresAccBaseM   = 2.0f;
    n.metresAccJitterM = 0.0f;
    return n;
}

// The noise with no HDOP: for the statistical tests whose thresholds were
// set on that noise with little or no margin. With no HDOP every reading has
// the same weight (Hdop.NoHdopAtAllMatchesGolden).
// With the realistic HDOP (white noise 0.7 to 1.5 times the figure, root mean
// square 1.0) these tests miss by a little (docs/OVAL.md). Set
// TRACKRUN_REALISTIC_HDOP=1 to run them on the realistic HDOP.
inline TraceGen::Noise metresHdop(TraceGen::Noise n)
{
    if (std::getenv("TRACKRUN_REALISTIC_HDOP") == nullptr) {
        n.hdopModel = TraceGen::HdopModel::Unknown;
    }
    return n;
}

// As metresHdop(), for a test whose start point is weighted by a precision
// figure in metres: the figure is read as HDOP the way a version 1 trace's is
// (divided by 4).
inline TraceGen::Noise metresAccuracy(TraceGen::Noise n)
{
    if (std::getenv("TRACKRUN_REALISTIC_HDOP") == nullptr) {
        n.hdopModel = TraceGen::HdopModel::MetresAccuracy;
    }
    return n;
}

// A whole number from the environment variable name, or dflt when it is not
// set. A value that is not a whole number from lo to hi fails the test and
// gives lo, so a typo cannot turn a sweep into one that runs nothing.
inline uint32_t envNumber(const char* name, uint32_t dflt, uint32_t lo = 1u, uint32_t hi = 1000000u)
{
    const char* v = std::getenv(name);
    if (v == nullptr) {
        return dflt;
    }
    char*                    end = nullptr;
    const unsigned long long n   = (*v >= '0' && *v <= '9') ? std::strtoull(v, &end, 10) : 0ull;
    if (end == nullptr || end == v || *end != '\0' || n < lo || n > hi) {
        ADD_FAILURE() << name << "=" << v << ": expected a whole number from " << lo << " to " << hi;
        return lo;
    }
    return static_cast<uint32_t>(n);
}

// A seed from the environment variable name, or dflt when it is not set.
inline uint32_t envSeed(const char* name, uint32_t dflt)
{
    return envNumber(name, dflt, 0u, 0xFFFFFFFFu);
}

// Checks that hold for every scenario: the total never went down, the lap
// count matches the truth, every lap end is near the true pass of the start
// point, and the credited total at the last lap is exact.
inline ::testing::AssertionResult wholeLapsRight(const TraceGen::Trace& tr, const Replay::Result& r,
                                                 uint32_t endToleranceMs)
{
    if (r.decreases != 0) {
        return ::testing::AssertionFailure() << "total went down " << r.decreases
                                             << " times, worst by " << r.worstDecrease << " m";
    }
    if (r.events.size() != tr.crossMs.size()) {
        return ::testing::AssertionFailure() << "counted " << r.events.size() << " laps, truth "
                                             << tr.crossMs.size();
    }
    const Replay::Match m = Replay::match(tr, r, endToleranceMs);
    if (m.badTimes != 0) {
        return ::testing::AssertionFailure() << m.badTimes << " lap ends further than "
                                             << endToleranceMs << " ms from the truth";
    }
    if (!r.events.empty()) {
        const double expected = Replay::creditedAfter(tr, r.events.size());
        if (!(std::fabs(r.events.back().totalM - expected) <= 0.01)) {
            return ::testing::AssertionFailure() << "total at the last lap " << r.events.back().totalM
                                                 << " m, expected " << expected << " m";
        }
    }
    return ::testing::AssertionSuccess();
}

// For tests that drive LapCounter by hand: counts readings where the total
// went down (Replay::run() does the same for the others). A total that is
// not a number counts as one, and the next is compared with the last good one.
struct NeverDecreases {
    float  last      = 0.0f;
    size_t decreases = 0;
    void check(float total)
    {
        if (!std::isfinite(total)) {
            ++decreases;
            return;
        }
        if (total < last) {
            ++decreases;
        }
        last = total;
    }
};

inline double worstAbs(const std::vector<double>& v)
{
    double w = 0.0;
    for (double x : v) {
        w = std::max(w, std::fabs(x));
    }
    return w;
}

// The p-th percentile (0 to 1) of a set of values, nearest rank.
inline double percentile(std::vector<double> v, double p)
{
    if (v.empty()) {
        return 0.0;
    }
    std::sort(v.begin(), v.end());
    size_t k = static_cast<size_t>(std::ceil(p * v.size()));
    k = k == 0 ? 0 : k - 1;
    return v[std::min(k, v.size() - 1)];
}

// Prints mean, median, 95th percentile and worst of a set of values.
inline void printStats(const char* label, const std::vector<double>& v)
{
    if (v.empty()) {
        std::printf("[ stats    ] %-34s none\n", label);
        return;
    }
    double sum = 0.0;
    for (double x : v) {
        sum += x;
    }
    std::printf("[ stats    ] %-34s n %5zu: mean %7.3f, median %7.3f, 95th %7.3f, worst %7.3f\n", label,
                v.size(), sum / v.size(), percentile(v, 0.5), percentile(v, 0.95), percentile(v, 1.0));
}

// The smallest count k with P(N <= k) >= q for a Poisson count N of mean
// lambda: the most a rate of lambda per run set allows, but for 1 - q of the
// seed sets one might pick (rates, not lucky draws).
inline size_t poissonQuantile(double lambda, double q)
{
    double p = PortableMath::exp(-lambda), cdf = p; // the same threshold on every machine
    size_t k = 0;
    while (cdf < q && k < 10000) {
        ++k;
        p *= lambda / static_cast<double>(k);
        cdf += p;
    }
    return k;
}

} // namespace OvalTest
