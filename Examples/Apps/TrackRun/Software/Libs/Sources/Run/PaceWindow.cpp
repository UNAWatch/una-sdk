/*******************************************************************************
 * @file   PaceWindow.cpp
 * @brief  Run: live speed from the total over a rolling window (PaceWindow.hpp).
 ******************************************************************************/

#include "Run/PaceWindow.hpp"

namespace Run {

namespace {

int32_t diff(uint32_t later, uint32_t earlier)
{
    return static_cast<int32_t>(later - earlier);
}

} // namespace

const PaceWindow::Entry& PaceWindow::at(uint8_t back) const noexcept
{
    return mE[(mHead + kSize - 1u - back) % kSize];
}

void PaceWindow::add(uint32_t tMs, float totalM) noexcept
{
    if (mCount > 0 && diff(tMs, at(0).tMs) <= 0) {
        return;
    }
    mE[mHead] = Entry{tMs, totalM};
    mHead     = static_cast<uint8_t>((mHead + 1u) % kSize);
    if (mCount < kSize) {
        ++mCount;
    }
}

void PaceWindow::rebase(float jumpM) noexcept
{
    if (!(jumpM > 0.0f) || mCount < 2) {
        return;
    }
    // Every entry but the newest (which already holds the jump).
    for (uint8_t b = 1; b < mCount; ++b) {
        mE[(mHead + kSize - 1u - b) % kSize].totalM += jumpM;
    }
}

float PaceWindow::speedOver(uint8_t newestBack) const noexcept
{
    if (mCount < newestBack + 2u) {
        return 0.0f;
    }
    const Entry& n = at(newestBack);
    // The oldest entry still inside the window.
    uint8_t o = newestBack;
    for (uint8_t b = static_cast<uint8_t>(newestBack + 1u); b < mCount; ++b) {
        if (diff(n.tMs, at(b).tMs) > static_cast<int32_t>(kWindowMs)) {
            break;
        }
        o = b;
    }
    const int32_t span = diff(n.tMs, at(o).tMs);
    if (span < static_cast<int32_t>(kMinSpanMs)) {
        return 0.0f;
    }
    const float d = n.totalM - at(o).totalM;
    return d > 0.0f ? d * 1000.0f / static_cast<float>(span) : 0.0f;
}

float PaceWindow::speedMps() const noexcept
{
    return speedOver(0);
}

float PaceWindow::speedBeforeNewestMps() const noexcept
{
    return speedOver(1);
}

bool PaceWindow::newest(uint32_t& tMs, float& totalM) const noexcept
{
    if (mCount == 0) {
        return false;
    }
    tMs    = at(0).tMs;
    totalM = at(0).totalM;
    return true;
}

bool PaceWindow::timeAt(float totalM, uint32_t& tMs) const noexcept
{
    if (mCount == 0 || totalM > at(0).totalM) {
        return false;
    }
    if (mCount == 1) {
        // One reading: its time, for its own total only.
        if (totalM == at(0).totalM) {
            tMs = at(0).tMs;
            return true;
        }
        return false;
    }
    for (uint8_t b = 0; b + 1u < mCount; ++b) {
        const Entry& hi = at(b);
        const Entry& lo = at(static_cast<uint8_t>(b + 1u));
        if (totalM >= lo.totalM && totalM <= hi.totalM) {
            const float span = hi.totalM - lo.totalM;
            const float f    = span > 0.0f ? (totalM - lo.totalM) / span : 1.0f;
            tMs = lo.tMs + static_cast<uint32_t>(static_cast<float>(diff(hi.tMs, lo.tMs)) * f + 0.5f);
            return true;
        }
    }
    return false;
}

bool PaceWindow::totalAt(uint32_t tMs, float& totalM) const noexcept
{
    if (mCount == 0) {
        return false;
    }
    if (diff(tMs, at(0).tMs) >= 0) {
        totalM = at(0).totalM;
        return true;
    }
    for (uint8_t b = 0; b + 1u < mCount; ++b) {
        const Entry& hi = at(b);
        const Entry& lo = at(static_cast<uint8_t>(b + 1u));
        if (diff(tMs, lo.tMs) >= 0) {
            const int32_t span = diff(hi.tMs, lo.tMs);
            const float   f    = span > 0 ? static_cast<float>(diff(tMs, lo.tMs)) / static_cast<float>(span) : 1.0f;
            totalM = lo.totalM + (hi.totalM - lo.totalM) * f;
            return true;
        }
    }
    return false;
}

float correctionJumpM(float riseM, uint32_t dtMs, float recentMps, bool forced) noexcept
{
    if (!(riseM > 0.0f)) {
        return 0.0f;
    }
    const float dt        = static_cast<float>(dtMs) * 0.001f;
    const float plausible = PaceWindow::kMaxRunMps * dt;
    if (!forced && riseM <= plausible) {
        return 0.0f;
    }
    const float explained = recentMps > 0.0f ? recentMps * dt * 1.25f + 2.0f : plausible;
    const float jump      = riseM - explained;
    return jump > 0.0f ? jump : 0.0f;
}

} // namespace Run
