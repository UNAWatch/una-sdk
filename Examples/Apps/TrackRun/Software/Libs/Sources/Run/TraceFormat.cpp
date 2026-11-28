/*******************************************************************************
 * @file   TraceFormat.cpp
 * @brief  Run: the lines of the GPS trace CSV (TraceFormat.hpp).
 ******************************************************************************/

#include "Run/TraceFormat.hpp"

#include <cmath>
#include <cstring>

namespace Run {
namespace TraceFormat {

namespace {

/** Appends text to buf at len; false if it does not fit (leaving room for
 *  nothing else). */
class Out {
public:
    Out(char* buf, size_t cap) noexcept : mBuf(buf), mCap(cap) {}

    void str(const char* s) noexcept
    {
        if (s == nullptr) {
            return;
        }
        const size_t n = std::strlen(s);
        if (!mOk || mLen + n > mCap) {
            mOk = false;
            return;
        }
        std::memcpy(mBuf + mLen, s, n);
        mLen += n;
    }
    void ch(char c) noexcept
    {
        if (!mOk || mLen + 1u > mCap) {
            mOk = false;
            return;
        }
        mBuf[mLen++] = c;
    }
    void u32(uint32_t v) noexcept
    {
        char tmp[12];
        int  n = 0;
        do {
            tmp[n++] = static_cast<char>('0' + v % 10u);
            v /= 10u;
        } while (v != 0u && n < 11);
        while (n > 0) {
            ch(tmp[--n]);
        }
    }
    void i32(int32_t v) noexcept
    {
        if (v < 0) {
            ch('-');
            u32(static_cast<uint32_t>(-(static_cast<int64_t>(v))));
        } else {
            u32(static_cast<uint32_t>(v));
        }
    }
    void num(double v, int decimals) noexcept
    {
        if (!mOk) {
            return;
        }
        const size_t n = fixed(mBuf + mLen, mCap - mLen, v, decimals);
        if (n == 0 && std::isfinite(v)) {
            mOk = false;
            return;
        }
        mLen += n;
    }
    size_t done() const noexcept { return mOk ? mLen : 0u; }

private:
    char*  mBuf;
    size_t mCap;
    size_t mLen = 0;
    bool   mOk  = true;
};

} // namespace

size_t fixed(char* buf, size_t cap, double v, int decimals) noexcept
{
    if (!std::isfinite(v) || decimals < 0 || decimals > 9) {
        return 0;
    }
    uint64_t scale = 1;
    for (int i = 0; i < decimals; ++i) {
        scale *= 10u;
    }
    const bool   neg = v < 0.0;
    const double a   = std::floor(std::fabs(v) * static_cast<double>(scale) + 0.5);
    if (a >= 9.0e18) {
        return 0;
    }
    const uint64_t q     = static_cast<uint64_t>(a);
    uint64_t       whole = q / scale;
    uint64_t       frac  = q % scale;

    char tmp[32];
    int  n = 0;
    for (int i = 0; i < decimals; ++i) {
        tmp[n++] = static_cast<char>('0' + frac % 10u);
        frac /= 10u;
    }
    if (decimals > 0) {
        tmp[n++] = '.';
    }
    do {
        tmp[n++] = static_cast<char>('0' + whole % 10u);
        whole /= 10u;
    } while (whole != 0u && n < 30);
    if (neg && q != 0u) {
        tmp[n++] = '-';
    }
    if (static_cast<size_t>(n) > cap) {
        return 0;
    }
    for (int i = 0; i < n; ++i) {
        buf[i] = tmp[n - 1 - i];
    }
    return static_cast<size_t>(n);
}

size_t versionLine(char* buf, size_t cap) noexcept
{
    Out o(buf, cap);
    o.str("# trackrun-gps-trace v");
    o.u32(static_cast<uint32_t>(kVersion));
    o.ch('\n');
    return o.done();
}

size_t metaLine(char* buf, size_t cap, const char* key, const char* value) noexcept
{
    Out o(buf, cap);
    o.str("# ");
    o.str(key);
    o.ch('=');
    o.str(value);
    o.ch('\n');
    return o.done();
}

size_t metaLine(char* buf, size_t cap, const char* key, uint32_t value) noexcept
{
    Out o(buf, cap);
    o.str("# ");
    o.str(key);
    o.ch('=');
    o.u32(value);
    o.ch('\n');
    return o.done();
}

size_t columnsLine(char* buf, size_t cap) noexcept
{
    Out o(buf, cap);
    o.str("t_ms,fix,lat,lon,alt_m,hdop,gps_dist_m,track_dist_m,lap,mode,lane,segment\n");
    return o.done();
}

size_t rowLine(char* buf, size_t cap, const Row& r) noexcept
{
    Out o(buf, cap);
    o.u32(r.tMs);
    o.ch(',');
    o.ch(r.fix ? '1' : '0');
    o.ch(',');
    o.num(r.lat, 7);
    o.ch(',');
    o.num(r.lon, 7);
    o.ch(',');
    o.num(r.altM, 1);
    o.ch(',');
    if (r.hdop >= 0.0f) {
        o.num(r.hdop, 2);
    }
    o.ch(',');
    o.num(r.gpsDistM, 2);
    o.ch(',');
    o.num(r.trackDistM, 2);
    o.ch(',');
    if (r.lap >= 0) {
        o.i32(r.lap);
    }
    o.ch(',');
    o.str(r.mode);
    o.ch(',');
    if (r.lane > 0) {
        o.u32(r.lane);
    }
    o.ch(',');
    o.str(r.segment);
    o.ch('\n');
    return o.done();
}

size_t actionLine(char* buf, size_t cap, const char* key, uint32_t tMs, int32_t value) noexcept
{
    Out o(buf, cap);
    o.str("# ");
    o.str(key);
    o.ch('=');
    o.u32(tMs);
    if (value >= 0) {
        o.ch(',');
        o.i32(value);
    }
    o.ch('\n');
    return o.done();
}

} // namespace TraceFormat
} // namespace Run
