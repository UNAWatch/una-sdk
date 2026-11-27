#include <gui/containers/SummaryFaceChart.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <SDK/GUI/Color.hpp>

#include <cstdio>

#include "LapCompare.hpp"

namespace
{
// Bars from kBarMinH (the fastest lap) to kBarsH (the slowest), the others
// between by their pace; the two lines under them. As RunLVGL's.
constexpr int16_t kBarsX     = 37;
constexpr int16_t kBarsY     = 68;
constexpr int16_t kBarsW     = 166;
constexpr int16_t kBarsH     = 86;
constexpr int16_t kBarMinH   = 40;
constexpr int16_t kBarGap    = 3;
constexpr int16_t kFastLineY = 162;
constexpr int16_t kSlowLineY = 184;

/** Pace in s/m as "4:12" in the watch's unit, to the nearest second. */
void paceText(char* buf, size_t n, float secPerM, bool imperial)
{
    const float    perUnit = secPerM * (imperial ? 1609.344f : 1000.0f);
    const uint32_t s       = static_cast<uint32_t>(perUnit + 0.5f);
    std::snprintf(buf, n, "%u:%02u", static_cast<unsigned>(s / 60u), static_cast<unsigned>(s % 60u));
}
} // namespace

SummaryFaceChart::SummaryFaceChart()
{
}

void SummaryFaceChart::initialize()
{
    setWidth(240);
    setHeight(240);
    for (touchgfx::Box& b : mBars) {
        b.setVisible(false);
        add(b);
    }
    mFastLine.setPosition(25, kFastLineY, 190, 22);
    mFastLine.setTypedText(touchgfx::TypedText(T_TMP_REGULAR_16));
    mFastLine.setColor(touchgfx::colortype(SDK::GUI::Color::WHITE));
    mFastLine.setWildcard(mFastBuf);
    add(mFastLine);
    mSlowLine.setPosition(30, kSlowLineY, 180, 20);
    mSlowLine.setTypedText(touchgfx::TypedText(T_TMP_REGULAR_14));
    mSlowLine.setColor(touchgfx::colortype(SDK::GUI::Color::WHITE));
    mSlowLine.setWildcard(mSlowBuf);
    add(mSlowLine);
    mTitle.setXY(50, 0);
    add(mTitle);
    mTitle.initialize();
    mTitle.set("LAP PACE");
}

void SummaryFaceChart::setLaps(const std::vector<LapSummary>& laps, bool isImperial)
{
    // The last kMaxBars laps long enough to chart, by pace (LapCompare), each
    // with its number in the lap list.
    float    pace[kMaxBars]   = {};
    uint16_t number[kMaxBars] = {};
    const size_t n = laps.size();
    size_t charted = 0;
    for (size_t i = 0; i < n; ++i) {
        charted += laps[i].distance >= LapCompare::kMinChartM && laps[i].paceAvg > 0.0f ? 1u : 0u;
    }
    const size_t skip = charted > kMaxBars ? charted - kMaxBars : 0;
    size_t  seen  = 0;
    uint8_t count = 0;
    for (size_t i = 0; i < n && count < kMaxBars; ++i) {
        const LapSummary& l = laps[i];
        if (!(l.distance >= LapCompare::kMinChartM && l.paceAvg > 0.0f) || seen++ < skip) {
            continue;
        }
        number[count] = static_cast<uint16_t>(i + 1);
        pace[count++] = l.paceAvg;
    }
    const LapCompare::Extremes e = LapCompare::extremes(
        count, [&pace](size_t i) { return pace[i]; }, [](size_t) { return LapCompare::kMinChartM; });
    mHasChart = e.fastest >= 0;

    const int16_t pitch = static_cast<int16_t>(kBarsW / (count < 6 ? 6 : count));
    const int16_t w     = pitch > kBarGap + 1 ? static_cast<int16_t>(pitch - kBarGap) : 1;
    const int16_t x0    = static_cast<int16_t>(kBarsX + (kBarsW - pitch * count + kBarGap) / 2);
    const float   lo    = mHasChart ? pace[e.fastest] : 0.0f;
    const float   hi    = mHasChart ? pace[e.slowest] : 0.0f;
    for (uint8_t i = 0; i < kMaxBars; ++i) {
        touchgfx::Box& b = mBars[i];
        if (!mHasChart || i >= count) {
            b.setVisible(false);
            continue;
        }
        const int16_t h = hi > lo
            ? static_cast<int16_t>(kBarMinH + static_cast<int16_t>((pace[i] - lo) * (kBarsH - kBarMinH) / (hi - lo) + 0.5f))
            : kBarsH;
        const uint32_t color = i == e.fastest ? SDK::GUI::Color::LIME_SOFT
                             : i == e.slowest ? SDK::GUI::Color::SALMON : SDK::GUI::Color::TEAL;
        b.setPosition(static_cast<int16_t>(x0 + i * pitch), static_cast<int16_t>(kBarsY + kBarsH - h), w, h);
        b.setColor(touchgfx::colortype(color));
        b.setVisible(true);
    }

    char p[16];
    char line[kLineSize];
    const char* unit = isImperial ? "/mi" : "/km";
    mFastBuf[0] = 0;
    mSlowBuf[0] = 0;
    if (mHasChart) {
        paceText(p, sizeof(p), pace[e.fastest], isImperial);
        std::snprintf(line, sizeof(line), "Fastest lap %u  %s %s", static_cast<unsigned>(number[e.fastest]), p, unit);
        touchgfx::Unicode::strncpy(mFastBuf, line, kLineSize - 1);
        paceText(p, sizeof(p), pace[e.slowest], isImperial);
        std::snprintf(line, sizeof(line), "Slowest lap %u  %s %s", static_cast<unsigned>(number[e.slowest]), p, unit);
        touchgfx::Unicode::strncpy(mSlowBuf, line, kLineSize - 1);
    }
    invalidate();
}
