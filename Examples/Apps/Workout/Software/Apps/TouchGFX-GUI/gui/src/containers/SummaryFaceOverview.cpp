#include <gui/containers/SummaryFaceOverview.hpp>
#include <gui/model/Model.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <SDK/Utils/Utils.hpp>

SummaryFaceOverview::SummaryFaceOverview()
{
}

void SummaryFaceOverview::initialize()
{
    SummaryFaceOverviewBase::initialize();
    title.set(T_TEXT_SUMMARY_UC);
}

void SummaryFaceOverview::setTotalTime(std::time_t sec)
{
    auto hms = SDK::Utils::toHMS(sec);
    Unicode::snprintf(timerValueBuffer, TIMERVALUE_SIZE,
        "%u:%02u:%02u", hms.h, hms.m, hms.s);
    timerValue.invalidate();
}

void SummaryFaceOverview::setAvgHR(float hr)
{
    if (hr < App::Display::kMinHR) {
        Unicode::snprintf(avgHRValueBuffer, AVGHRVALUE_SIZE, "---");
    } else {
        Unicode::snprintfFloat(avgHRValueBuffer, AVGHRVALUE_SIZE, "%.0f", hr);
    }
    avgHRValue.invalidate();
}

void SummaryFaceOverview::setMaxHR(float hr)
{
    if (hr < App::Display::kMinHR) {
        Unicode::snprintf(maxHRValueBuffer, MAXHRVALUE_SIZE, "---");
    } else {
        Unicode::snprintfFloat(maxHRValueBuffer, MAXHRVALUE_SIZE, "%.0f", hr);
    }
    maxHRValue.invalidate();
}
