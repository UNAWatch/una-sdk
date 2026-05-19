#include <gui/trackintervalscountdown_screen/TrackIntervalsCountdownView.hpp>

TrackIntervalsCountdownView::TrackIntervalsCountdownView() :
    mTimerValueChangedCb(this, &TrackIntervalsCountdownView::onTimerValueChanged),
    mTimerCompleteCb(this, &TrackIntervalsCountdownView::onTimerComplete)
{

}

void TrackIntervalsCountdownView::setupScreen()
{
    TrackIntervalsCountdownViewBase::setupScreen();

    // Default button states
    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);

    updateTimerText(kTimeoutSec);
    
    timerRing.setFPS(App::Config::kFrameRate);
    timerRing.setMode(TimerRing::DRAIN);
    timerRing.setMaxValue(kTimeoutSec * 1000);
    timerRing.setSpeed(1000);
    timerRing.setValueChangedCallback(mTimerValueChangedCb);
    timerRing.setCompleteCallback(mTimerCompleteCb);
    timerRing.setValue(kTimeoutSec * 1000); // Start from 5s
    timerRing.animateTo(0);
}

void TrackIntervalsCountdownView::tearDownScreen()
{
    TrackIntervalsCountdownViewBase::tearDownScreen();
}

void TrackIntervalsCountdownView::setIntervals(const Settings::Intervals& inervals, bool isImperial)
{
    // Repeats
    if (inervals.repeatsNum == 0) {
        Unicode::snprintf(repsTextBuffer, REPSTEXT_SIZE, "%s: %s", 
            touchgfx::TypedText(T_TEXT_REPS).getText(), touchgfx::TypedText(T_TEXT_OPEN).getText());
    } else {
        Unicode::snprintf(repsTextBuffer, REPSTEXT_SIZE, "%s: x%u", 
            touchgfx::TypedText(T_TEXT_REPS).getText(), inervals.repeatsNum);
    }

    // Run
    if (inervals.runMetric == Settings::Intervals::OPEN) {
        Unicode::snprintf(runTextBuffer, RUNTEXT_SIZE, "%s: %s", 
            touchgfx::TypedText(T_TEXT_RUN).getText(), touchgfx::TypedText(T_TEXT_OPEN).getText());
    } else if (inervals.runMetric == Settings::Intervals::DISTANCE) {
        if (inervals.runDistance < 0.001f) {
            Unicode::snprintf(runTextBuffer, RUNTEXT_SIZE, "%s: %s",
                touchgfx::TypedText(T_TEXT_RUN).getText(), touchgfx::TypedText(T_TEXT_OPEN).getText());
        } else {
            Unicode::snprintf(runTextBuffer, RUNTEXT_SIZE, "%s: ",
                touchgfx::TypedText(T_TEXT_RUN).getText());
            uint16_t len = Unicode::strlen(runTextBuffer);

            float v = inervals.runDistance / 1000.0f; // km
            if (isImperial) {
                v = SDK::Utils::kmToMiles(v); // mi
            }
            Unicode::snprintfFloat(&runTextBuffer[len], RUNTEXT_SIZE - len, "%02.02f", v);
        }
    } else {
        if (inervals.runTime == 0) {
            Unicode::snprintf(runTextBuffer, RUNTEXT_SIZE, "%s: %s",
                touchgfx::TypedText(T_TEXT_RUN).getText(), touchgfx::TypedText(T_TEXT_OPEN).getText());
        } else {
            auto hms = SDK::Utils::toHMS(inervals.runTime);
            Unicode::snprintf(runTextBuffer, RUNTEXT_SIZE, "%s: %02u:%02u",
                touchgfx::TypedText(T_TEXT_RUN).getText(), hms.m + (hms.h * 60), hms.s);
        }
    }

    // Rest
    if (inervals.restMetric == Settings::Intervals::OPEN) {
        Unicode::snprintf(restTextBuffer, RESTTEXT_SIZE, "%s: %s",
            touchgfx::TypedText(T_TEXT_REST).getText(), touchgfx::TypedText(T_TEXT_OPEN).getText());
    } else if (inervals.restMetric == Settings::Intervals::DISTANCE) {
        if (inervals.restDistance < 0.001f) {
            Unicode::snprintf(restTextBuffer, RESTTEXT_SIZE, "%s: %s",
                touchgfx::TypedText(T_TEXT_REST).getText(), touchgfx::TypedText(T_TEXT_OPEN).getText());
        } else {
            Unicode::snprintf(restTextBuffer, RESTTEXT_SIZE, "%s: ",
                touchgfx::TypedText(T_TEXT_REST).getText());
            uint16_t len = Unicode::strlen(restTextBuffer);

            float v = inervals.restDistance / 1000.0f; // km
            if (isImperial) {
                v = SDK::Utils::kmToMiles(v); // mi
            }
            Unicode::snprintfFloat(&restTextBuffer[len], RESTTEXT_SIZE - len, "%02.02f", v);
        }
    } else {
        if (inervals.restTime == 0) {
            Unicode::snprintf(restTextBuffer, RESTTEXT_SIZE, "%s: %s",
                touchgfx::TypedText(T_TEXT_REST).getText(), touchgfx::TypedText(T_TEXT_OPEN).getText());
        } else {
            auto hms = SDK::Utils::toHMS(inervals.restTime);
            Unicode::snprintf(restTextBuffer, RESTTEXT_SIZE, "%s: %02u:%02u",
                touchgfx::TypedText(T_TEXT_REST).getText(), hms.m + (hms.h * 60), hms.s);
        }
    }

    repsText.invalidate();
    runText.invalidate();
    restText.invalidate();
}

void TrackIntervalsCountdownView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
    }

    if (key == SDK::GUI::Button::L2) {
    }

    if (key == SDK::GUI::Button::R1) {
        onConfirm();
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoMenuIntervalsScreenNoTransition();
    }
}

void TrackIntervalsCountdownView::onConfirm()
{
    presenter->startTrack();
}

void TrackIntervalsCountdownView::updateTimerText(uint32_t seconds)
{
    // Text
    Unicode::snprintf(timeValueBuffer, TIMEVALUE_SIZE, "%u", seconds);
    timeValue.invalidate();
}

void TrackIntervalsCountdownView::onTimerValueChanged(int32_t value)
{
    uint32_t remainingSeconds = value / 1000;

    if (mLastDisplayedSecond != remainingSeconds) {
        mLastDisplayedSecond = remainingSeconds;
        updateTimerText(remainingSeconds);
    }
}

void TrackIntervalsCountdownView::onTimerComplete(int32_t value)
{
    onConfirm();
}