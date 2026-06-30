#include <gui/trackholdconfirmation_screen/TrackHoldConfirmationView.hpp>
#include <touchgfx/Color.hpp>

namespace {
// Arc colours: green for Finish, red for Discard (match the tick glyphs + Buttons palette).
const touchgfx::colortype kFinishColor  = touchgfx::Color::getColorFromRGB(64, 192, 0);
const touchgfx::colortype kDiscardColor = touchgfx::Color::getColorFromRGB(192, 0, 0);
}

TrackHoldConfirmationView::TrackHoldConfirmationView() :
    mTimerValueChangedCb(this, &TrackHoldConfirmationView::onTimerValueChanged),
    mTimerCompleteCb(this, &TrackHoldConfirmationView::onTimerComplete)
{
}

void TrackHoldConfirmationView::setupScreen()
{
    TrackHoldConfirmationViewBase::setupScreen();

    mMode = presenter->getHoldConfirmMode();
    const bool finish = (mMode == Model::HoldConfirmMode::Finish);

    // Label ("Hold to Finish" / "Hold to Discard").
    questionText.setTypedText(touchgfx::TypedText(finish ? T_TEXT_HOLD_TO_FINISH
                                                         : T_TEXT_HOLD_TO_DISCARD));
    questionText.invalidate();

    // R1 = confirm (green finish / red discard). No R2/back hint: releasing R1 cancels.
    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(finish ? Buttons::GREEN : Buttons::RED);
    buttons.setR2(Buttons::NONE);

    // Confirm tick glyph matches the action colour.
    tick.setBitmap(touchgfx::Bitmap(finish ? BITMAP_TICKGREEN_22X17_ID
                                           : BITMAP_TICKRED_22X17_ID));
    tick.invalidate();

    // The hold began on the action menu (R1 press). Start the countdown immediately;
    // it keeps running while R1 is held. Releasing R1 cancels back to the menu.
    timerRing.setFPS(App::Config::kFrameRate);
    timerRing.setMode(TimerRing::FILL);
    timerRing.setMaxValue(kHoldMs);
    timerRing.setSpeed(1000);  // 1000 units/s -> kHoldMs ms to fill
    timerRing.setProgressColor(finish ? kFinishColor : kDiscardColor);
    timerRing.setValueChangedCallback(mTimerValueChangedCb);
    timerRing.setCompleteCallback(mTimerCompleteCb);
    timerRing.setValue(0);

    mFired = false;
    mLastDisplayedNumber = 3;
    updateCountdownText(3);
    timerRing.animateTo(kHoldMs);  // auto-start (R1 already held from the menu)
}

void TrackHoldConfirmationView::tearDownScreen()
{
    timerRing.stop();
    TrackHoldConfirmationViewBase::tearDownScreen();
}

void TrackHoldConfirmationView::handleKeyEvent(uint8_t key)
{
    // Releasing R1 before the countdown completes cancels and returns to the menu.
    if (key == SDK::GUI::Button::R1_RELEASE && !mFired) {
        timerRing.stop();
        application().gotoTrackActionScreenNoTransition();
    }
    // Everything else is ignored: the countdown runs on its own and only completing
    // it (R1 held for the full duration) confirms the action.
}

void TrackHoldConfirmationView::onTimerValueChanged(int32_t value)
{
    if (value < 0) value = 0;
    // Map the 0..kHoldMs fill onto a 3 -> 2 -> 1 countdown (never 0; completion fires first).
    uint32_t number = 3 - (static_cast<uint32_t>(value) / (kHoldMs / 3));
    if (number < 1) number = 1;
    if (number != mLastDisplayedNumber) {
        mLastDisplayedNumber = number;
        updateCountdownText(number);
    }
}

void TrackHoldConfirmationView::onTimerComplete(int32_t /*value*/)
{
    if (mFired) return;
    mFired = true;
    if (mMode == Model::HoldConfirmMode::Finish) {
        application().gotoTrackSavedScreenNoTransition();
    } else {
        application().gotoTrackDiscardedScreenNoTransition();
    }
}

void TrackHoldConfirmationView::updateCountdownText(uint32_t number)
{
    Unicode::snprintf(timeValueBuffer, TIMEVALUE_SIZE, "%u", number);
    timeValue.invalidate();
}
