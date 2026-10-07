/**
 ******************************************************************************
 * @file    StepLabel.cpp
 * @brief   Short text for a workout step.
 ******************************************************************************
 */

#include "SDK/Workout/StepLabel.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace SDK::Workout {

namespace {

constexpr uint32_t kCmPerMile = 160934;  // 1609.344 m

// Appends to a buffer, keeping it NUL-terminated and cut on a character
// boundary when full.
class Text {
public:
    Text(char* buf, size_t cap) : mBuf(buf), mCap(cap)
    {
        if (mCap > 0) {
            mBuf[0] = '\0';
        }
    }

    void add(const char* s) { addBytes(s, std::strlen(s)); }

    void addf(const char* fmt, ...)
    {
        char    tmp[48];
        va_list args;
        va_start(args, fmt);
        const int n = std::vsnprintf(tmp, sizeof(tmp), fmt, args);
        va_end(args);
        if (n > 0) {
            addBytes(tmp, static_cast<size_t>(n) < sizeof(tmp) ? static_cast<size_t>(n) : sizeof(tmp) - 1);
        }
    }

    // A space before the next part, if there is one already.
    void separate()
    {
        if (mLen > 0) {
            add(" ");
        }
    }

    void capitalise()
    {
        if (mLen > 0 && mBuf[0] >= 'a' && mBuf[0] <= 'z') {
            mBuf[0] = static_cast<char>(mBuf[0] - 'a' + 'A');
        }
    }

    size_t length() const { return mLen; }

private:
    void addBytes(const char* s, size_t n)
    {
        if (mCap == 0) {
            return;
        }
        size_t room = mCap - 1 - mLen;
        if (n > room) {
            n = room;
            while (n > 0 && (static_cast<uint8_t>(s[n]) & 0xC0) == 0x80) {
                --n;  // don't stop inside a character
            }
        }
        std::memcpy(mBuf + mLen, s, n);
        mLen += n;
        mBuf[mLen] = '\0';
    }

    char*  mBuf;
    size_t mCap;
    size_t mLen = 0;
};

// @p hundredths as "1.25", "1.5" or "2".
void addHundredths(Text& t, uint32_t hundredths)
{
    const uint32_t whole = hundredths / 100u;
    const uint32_t frac  = hundredths % 100u;
    if (frac == 0) {
        t.addf("%lu", static_cast<unsigned long>(whole));
    } else if (frac % 10u == 0) {
        t.addf("%lu.%lu", static_cast<unsigned long>(whole), static_cast<unsigned long>(frac / 10u));
    } else {
        t.addf("%lu.%02lu", static_cast<unsigned long>(whole), static_cast<unsigned long>(frac));
    }
}

void addTime(Text& t, uint32_t ms)
{
    // Short steps, a rest or a stride, are counted in seconds.
    const uint32_t s = (ms + 500u) / 1000u;
    if (s < 120u) {
        t.addf("%lu s", static_cast<unsigned long>(s));
    } else if (s < 3600u && s % 60u == 0) {
        t.addf("%lu min", static_cast<unsigned long>(s / 60u));
    } else if (s < 3600u) {
        t.addf("%lu:%02lu", static_cast<unsigned long>(s / 60u), static_cast<unsigned long>(s % 60u));
    } else {
        t.addf("%lu:%02lu:%02lu", static_cast<unsigned long>(s / 3600u),
               static_cast<unsigned long>(s / 60u % 60u), static_cast<unsigned long>(s % 60u));
    }
}

void addDistance(Text& t, uint32_t cm, bool imperial)
{
    if (imperial) {
        const uint64_t hundredths = (static_cast<uint64_t>(cm) * 100u + kCmPerMile / 2u) / kCmPerMile;
        addHundredths(t, static_cast<uint32_t>(hundredths));
        t.add(" mi");
        return;
    }
    const uint32_t m = (cm + 50u) / 100u;
    if (m < 1000u) {
        t.addf("%lu m", static_cast<unsigned long>(m));
        return;
    }
    addHundredths(t, (cm + 500u) / 1000u);
    t.add(" km");
}

void addPace(Text& t, uint32_t seconds)
{
    t.addf("%lu:%02lu", static_cast<unsigned long>(seconds / 60u),
           static_cast<unsigned long>(seconds % 60u));
}

// "a-b", or "a" if they are the same.
void addRange(Text& t, uint32_t a, uint32_t b, const char* suffix)
{
    if (a == b) {
        t.addf("%lu%s", static_cast<unsigned long>(a), suffix);
    } else {
        t.addf("%lu-%lu%s", static_cast<unsigned long>(a), static_cast<unsigned long>(b), suffix);
    }
}

void durationInto(Text& t, const Step& s, bool imperial)
{
    switch (s.end) {
    case StepEnd::Time:     addTime(t, s.durationMs); break;
    case StepEnd::Distance: addDistance(t, s.distanceCm, imperial); break;
    case StepEnd::Repeat:
        if (s.repeatCount != kRepeatForever) {
            t.addf("x%lu", static_cast<unsigned long>(s.repeatCount));
        }
        break;
    default: break;
    }
}

void targetInto(Text& t, const Step& s, bool imperial)
{
    if (s.end == StepEnd::Repeat) {
        return;
    }
    switch (s.target) {
    case Target::Speed: {
        // Faster first: the higher speed is the lower pace.
        const uint32_t fast = paceSeconds(s.speedHighMmps, imperial);
        const uint32_t slow = paceSeconds(s.speedLowMmps, imperial);
        if (fast == 0 || slow == 0) {
            return;
        }
        addPace(t, fast < slow ? fast : slow);
        if (fast != slow) {
            t.add("-");
            addPace(t, fast < slow ? slow : fast);
        }
        t.add(imperial ? " /mi" : " /km");
        break;
    }
    case Target::HeartRate:
        switch (s.hrKind) {
        case HeartRateTarget::Zone:
            t.addf("zone %u", static_cast<unsigned>(s.hrZone));
            break;
        case HeartRateTarget::Bpm:
            addRange(t, s.hrLow, s.hrHigh, " bpm");
            break;
        case HeartRateTarget::PercentMax:
            addRange(t, s.hrLow, s.hrHigh, "% max");
            break;
        }
        break;
    default:
        break;
    }
}

}  // namespace

uint32_t paceSeconds(uint32_t speedMmps, bool imperial)
{
    if (speedMmps == 0) {
        return 0;
    }
    // A kilometre is 1e6 mm; a mile is 1609344 mm.
    const uint64_t mm = imperial ? 1609344u : 1000000u;
    return static_cast<uint32_t>((mm + speedMmps / 2u) / speedMmps);
}

size_t stepDuration(const Step& step, bool imperial, char* buf, size_t cap)
{
    Text t(buf, cap);
    durationInto(t, step, imperial);
    return t.length();
}

size_t stepTarget(const Step& step, bool imperial, char* buf, size_t cap)
{
    Text t(buf, cap);
    targetInto(t, step, imperial);
    return t.length();
}

size_t stepLabel(const Step& step, bool imperial, char* buf, size_t cap)
{
    Text t(buf, cap);
    if (step.notes[0] != '\0') {
        t.add(step.notes);
        return t.length();
    }
    durationInto(t, step, imperial);
    if (step.end != StepEnd::Repeat) {
        const char* kind = step.intensity == Intensity::Warmup   ? "warm up"
                         : step.intensity == Intensity::Cooldown ? "cool down"
                         : step.intensity == Intensity::Rest     ? "rest"
                                                                 : "";
        if (kind[0] != '\0') {
            t.separate();
            t.add(kind);
        }
        char target[32];
        if (stepTarget(step, imperial, target, sizeof(target)) > 0) {
            t.separate();
            t.add(target);
        }
        if (t.length() == 0) {
            t.add("open");
        }
    }
    t.capitalise();
    return t.length();
}

}  // namespace SDK::Workout
