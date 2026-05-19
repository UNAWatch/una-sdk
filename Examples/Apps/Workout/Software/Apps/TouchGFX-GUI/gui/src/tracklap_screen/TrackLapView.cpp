#include <gui/tracklap_screen/TrackLapView.hpp>
#include <texts/TextKeysAndLanguages.hpp>


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

void TrackLapView::setAvgLapHR(float hr)
{
    if (hr < App::Display::kMinHR) {
        Unicode::snprintf(hrValueBuffer, HRVALUE_SIZE, "---");
    } else {
        Unicode::snprintfFloat(hrValueBuffer, HRVALUE_SIZE, "%.0f", hr);
    }
    hrValue.invalidate();
}

void TrackLapView::setLapCalories(float calories)
{
    Unicode::snprintfFloat(caloriesValueBuffer, CALORIESVALUE_SIZE, "%.0f", calories);
    caloriesValue.invalidate();
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
