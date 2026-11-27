#ifndef SUMMARYFACECHART_HPP
#define SUMMARYFACECHART_HPP

#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <gui/containers/Title.hpp>

#include <vector>

#include "ActivitySummary.hpp"

/**
 * @brief The summary's lap pace chart: one bar per lap of
 *        LapCompare::kMinChartM or more (the last kMaxBars), from the
 *        fastest (shortest bar, green) to the slowest (tallest, coral),
 *        with "Fastest lap n  4:12 /km" and "Slowest lap n  4:48 /km" under
 *        it. Written in code, not in the Designer; RunLVGL's is the same.
 */
class SummaryFaceChart : public touchgfx::Container
{
public:
    static constexpr uint8_t kMaxBars = 24;

    SummaryFaceChart();
    virtual ~SummaryFaceChart() {}

    void initialize();

    /** Fills the chart; hasChart() says whether two or more laps count. */
    void setLaps(const std::vector<LapSummary>& laps, bool isImperial);
    bool hasChart() const { return mHasChart; }

private:
    static constexpr uint16_t kLineSize = 40;

    Title                             mTitle;
    touchgfx::Box                     mBars[kMaxBars];
    touchgfx::TextAreaWithOneWildcard mFastLine;
    touchgfx::TextAreaWithOneWildcard mSlowLine;
    touchgfx::Unicode::UnicodeChar    mFastBuf[kLineSize] {};
    touchgfx::Unicode::UnicodeChar    mSlowBuf[kLineSize] {};
    bool                              mHasChart = false;
};

#endif // SUMMARYFACECHART_HPP
