#include <gui/containers/SummaryFaceCal.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <SDK/Utils/Utils.hpp>

SummaryFaceCal::SummaryFaceCal()
{
}

void SummaryFaceCal::initialize()
{
    SummaryFaceCalBase::initialize();
    title.set(T_TEXT_SUMMARY_UC);
}

void SummaryFaceCal::setTotalTime(std::time_t sec)
{
    auto hms = SDK::Utils::toHMS(sec);
    Unicode::snprintf(timerValueBuffer, TIMERVALUE_SIZE,
        "%u:%02u:%02u", hms.h, hms.m, hms.s);
    timerValue.invalidate();
}

void SummaryFaceCal::setTotalCalories(float kcal)
{
    Unicode::snprintfFloat(totalValueBuffer, TOTALVALUE_SIZE, "%.0f", kcal);
    totalValue.invalidate();
}

void SummaryFaceCal::setActiveCalories(float kcal)
{
    Unicode::snprintfFloat(activeValueBuffer, ACTIVEVALUE_SIZE, "%.0f", kcal);
    activeValue.invalidate();
}

void SummaryFaceCal::setRestingCalories(float kcal)
{
    Unicode::snprintfFloat(restingValueBuffer, RESTINGVALUE_SIZE, "%.0f", kcal);
    restingValue.invalidate();
}
