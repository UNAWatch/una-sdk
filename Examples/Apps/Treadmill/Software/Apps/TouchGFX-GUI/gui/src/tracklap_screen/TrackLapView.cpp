#include <gui/tracklap_screen/TrackLapView.hpp>


static constexpr uint16_t kDismissTicks = SDK::Utils::secToTicks(5, App::Config::kFrameRate);

TrackLapView::TrackLapView()
    : mDismissCb(this, &TrackLapView::onDismiss)
{
}

void TrackLapView::setupScreen()
{
    TrackLapViewBase::setupScreen();

    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::NONE);
    buttons.setR2(Buttons::NONE);

    mDismissTimer.setDuration(kDismissTicks);
    mDismissTimer.setCallback(mDismissCb);
    mDismissTimer.start();
}

void TrackLapView::tearDownScreen()
{
    mDismissTimer.stop();
    TrackLapViewBase::tearDownScreen();
}

void TrackLapView::setUnitsImperial(bool isImperial)
{
    mIsImperial = isImperial;
}

void TrackLapView::setLapNum(uint32_t n)
{
    const uint16_t kBuffSize = 16;
    touchgfx::Unicode::UnicodeChar buffer[kBuffSize] { };

    Unicode::snprintf(buffer, kBuffSize, "%s %u",
        touchgfx::TypedText(T_TEXT_LAP).getText(), n + 1);

    title.set(buffer);
}

void TrackLapView::setPace(float secPerM)
{
    auto paceConv = [this](float s) -> float {
        if (s < 1e-6f) return 0.0f;
        const float secPerKm = s * 1000.0f;
        return mIsImperial ? secPerKm / SDK::Utils::kmToMiles(1.0f) : secPerKm;
    };

    float value = paceConv(secPerM);

    if (value < App::Display::kMinPace) {
        Unicode::snprintf(paceValueBuffer, PACEVALUE_SIZE, "---");
    } else {
        auto hms = SDK::Utils::toHMS(static_cast<std::time_t>(value));
        if (hms.h > 0) {
            Unicode::snprintf(paceValueBuffer, PACEVALUE_SIZE, "%u:%02u", hms.h, hms.m);
        } else {
            Unicode::snprintf(paceValueBuffer, PACEVALUE_SIZE, "%u:%02u", hms.m, hms.s);
        }
    }
    paceValue.invalidate();
}

void TrackLapView::setDistance(float metres)
{
    auto distConv = [this](float m) -> float {
        const float km = m / 1000.0f;
        return mIsImperial ? SDK::Utils::kmToMiles(km) : km;
    };

    float value = distConv(metres);

    if (value < App::Display::kMinDist) {
        Unicode::snprintf(distanceValueBuffer, DISTANCEVALUE_SIZE, "---");
    } else if (value < 10.0f) {
        Unicode::snprintfFloat(distanceValueBuffer, DISTANCEVALUE_SIZE, "%.02f", value);
    } else if (value < 100.0f) {
        Unicode::snprintfFloat(distanceValueBuffer, DISTANCEVALUE_SIZE, "%.01f", value);
    } else {
        Unicode::snprintfFloat(distanceValueBuffer, DISTANCEVALUE_SIZE, "%.0f", value);
    }

    distanceValue.invalidate();
}

void TrackLapView::setTimer(std::time_t sec)
{
    auto hms = SDK::Utils::toHMS(sec);
    if (hms.h == 0) {
        Unicode::snprintf(timeValueBuffer, TIMEVALUE_SIZE, "%u:%02u", hms.m, hms.s);
    } else {
        Unicode::snprintf(timeValueBuffer, TIMEVALUE_SIZE, "%u:%02u", hms.h, hms.m);
    }
    timeValue.invalidate();
}

void TrackLapView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L2) {
        // Disabled by design
        //mDismissTimer.stop();
        //application().gotoTrackScreenNoTransition();
    }
}

void TrackLapView::onDismiss()
{
    application().gotoTrackScreenNoTransition();
}
