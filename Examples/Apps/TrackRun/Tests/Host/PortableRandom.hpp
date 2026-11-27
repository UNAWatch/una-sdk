// Random draws that are the same on every compiler, for the host tests.
//
// The C++ standard fixes the raw output of std::mt19937, but not how
// std::uniform_real_distribution, std::normal_distribution or
// std::uniform_int_distribution turn it into numbers: MSVC and GCC give
// different values from the same seed (for normals, even the order of each
// pair differs), so the same seed would make a different trace on each
// compiler. These replacements take the same arguments and are called the
// same way, but are built only on mt19937's raw output and on + - * / and
// sqrt, which IEEE 754 rounds the same way everywhere. Even the natural log
// the normal needs is written out (PortableMath.hpp), so the maths library
// cannot make two machines differ either.
//
// - Uniform: 53 bits from two raw draws (genrand_res53), exact on any compiler.
// - UniformInt: the 64-bit multiply-shift of one raw draw. Its bias is under
//   n / 2^32, below 1e-7 for the ranges the tests use.
// - Normal: Box-Muller in Marsaglia's polar form (no sine or cosine). Each
//   accepted pair gives two values; the second is kept for the next call,
//   as the standard libraries do.
//
// TRACKRUN_SEED_SET (0 by default) moves every randomised test onto another
// set of draws: seeded() mixes it into a seed, and leaves the seed as it is
// when it is 0 or unset.
//
// Host only. Not part of the watch build.
#pragma once

#include "PortableMath.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <random>

namespace PortableRandom {

// A uniform draw in [0, 1) with 53 random bits.
inline double uniform01(std::mt19937& rng)
{
    const uint32_t a = rng() >> 5; // 27 bits
    const uint32_t b = rng() >> 6; // 26 bits
    return (a * 67108864.0 + b) * (1.0 / 9007199254740992.0);
}

// As std::uniform_real_distribution<double>: a value in [lo, hi).
class Uniform {
public:
    explicit Uniform(double lo = 0.0, double hi = 1.0) : mLo(lo), mHi(hi) {}
    double operator()(std::mt19937& rng) const
    {
        // Rounding can land on hi when lo and hi are close: the value below.
        const double v = mLo + (mHi - mLo) * uniform01(rng);
        return v < mHi ? v : std::nextafter(mHi, mLo);
    }

private:
    double mLo;
    double mHi;
};

// As std::uniform_int_distribution<int>: a value in [lo, hi], both included.
class UniformInt {
public:
    UniformInt(int lo, int hi) : mLo(lo), mN(static_cast<uint64_t>(static_cast<int64_t>(hi) - lo + 1)) {}
    int operator()(std::mt19937& rng) const
    {
        // Added as 64 bits: the offset alone can be past INT_MAX.
        const uint64_t off = (static_cast<uint64_t>(rng()) * mN) >> 32;
        return static_cast<int>(static_cast<int64_t>(mLo) + static_cast<int64_t>(off));
    }

private:
    int      mLo;
    uint64_t mN;
};

// As std::normal_distribution<double>.
class Normal {
public:
    explicit Normal(double mean = 0.0, double sd = 1.0) : mMean(mean), mSd(sd) {}
    double operator()(std::mt19937& rng)
    {
        if (mHaveSpare) {
            mHaveSpare = false;
            return mMean + mSd * mSpare;
        }
        double u = 0.0, v = 0.0, s = 0.0;
        do {
            u = 2.0 * uniform01(rng) - 1.0;
            v = 2.0 * uniform01(rng) - 1.0;
            s = u * u + v * v;
        } while (s >= 1.0 || s == 0.0);
        const double f = std::sqrt(-2.0 * PortableMath::ln(s) / s);
        mSpare     = v * f;
        mHaveSpare = true;
        return mMean + mSd * (u * f);
    }

private:
    double mMean;
    double mSd;
    double mSpare     = 0.0;
    bool   mHaveSpare = false;
};

// Shorthands for the common calls.
inline double uniform(std::mt19937& rng, double lo, double hi) { return Uniform(lo, hi)(rng); }
inline int    pick(std::mt19937& rng, int lo, int hi) { return UniformInt(lo, hi)(rng); }

// The seed set from TRACKRUN_SEED_SET: 0 (the default) is the suite's own draws.
inline uint32_t seedSet()
{
    const char* v = std::getenv("TRACKRUN_SEED_SET");
    return v ? static_cast<uint32_t>(std::strtoul(v, nullptr, 10)) : 0u;
}

// A seed moved onto the current seed set (unchanged in set 0).
inline uint32_t seeded(uint32_t seed)
{
    const uint32_t set = seedSet();
    if (set == 0u) {
        return seed;
    }
    // A splitmix-style mix, so nearby seeds in nearby sets do not overlap.
    uint32_t x = seed ^ (set * 0x9E3779B9u);
    x = (x ^ (x >> 16)) * 0x85EBCA6Bu;
    x = (x ^ (x >> 13)) * 0xC2B2AE35u;
    return x ^ (x >> 16);
}

} // namespace PortableRandom
