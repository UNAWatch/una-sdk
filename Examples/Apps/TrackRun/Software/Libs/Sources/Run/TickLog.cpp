/*******************************************************************************
 * @file   TickLog.cpp
 * @brief  Run: per-second stats for splits that settle late (TickLog.hpp).
 ******************************************************************************/

#include "Run/TickLog.hpp"

#include <cmath>
#include <cstring>

namespace Run {

namespace {

int32_t diff(uint32_t later, uint32_t earlier)
{
    return static_cast<int32_t>(later - earlier);
}

uint16_t toDm(float m)
{
    const double dm = std::floor(static_cast<double>(m) * 10.0 + 0.5);
    return static_cast<uint16_t>(static_cast<int64_t>(dm) & 0xFFFF);
}

float dmSpan(uint16_t from, uint16_t to)
{
    return 0.1f * static_cast<float>(static_cast<uint16_t>(to - from));
}

constexpr uint32_t kTimeMask = 0xFFFFFFu; // the 24 bits of time an entry keeps

uint32_t low24(uint16_t lo, uint8_t hi)
{
    return static_cast<uint32_t>(lo) | (static_cast<uint32_t>(hi) << 16);
}

} // namespace

uint32_t TickLog::timeOf(const Entry& e) const noexcept
{
    const Entry& newest = back(0);
    return mNewestT - ((low24(newest.tLo, newest.tHi) - low24(e.tLo, e.tHi)) & kTimeMask);
}

float TickLog::altOf(const Entry& e) noexcept
{
    float m;
    static_assert(sizeof(m) == sizeof(e.alt), "a float is 32 bits");
    std::memcpy(&m, e.alt, sizeof(m));
    return m;
}

void TickLog::setAlt(Entry& e, float altM) noexcept
{
    std::memcpy(e.alt, &altM, sizeof(e.alt));
}

void TickLog::add(uint32_t tMs, float hrBpm, float speedMps, float altM, float ascentM, float descentM) noexcept
{
    if (mCount > 0 && diff(tMs, mNewestT) <= 0) {
        return;
    }
    if (mCount == kSize) {
        mLeftT      = timeOf(mE[mHead]);
        mLeftAscDm  = mE[mHead].ascDm;
        mLeftDescDm = mE[mHead].descDm;
        mLeftAny    = true;
    }
    if (mCount == kSize && mCarry.set) {
        // The oldest tick leaves the log: kept in sum if the oldest split
        // held has it.
        const Entry& old = mE[mHead];
        if (diff(timeOf(old), mCarry.fromMs) >= 0) {
            if (!mCarry.base) {
                // No tick before the split was kept: its climb counts from its first.
                mCarry.ascDm  = old.ascDm;
                mCarry.descDm = old.descDm;
                mCarry.base   = true;
            }
            ++mCarry.ticks;
            if (old.hr > 0) {
                mCarry.hrSum += old.hr;
                ++mCarry.hrN;
                mCarry.hrMax = old.hr > mCarry.hrMax ? old.hr : mCarry.hrMax;
            }
            mCarry.speedCms = old.speedCms > mCarry.speedCms ? old.speedCms : mCarry.speedCms;
        }
    }
    Entry& e   = mE[mHead];
    e.tLo      = static_cast<uint16_t>(tMs & 0xFFFFu);
    e.tHi      = static_cast<uint8_t>((tMs >> 16) & 0xFFu);
    setAlt(e, altM);
    const float cms = speedMps > 0.0f ? speedMps * 100.0f + 0.5f : 0.0f;
    e.speedCms = static_cast<uint16_t>(cms < 65535.0f ? cms : 65535.0f);
    const float hr = hrBpm > 0.0f ? hrBpm + 0.5f : 0.0f;
    e.hr       = static_cast<uint8_t>(hr < 255.0f ? hr : 255.0f);
    e.ascDm    = toDm(ascentM);
    e.descDm   = toDm(descentM);
    mNewestT   = tMs;
    mHead      = static_cast<uint16_t>((mHead + 1u) % kSize);
    if (mCount < kSize) {
        ++mCount;
    }
}

void TickLog::anchor(uint32_t fromMs) noexcept
{
    mCarry        = Carry();
    mCarry.fromMs = fromMs;
    mCarry.set    = true;
    // Ticks from fromMs on that have already left cannot be carried: a
    // split from here is then partial, however the carry goes on.
    mCarry.lost   = mLeftAny && diff(mLeftT, fromMs) >= 0;
    // The ascent and descent before it: the last tick before fromMs.
    for (uint16_t b = 0; b < mCount; ++b) {
        if (diff(timeOf(back(b)), fromMs) < 0) {
            mCarry.ascDm  = back(b).ascDm;
            mCarry.descDm = back(b).descDm;
            mCarry.base   = true;
            break;
        }
    }
}

TickLog::Stats TickLog::stats(uint32_t startMs, uint32_t endMs) const noexcept
{
    Stats st;
    if (mCount == 0) {
        return st;
    }
    const uint32_t oldest = timeOf(back(static_cast<uint16_t>(mCount - 1u)));
    st.partial = diff(oldest, startMs) > 0;
    // A split from the anchor has the ticks that left the log in the carry,
    // which holds every tick from the anchor on that has left. They are all
    // inside the split only while the oldest tick kept is not after its end:
    // otherwise the carry has later splits' ticks too, and the split stays
    // partial.
    const bool carried = st.partial && mCarry.set && !mCarry.lost && mCarry.fromMs == startMs &&
                         diff(oldest, endMs) <= 0;
    uint32_t hrSum = 0;
    uint32_t hrN   = 0;
    int32_t  first = -1; // oldest tick inside
    int32_t  last  = -1; // newest tick inside
    for (uint16_t b = 0; b < mCount; ++b) {
        const Entry&   e = back(b);
        const uint32_t t = timeOf(e);
        if (diff(t, endMs) >= 0) {
            continue;
        }
        if (diff(t, startMs) < 0) {
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
    if (carried) {
        st.ticks += mCarry.ticks;
        hrSum += mCarry.hrSum;
        hrN += mCarry.hrN;
        st.hrMax    = mCarry.hrMax > st.hrMax ? mCarry.hrMax : st.hrMax;
        const float v = 0.01f * static_cast<float>(mCarry.speedCms);
        st.speedMax = v > st.speedMax ? v : st.speedMax;
        st.partial  = false;
    }
    if (hrN > 0) {
        st.hrAvg = static_cast<float>(hrSum) / static_cast<float>(hrN);
    }
    if (first >= 0) {
        // From the tick before the split (if kept) to its last tick.
        const uint16_t fromB = static_cast<uint16_t>(first + 1 < mCount ? first + 1 : first);
        const Entry&   a     = back(fromB);
        const Entry&   z     = back(static_cast<uint16_t>(last));
        const bool     base  = carried && mCarry.base;
        st.ascentM  = dmSpan(base ? mCarry.ascDm : a.ascDm, z.ascDm);
        st.descentM = dmSpan(base ? mCarry.descDm : a.descDm, z.descDm);
    } else if (carried && mCarry.base && mLeftAny) {
        // Every tick of the split has left (in the carry): the climb from the
        // carry's base to the last tick that left, the split's last (the
        // oldest kept is at or after its end, not in it).
        st.ascentM  = dmSpan(mCarry.ascDm, mLeftAscDm);
        st.descentM = dmSpan(mCarry.descDm, mLeftDescDm);
    }
    return st;
}

bool TickLog::since(uint32_t tMs, float& ascentM, float& descentM, float& altM) const noexcept
{
    if (mCount == 0) {
        return false;
    }
    // The last tick at or before tMs, else the oldest kept.
    uint16_t pick = static_cast<uint16_t>(mCount - 1u);
    for (uint16_t b = 0; b < mCount; ++b) {
        if (diff(timeOf(back(b)), tMs) <= 0) {
            pick = b;
            break;
        }
    }
    const Entry& e = back(pick);
    ascentM  = dmSpan(e.ascDm, back(0).ascDm);
    descentM = dmSpan(e.descDm, back(0).descDm);
    altM     = altOf(e);
    return true;
}

void TickLog::flattenFrom(uint32_t tMs, float altM) noexcept
{
    // The entry at or before tMs (else the oldest) sets the level; later
    // ones take it.
    uint16_t asc = 0, desc = 0;
    bool     have = false;
    for (uint16_t b = 0; b < mCount; ++b) {
        if (diff(timeOf(back(b)), tMs) <= 0) {
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
    if (have && mCarry.set && mCarry.base && diff(mCarry.fromMs, tMs) > 0) {
        // The level before the anchor is after tMs too.
        mCarry.ascDm  = asc;
        mCarry.descDm = desc;
    }
    for (uint16_t b = 0; b < mCount; ++b) {
        Entry& e = back(b);
        if (diff(timeOf(e), tMs) <= 0) {
            break;
        }
        if (have) {
            e.ascDm  = asc;
            e.descDm = desc;
        }
        setAlt(e, altM);
    }
}

} // namespace Run
