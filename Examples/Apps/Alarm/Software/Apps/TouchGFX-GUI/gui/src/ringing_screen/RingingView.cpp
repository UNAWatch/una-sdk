#include <gui/ringing_screen/RingingView.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Color.hpp>

static constexpr uint16_t kSnoozeTicks = SDK::Utils::secToTicks(120, App::Config::kFrameRate);
static constexpr uint16_t kPlayTicks = SDK::Utils::secToTicks(5, App::Config::kFrameRate);

RingingView::RingingView()
    : mSnoozeCb(this, &RingingView::onSnooze)
    , mPlayCb(this, &RingingView::onPlay)
{
    // Top-right = Snooze (teal), bottom-right = Stop (grey).
    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::TEAL);
    buttons.setR2(Buttons::WHITE);
}

void RingingView::setupScreen()
{
    RingingViewBase::setupScreen();

    // The redesign shows a large "Alarm" title instead of the bell icon + time.
    icon.setVisible(false);
    timeValue.setVisible(false);

    mAlarmTitle.setTypedText(touchgfx::TypedText(T_TMP_SEMIBOLD_40));
    mAlarmTitle.setColor(touchgfx::Color::getColorFromRGB(192, 128, 0));
    mAlarmTitle.setLinespacing(0);
    mAlarmTitle.setWildcard(mAlarmTitleBuffer);
    Unicode::snprintf(mAlarmTitleBuffer, ALARM_TITLE_SIZE, "%s",
                      touchgfx::TypedText(T_TEXT_ALARM).getText());
    mAlarmTitle.setPosition(0, 96, 240, 55);
    add(mAlarmTitle);

    // Snooze -> top-right (teal); Stop -> bottom-right (grey); italic labels
    // right-aligned next to their buttons. The wildcard buffers are populated
    // with "Snooze"/"Stop" by the generated base; re-bind them here so this
    // retarget to a wildcard-template TypedText is self-contained.
    snoozeText.setWildcard(snoozeTextBuffer);
    snoozeText.setTypedText(touchgfx::TypedText(T_TMP_ITALIC_18_R));
    snoozeText.setColor(touchgfx::Color::getColorFromRGB(0, 128, 128));
    snoozeText.setPosition(106, 55, 100, 27);

    stopText.setWildcard(stopTextBuffer);
    stopText.setTypedText(touchgfx::TypedText(T_TMP_ITALIC_18_R));
    stopText.setColor(touchgfx::Color::getColorFromRGB(192, 192, 192));
    stopText.setPosition(108, 156, 100, 27);

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
    if (key == SDK::GUI::Button::R1) {
        presenter->snooze();
    }

    if (key == SDK::GUI::Button::R2) {
        presenter->stop();
    }
}
