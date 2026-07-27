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
}

void EditView::confirm()
{
    const uint16_t dur = static_cast<uint16_t>(mMins.getValue() * 60 + mSecs.getValue());
    presenter->setDuration(dur);
    application().gotoAlertScreenNoTransition();
}

void EditView::handleKeyEvent(uint8_t key)
{
    SpherePicker& active = (mStep == STEP_MINS) ? mMins : mSecs;

    if (key == SDK::GUI::Button::L1) {
        active.selectPrev();
    }
    else if (key == SDK::GUI::Button::L2) {
        active.selectNext();
    }
    else if (key == SDK::GUI::Button::R1) {
        if (mStep == STEP_MINS) {
            mStep = STEP_SECS;
            updateActive();
        } else {
            confirm();
        }
    }
    else if (key == SDK::GUI::Button::R2) {
        if (mStep == STEP_SECS) {
            mStep = STEP_MINS;
            updateActive();
        } else {
            application().gotoMainScreenNoTransition();
        }
    }
}
