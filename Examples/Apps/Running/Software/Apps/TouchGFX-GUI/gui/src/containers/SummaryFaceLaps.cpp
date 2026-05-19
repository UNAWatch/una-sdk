#include <gui/containers/SummaryFaceLaps.hpp>
#include <texts/TextKeysAndLanguages.hpp>

static constexpr uint32_t skListAnimationSteps = App::Config::kMenuAnimationSteps / 2;

SummaryFaceLaps::SummaryFaceLaps()
{
}

void SummaryFaceLaps::initialize()
{
    SummaryFaceLapsBase::initialize();
    title.set(T_TEXT_LAPS_UC);
}

void SummaryFaceLaps::setLaps(const std::vector<LapSummary>& laps, bool isImperial)
{
    mLaps       = &laps;
    mIsImperial = isImperial;

    Unicode::snprintf(lapsTextBuffer, LAPSTEXT_SIZE, "%u %s",
        static_cast<uint32_t>(laps.size()),
        touchgfx::TypedText(T_TEXT_LAPS_UC).getText());
    lapsText.invalidate();

    time_t total = 0;
    for (const auto& lap : laps) {
        total += lap.duration;
    }
    auto hms = SDK::Utils::toHMS(total);
    Unicode::snprintf(timerTextBuffer, TIMERTEXT_SIZE, "%s: %u:%02u:%02u",
        touchgfx::TypedText(T_TEXT_TOTAL).getText(), hms.h, hms.m, hms.s);
    timerText.invalidate();

    mCurrentPage = 0;
    mNumPages = (laps.empty())
        ? 0u
        : static_cast<uint8_t>((laps.size() + kPageSize - 1) / kPageSize);

    // Pad to (numPages-1)*kPageSize + kVisible so every page transition
    // selects an item outside the current window, forcing a real scroll.
    const int16_t paddedSize = (mNumPages > 0)
        ? static_cast<int16_t>((mNumPages - 1) * kPageSize + kVisible)
        : 0;
    scrollList.setNumberOfItems(paddedSize);
    scrollList.animateToItem(0, 0);
}

bool SummaryFaceLaps::canScrollDown() const
{
    return mCurrentPage + 1 < mNumPages;
}

bool SummaryFaceLaps::canScrollUp() const
{
    return mCurrentPage > 0;
}

void SummaryFaceLaps::scrollDown()
{
    if (canScrollDown()) {
        ++mCurrentPage;
        // Select last item of new window -- always outside previous window.
        scrollList.animateToItem(
            static_cast<int16_t>(mCurrentPage * kPageSize + kVisible - 1), skListAnimationSteps);
    }
}

void SummaryFaceLaps::scrollUp()
{
    if (canScrollUp()) {
        --mCurrentPage;
        // Select first item of new window -- always outside previous window.
        scrollList.animateToItem(
            static_cast<int16_t>(mCurrentPage * kPageSize), skListAnimationSteps);
    }
}

void SummaryFaceLaps::scrollListUpdateItem(LapListItem& item, int16_t itemIndex)
{
    static const touchgfx::Unicode::UnicodeChar kEmpty[1] = {0};

    if (mLaps == nullptr || itemIndex < 0 || itemIndex >= static_cast<int16_t>(mLaps->size())) {
        item.setIndex(kEmpty);
        item.setDistance(kEmpty);
        item.setUnits(kEmpty);
        item.setTime(kEmpty);
        item.setPace(kEmpty);
        return;
    }

    const LapSummary& lap = (*mLaps)[itemIndex];

    const float km   = lap.distance / 1000.0f;
    const float dist = mIsImperial ? SDK::Utils::kmToMiles(km) : km;

    const float   secPerKm = (lap.paceAvg > 1e-6f) ? lap.paceAvg * 1000.0f : 0.0f;
    const time_t  pace     = (secPerKm > 0.0f)
        ? static_cast<time_t>(mIsImperial ? secPerKm / SDK::Utils::kmToMiles(1.0f) : secPerKm)
        : 0;

    touchgfx::Unicode::UnicodeChar buf[32];

    Unicode::snprintf(buf, 32, "%u.", static_cast<uint32_t>(itemIndex + 1));
    item.setIndex(buf);

    if (dist < 100.0f) {
        Unicode::snprintfFloat(buf, 32, "%.2f", dist);
    } else {
        Unicode::snprintfFloat(buf, 32, "%.1f", dist);
    }
    item.setDistance(buf);

    item.setUnits(touchgfx::TypedText(mIsImperial ? T_TEXT_MI : T_TEXT_KM).getText());

    auto dur = SDK::Utils::toHMS(lap.duration);
    if (dur.h > 0) {
        Unicode::snprintf(buf, 32, "%u:%02u", dur.h, dur.m);
    } else {
        Unicode::snprintf(buf, 32, "%u:%02u", dur.m, dur.s);
    }
    item.setTime(buf);

    if (pace < static_cast<time_t>(App::Display::kMinPace)) {
        Unicode::snprintf(buf, 32, "---");
    } else {
        auto pac = SDK::Utils::toHMS(pace);
        if (pac.h > 0) {
            Unicode::snprintf(buf, 32, "%u:%02u", pac.h, pac.m);
        } else {
            Unicode::snprintf(buf, 32, "%u:%02u", pac.m, pac.s);
        }
    }
    item.setPace(buf);
}
