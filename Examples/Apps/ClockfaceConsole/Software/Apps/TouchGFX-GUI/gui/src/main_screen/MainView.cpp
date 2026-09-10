#include <gui/main_screen/MainView.hpp>
#include <gui/common/ConsoleLabels.hpp>

/// The face is square and the clock group is centred on it.
static const int16_t kFaceWidth = 240;

MainView::MainView()
    : mShown{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }  // nothing on screen yet, so the first reading draws
    , mIs12h(false)
{

}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    // The Designer leaves every wildcard on its placeholder, so the real
    // readings go up before the first frame rather than after the first tick.
    // The clock format is adopted first, because it decides how the clock
    // reads and whether the meridiem is on screen at all.
    mIs12h = presenter->is12h();

    setTime(presenter->currentTime());
    setSteps(presenter->steps());
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::place(touchgfx::TextArea &area, int16_t x, int16_t y,
                     int16_t width, int16_t height)
{
    // Repaint where it was, then where it is: setPosition() invalidates
    // neither, and a widget that has moved has left a hole behind it.
    area.invalidate();
    area.setPosition(x, y, width, height);
    area.invalidate();
}

void MainView::layoutClock()
{
    const int16_t hourWidth   = static_cast<int16_t>(hourText.getTextWidth());
    const int16_t minuteWidth = static_cast<int16_t>(minuteText.getTextWidth());
    const int16_t sepWidth    = mIs12h ? kSeparator12 : kSeparator24;

    // The meridiem is deliberately left out of the width the group is centred
    // on, so the digits stay centred on the face and the label hangs off to
    // their right. That is what the design does, and the reason is above the
    // clock: the day of month and the month are both centred, and the clock
    // reads as the third line of that stack only if its digits are too.
    const int16_t total = static_cast<int16_t>(hourWidth + sepWidth + minuteWidth);

    int16_t x = static_cast<int16_t>((kFaceWidth - total) / 2);

    place(hourText, x, kClockY, hourWidth, kClockHeight);
    x = static_cast<int16_t>(x + hourWidth);

    // The colon is centred in the separator, which is why it is given the
    // separator's width rather than its own.
    place(colonText, x, kClockY, sepWidth, kClockHeight);
    x = static_cast<int16_t>(x + sepWidth);

    place(minuteText, x, kClockY, minuteWidth, kClockHeight);
    x = static_cast<int16_t>(x + minuteWidth + kMeridiemGap);

    place(meridiemText, x, static_cast<int16_t>(kClockY + kMeridiemDrop),
          static_cast<int16_t>(meridiemText.getTextWidth()), kMeridiemHeight);

    // invalidate(), not invalidateContent(): the latter does nothing on a
    // widget that has just been hidden, which is exactly when the area it used
    // to cover has to be repainted.
    colonText.setVisible(mIs12h);
    colonText.invalidate();
    meridiemText.setVisible(mIs12h);
    meridiemText.invalidate();
}

void MainView::updateClockText()
{
    if (mIs12h) {
        // 12, not 0, and no leading zero -- both are what the design shows,
        // and dropping the zero is what makes the group narrower and so
        // re-centres it.
        const uint8_t hour12 = (mShown.hour % 12u) == 0u ? 12u : (mShown.hour % 12u);
        Unicode::snprintf(hourTextBuffer, HOURTEXT_SIZE, "%u",
                          static_cast<unsigned>(hour12));

        meridiemText.setTypedText(touchgfx::TypedText(
            App::Labels::kMeridiemLabels[mShown.hour >= 12u ? 1 : 0]));
    } else {
        Unicode::snprintf(hourTextBuffer, HOURTEXT_SIZE, "%02u",
                          static_cast<unsigned>(mShown.hour));
    }

    Unicode::snprintf(minuteTextBuffer, MINUTETEXT_SIZE, "%02u",
                      static_cast<unsigned>(mShown.minute));
}

void MainView::updateDateText()
{
    // Each part is centred across the full width, so writing them is all there
    // is to do -- none of them has to be positioned.
    Unicode::snprintf(weekdayTextBuffer, WEEKDAYTEXT_SIZE, "%s",
                      touchgfx::TypedText(
                          App::Labels::kDayLabels[mShown.wday % 7u]).getText());
    weekdayText.invalidate();

    Unicode::snprintf(dayTextBuffer, DAYTEXT_SIZE, "%02u",
                      static_cast<unsigned>(mShown.mday));
    dayText.invalidate();

    Unicode::snprintf(monthTextBuffer, MONTHTEXT_SIZE, "%s",
                      touchgfx::TypedText(
                          App::Labels::kMonthLabels[mShown.mon % 12u]).getText());
    monthText.invalidate();
}

void MainView::setTime(const WallTime &time)
{
    if (time == mShown) {
        return;
    }

    // The meridiem turns on the hour and the clock's width changes with it, so
    // the group is laid out again whenever any part of the reading moves. At
    // once a minute, splitting that finer would buy nothing.
    mShown = time;

    updateClockText();
    layoutClock();
    updateDateText();
}

void MainView::setClockFormat(bool is12h)
{
    if (is12h == mIs12h) {
        return;
    }

    mIs12h = is12h;

    updateClockText();
    layoutClock();
}

void MainView::setSteps(uint32_t steps)
{
    App::Labels::formatGrouped(stepsTextBuffer, STEPSTEXT_SIZE, steps);
    stepsText.invalidate();
}
