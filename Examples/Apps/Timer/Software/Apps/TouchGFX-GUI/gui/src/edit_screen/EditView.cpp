#include <gui/edit_screen/EditView.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <SDK/GUI/Button.hpp>
#include <SDK/GUI/Color.hpp>

// Value step per column, kept as constants so seconds can move to a coarser
// step (e.g. 5) without touching the logic.
static constexpr int16_t kMinsStep = 1;
static constexpr int16_t kSecsStep = 1;

static constexpr int16_t kMinsMax  = 99;
static constexpr int16_t kSecsMax  = 59;

// Hold-to-repeat timing (GUI ticks). A held button starts auto-scrolling after
// kHoldStart, then repeats every kRepeatSlow ticks and speeds up one tick per
// step down to kRepeatFast, so scanning a long way ramps up smoothly.
static constexpr uint32_t kHoldStart  = SDK::Utils::secToTicks(1, App::Config::kFrameRate);
static constexpr int16_t  kRepeatSlow = 4;
static constexpr int16_t  kRepeatFast = 1;

EditView::EditView()
{
}

void EditView::setupScreen()
{
    EditViewBase::setupScreen();

    title.set(T_TEXT_SET_TIMER_UC);

    buttons.setL1(Buttons::WHITE);
    buttons.setL2(Buttons::WHITE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);

    // Box reaches low enough that the second neighbour is fully drawn; the third
    // sits just past the bottom and slides up into view. Centre stays at y120.
    mMins.initialize();
    add(mMins);
    mMins.setColumn(10, 44, 105, 170, 76);
    mMins.setRange(kMinsMax, kMinsStep);
    mMins.setAnimationSteps(App::Config::kMenuAnimationSteps);

    mSecs.initialize();
    add(mSecs);
    mSecs.setColumn(125, 44, 105, 170, 76);
    mSecs.setRange(kSecsMax, kSecsStep);
    mSecs.setAnimationSteps(App::Config::kMenuAnimationSteps);

    mStep = STEP_MINS;
    updateActive();
}

void EditView::tearDownScreen()
{
    EditViewBase::tearDownScreen();
}

void EditView::set(uint16_t durationSec)
{
    mMins.setValue(static_cast<int16_t>(durationSec / 60));
    mSecs.setValue(static_cast<int16_t>(durationSec % 60));
}

void EditView::updateActive()
{
    const bool minsActive = (mStep == STEP_MINS);

    mMins.setActive(minsActive);
    mSecs.setActive(!minsActive);

    minsHdr.setColor(minsActive ? SDK::GUI::Color::TEAL : SDK::GUI::Color::WHITE);
    secsHdr.setColor(minsActive ? SDK::GUI::Color::WHITE : SDK::GUI::Color::TEAL);
    minsHdr.invalidate();
    secsHdr.invalidate();

    syncConfirmButton();
}

void EditView::syncConfirmButton()
{
    // R1 advances on the Mins step, and confirms on the Secs step -- but a
    // 00:00 timer cannot be started, so hide (and ignore) the confirm then.
    const bool isZero = (mMins.getValue() * 60 + mSecs.getValue()) == 0;
    const bool showR1 = (mStep == STEP_MINS) || !isZero;
    buttons.setR1(showR1 ? Buttons::AMBER : Buttons::NONE);
}

void EditView::confirm()
{
    const uint16_t dur = static_cast<uint16_t>(mMins.getValue() * 60 + mSecs.getValue());
    presenter->setDuration(dur);
    application().gotoAlertScreenNoTransition();
}

void EditView::scrollActive(bool forward, bool snap)
{
    SpherePicker& active = (mStep == STEP_MINS) ? mMins : mSecs;
    if (forward) {
        active.selectNext();
    } else {
        active.selectPrev();
    }
    if (snap) {
        active.setValue(active.getValue());   // pin the display to the new value
    }
    syncConfirmButton();
}

void EditView::handleKeyEvent(uint8_t key)
{
    switch (key) {
    // Click: a single step (a short tap that never becomes a hold).
    case SDK::GUI::Button::L1:
        scrollActive(false);
        break;
    case SDK::GUI::Button::L2:
        scrollActive(true);
        break;

    // Press/release bracket a hold; the tick handler does the repeating.
    case SDK::GUI::Button::L1_PRESS:
        mHeldDir = SDK::GUI::Button::L1;
        mHoldTicks = 0;
        break;
    case SDK::GUI::Button::L2_PRESS:
        mHeldDir = SDK::GUI::Button::L2;
        mHoldTicks = 0;
        break;
    case SDK::GUI::Button::L1_RELEASE:
    case SDK::GUI::Button::L2_RELEASE:
        mHeldDir = 0;
        break;

    case SDK::GUI::Button::R1:
        mHeldDir = 0;
        if (mStep == STEP_MINS) {
            mStep = STEP_SECS;
            updateActive();
        } else if (mMins.getValue() * 60 + mSecs.getValue() > 0) {
            confirm();   // 00:00 is not startable -- R1 is hidden and ignored
        }
        break;
    case SDK::GUI::Button::R2:
        mHeldDir = 0;
        if (mStep == STEP_SECS) {
            mStep = STEP_MINS;
            updateActive();
        } else {
            application().gotoMainScreenNoTransition();
        }
        break;

    default:
        break;
    }
}

void EditView::handleTickEvent()
{
    EditViewBase::handleTickEvent();

    if (mHeldDir == 0) {
        return;
    }

    // Wait out the hold threshold; a tap releases before this and never repeats.
    if (++mHoldTicks < kHoldStart) {
        return;
    }
    if (mHoldTicks == kHoldStart) {
        mRepeatInterval  = kRepeatSlow;
        mRepeatCountdown = 0;   // fire the first auto-scroll straight away
    }

    if (--mRepeatCountdown > 0) {
        return;
    }
    scrollActive(mHeldDir == SDK::GUI::Button::L2, true);   // snap: fast + no overshoot
    mRepeatCountdown = mRepeatInterval;
    if (mRepeatInterval > kRepeatFast) {
        --mRepeatInterval;   // accelerate toward the fastest step
    }
}
