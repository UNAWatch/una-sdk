#include <gui/containers/TrackFaceLap.hpp>

TrackFaceLap::TrackFaceLap()
{
}

void TrackFaceLap::initialize()
{
    TrackFaceLapBase::initialize();
}

void TrackFaceLap::setLapAvgHR(float hr)
{
    if (hr < App::Display::kMinHR) {
        Unicode::snprintf(hrValueBuffer, HRVALUE_SIZE, "---");
    } else {
        Unicode::snprintfFloat(hrValueBuffer, HRVALUE_SIZE, "%.0f", hr);
    }
    hrValue.invalidate();
}

void TrackFaceLap::setLapNumber(uint32_t lapNum)
{
    Unicode::snprintf(lapValueBuffer, LAPVALUE_SIZE, "%u", static_cast<unsigned>(lapNum + 1));
    lapValue.invalidate();
}

void TrackFaceLap::setTimer(std::time_t sec)
{
    auto hms = SDK::Utils::toHMS(sec);
    Unicode::snprintf(timerValueBuffer, TIMERVALUE_SIZE,
        "%u:%02u:%02u", hms.h, hms.m, hms.s);
    timerValue.invalidate();
}
