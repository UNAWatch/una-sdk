#include <gui/containers/SummaryFaceHeartRate.hpp>
#include <texts/TextKeysAndLanguages.hpp>

SummaryFaceHeartRate::SummaryFaceHeartRate()
{
}

void SummaryFaceHeartRate::initialize()
{
    SummaryFaceHeartRateBase::initialize();
    title.set(T_TEXT_HEART_RATE_UC);
}

void SummaryFaceHeartRate::setMaxHR(float hr)
{
    if (hr < App::Display::kMinHR) {
        Unicode::snprintf(maxHrValueBuffer, MAXHRVALUE_SIZE, "---");
    } else {
        Unicode::snprintfFloat(maxHrValueBuffer, MAXHRVALUE_SIZE, "%.0f", hr);
    }
    maxHrValue.invalidate();
}

void SummaryFaceHeartRate::setAvgHR(float hr)
{
    if (hr < App::Display::kMinHR) {
        Unicode::snprintf(avgHrValueBuffer, AVGHRVALUE_SIZE, "---");
    } else {
        Unicode::snprintfFloat(avgHrValueBuffer, AVGHRVALUE_SIZE, "%.0f", hr);
    }
    avgHrValue.invalidate();
}
