// Maths functions that give the same bits on every machine, for the test
// generator.
//
// The maths library's sin, cos, exp, atan2, hypot and log are free to differ
// in the last bit from one library to another (Windows' UCRT, glibc, glibc on
// ARM), so a trace built with them would come out a little different on each. These
// are built only on + - * /, sqrt and exact operations (floor, frexp, ldexp),
// which IEEE 754 rounds the same way everywhere, as long as no multiply-add is
// fused (Tests/Host/CMakeLists.txt turns that off for GCC and Clang; MSVC does
// not fuse by default). They agree with the library to a few units in the
// last place; what matters is that they agree with themselves everywhere.
//
// Host only. Not part of the watch build.
#pragma once

#include <cmath>
#include <cstdint>

namespace PortableMath {

constexpr double kPi = 3.14159265358979323846;

// ln(x) for finite x > 0: x = m 2^e with m in [sqrt(1/2), sqrt(2)), and
// ln(m) = 2 atanh(z), z = (m - 1) / (m + 1), by its series (|z| <= 0.172, so
// 12 terms reach double precision).
inline double ln(double x)
{
    int    e = 0;
    double m = std::frexp(x, &e); // exact
    if (m < 0.70710678118654752440) {
        m *= 2.0;
        --e;
    }
    const double z  = (m - 1.0) / (m + 1.0);
    const double z2 = z * z;
    double s = 1.0 / 23.0;
    for (int k = 21; k >= 1; k -= 2) {
        s = s * z2 + 1.0 / k;
    }
    return 2.0 * z * s + e * 0.69314718055994530942;
}

// exp(x): x = k ln 2 + r with |r| <= ln 2 / 2 (ln 2 in two parts, so k ln 2
// is exact for the k the tests reach), then the series of exp(r) to r^14.
inline double exp(double x)
{
    const double k  = std::floor(x * 1.44269504088896340736 + 0.5);
    const double r  = (x - k * 6.93147180369123816490e-01) - k * 1.90821492927058770002e-10;
    double p = 1.0;
    for (int n = 14; n >= 1; --n) {
        p = 1.0 + p * r / n;
    }
    return std::ldexp(p, static_cast<int>(k)); // exact
}

namespace detail {

// x = q pi/2 + r with |r| <= pi/4 (pi/2 in two parts, fdlibm's, so q pi/2 is
// exact for |q| up to about 2^19: angles to some 800,000 radians).
inline double reduce(double x, int& q)
{
    const double k = std::floor(x * 0.63661977236758134308 + 0.5);
    q = static_cast<int>(static_cast<int64_t>(k) & 3);
    return (x - k * 1.57079632673412561417e+00) - k * 6.07710050650619224932e-11;
}

// The series of sin and cos for |r| <= pi/4, to r^17 and r^18.
inline double sinSeries(double r)
{
    const double r2 = r * r;
    double s = 1.0;
    for (int n = 17; n >= 3; n -= 2) {
        s = 1.0 - s * r2 / (n * (n - 1));
    }
    return r * s;
}

inline double cosSeries(double r)
{
    const double r2 = r * r;
    double c = 1.0;
    for (int n = 18; n >= 2; n -= 2) {
        c = 1.0 - c * r2 / (n * (n - 1));
    }
    return c;
}

// atan(x) for |x| <= 1: halved twice by atan(x) = 2 atan(x / (1 + sqrt(1 + x^2)))
// (to |x| <= tan(pi/16)), then its series to x^29.
inline double atanSmall(double x)
{
    for (int i = 0; i < 2; ++i) {
        x = x / (1.0 + std::sqrt(1.0 + x * x));
    }
    const double x2 = x * x;
    double s = 1.0 / 29.0;
    for (int n = 27; n >= 1; n -= 2) {
        s = 1.0 / n - x2 * s;
    }
    return 4.0 * x * s;
}

} // namespace detail

inline double sin(double x)
{
    int q = 0;
    const double r = detail::reduce(x, q);
    switch (q) {
    case 0: return detail::sinSeries(r);
    case 1: return detail::cosSeries(r);
    case 2: return -detail::sinSeries(r);
    default: return -detail::cosSeries(r);
    }
}

inline double cos(double x)
{
    int q = 0;
    const double r = detail::reduce(x, q);
    switch (q) {
    case 0: return detail::cosSeries(r);
    case 1: return -detail::sinSeries(r);
    case 2: return -detail::cosSeries(r);
    default: return detail::sinSeries(r);
    }
}

inline double atan(double x)
{
    if (x > 1.0) {
        return 0.5 * kPi - detail::atanSmall(1.0 / x);
    }
    if (x < -1.0) {
        return -0.5 * kPi - detail::atanSmall(1.0 / x);
    }
    return detail::atanSmall(x);
}

inline double atan2(double y, double x)
{
    if (x > 0.0) {
        return atan(y / x);
    }
    if (x < 0.0) {
        return y >= 0.0 ? atan(y / x) + kPi : atan(y / x) - kPi;
    }
    return y > 0.0 ? 0.5 * kPi : (y < 0.0 ? -0.5 * kPi : 0.0);
}

// sqrt is exact in IEEE 754; the library's hypot need not be. The tests'
// lengths are far from overflow.
inline double hypot(double a, double b)
{
    return std::sqrt(a * a + b * b);
}

} // namespace PortableMath
