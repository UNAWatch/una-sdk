#include <gui/ringing_screen/RingingView.hpp>

static constexpr uint16_t kSnoozeTicks = SDK::Utils::secToTicks(120, App::Config::kFrameRate);
static constexpr uint16_t kPlayTicks = SDK::Utils::secToTicks(5, App::Config::kFrameRate);

RingingView::RingingView()
    : mSnoozeCb(this, &RingingView::onSnooze)
    , mPlayCb(this, &RingingView::onPlay)
{
    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);
}

void RingingView::setupScreen()
{
    RingingViewBase::setupScreen();

    mSnoozeTimer.setDuration(kSnoozeTicks);
    mSnoozeTimer.setCallback(mSnoozeCb);
    mSnoozeTimer.start();

    mPlayTimer.setDuration(kPlayTicks);
    mPlayTimer.setCallback(mPlayCb);
    mPlayTimer.start();
}

void RingingView::tearDownScreen()
{
    mSnoozeTimer.stop();
    mPlayTimer.stop();
    RingingViewBase::tearDownScreen();
}

void RingingView::setTime(uint8_t h, uint8_t m)
{
    Unicode::snprintf(timeValueBuffer, TIMEVALUE_SIZE, "%02u:%02u", h, m);
    timeValue.invalidate();
}

void RingingView::onSnooze()
{
    presenter->snooze(); // exit from screen
}

void RingingView::onPlay()
{
    presenter->play();
    mPlayTimer.start();
}

void RingingView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {

    }

    if (key == SDK::GUI::Button::L2) {

    }

    if (key == SDK::GUI::Button::R1) {
        presenter->stop();
    }

    if (key == SDK::GUI::Button::R2) {
        presenter->snooze();
    }
}
