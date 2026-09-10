#include <gui/main_screen/MainView.hpp>
#include <gui/common/RetroLabels.hpp>

#include <images/BitmapDatabase.hpp>

/// The face is square and the clock is centred on it.
static const int16_t kFaceWidth = 240;

/// Charge bands, one supplied bitmap each. Below the first the indicator is
/// taken down rather than drawn empty: there is no artwork for an unlit
/// battery, and by then the kernel's own low-battery screen is imminent.
static const uint8_t kBand2 = 25;
static const uint8_t kBand3 = 50;
static const uint8_t kBand4 = 75;

/// What a heart-rate row shows when the service has no reading it trusts.
static const char kNoReading[] = "---";

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
    // The order matters only in that the clock format decides how the clock
    // and the date read, so it is adopted before either is written.
    mIs12h = presenter->is12h();

    setTime(presenter->currentTime());
    setHealth(presenter->health());
    setBatteryLevel(presenter->batteryLevel());
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::place(touchgfx::TextArea &area, int16_t x, int16_t width)
{
    // Repaint where it was, then where it is: setPosition() invalidates
    // neither, and a widget that has moved has left a hole behind it.
    area.invalidate();
    area.setPosition(x, kClockY, width, kClockHeight);
    area.invalidate();
}

void MainView::layoutClock()
{
    const int16_t hourWidth   = static_cast<int16_t>(hourText.getTextWidth());
    const int16_t minuteWidth = static_cast<int16_t>(minuteText.getTextWidth());
    const int16_t colonWidth  = mIs12h
                              ? static_cast<int16_t>(colonText.getTextWidth())
                              : kGap24;

    const int16_t total = static_cast<int16_t>(hourWidth + colonWidth + minuteWidth);

    int16_t x = static_cast<int16_t>((kFaceWidth - total) / 2);

    place(hourText, x, hourWidth);
    x = static_cast<int16_t>(x + hourWidth);

    place(colonText, x, colonWidth);
    x = static_cast<int16_t>(x + colonWidth);

    place(minuteText, x, minuteWidth);

    // invalidate(), not invalidateContent(): the latter does nothing on a
    // widget that has just been hidden, which is exactly when the area it used
    // to cover has to be repainted.
    colonText.setVisible(mIs12h);
    colonText.invalidate();
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
    } else {
        Unicode::snprintf(hourTextBuffer, HOURTEXT_SIZE, "%02u",
                          static_cast<unsigned>(mShown.hour));
    }

    Unicode::snprintf(minuteTextBuffer, MINUTETEXT_SIZE, "%02u",
                      static_cast<unsigned>(mShown.minute));
}

void MainView::updateDateText()
{
    const touchgfx::TypedText day(App::Labels::kDayLabels[mShown.wday % 7u]);
    const touchgfx::TypedText month(App::Labels::kMonthLabels[mShown.mon % 12u]);

    if (mIs12h) {
        const touchgfx::TypedText meridiem(
            App::Labels::kMeridiemLabels[mShown.hour >= 12u ? 1 : 0]);

        Unicode::snprintf(dateTextBuffer, DATETEXT_SIZE, "%s %u %s|%s",
                          day.getText(),
                          static_cast<unsigned>(mShown.mday),
                          month.getText(),
                          meridiem.getText());
    } else {
        Unicode::snprintf(dateTextBuffer, DATETEXT_SIZE, "%s %u %s",
                          day.getText(),
                          static_cast<unsigned>(mShown.mday),
                          month.getText());
    }

    dateText.invalidate();
}

void MainView::setTime(const WallTime &time)
{
    if (time == mShown) {
        return;
    }

    // The date line carries the meridiem, so it turns on the hour as well as
    // on the day; and the clock's width changes with the hour, so the layout
    // is redone whenever either part of the reading moves. Both are cheap
    // enough at once a minute that splitting them would buy nothing.
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
    updateDateText();
}

void MainView::setBatteryLevel(uint8_t level)
{
    if (level == 0u) {
        battery.setVisible(false);
        battery.invalidate();
        return;
    }

    const uint16_t bitmap = (level >= kBand4) ? BITMAP_BATTERY4_46X15_ID :
                            (level >= kBand3) ? BITMAP_BATTERY3_46X15_ID :
                            (level >= kBand2) ? BITMAP_BATTERY2_46X15_ID :
                                                BITMAP_BATTERY1_46X15_ID;

    battery.setBitmap(touchgfx::Bitmap(bitmap));
    battery.setVisible(true);
    battery.invalidate();
}

void MainView::setHealth(const DailyHealth &health)
{
    App::Labels::formatGrouped(stepsTextBuffer, STEPSTEXT_SIZE, health.steps);
    stepsText.invalidate();

    App::Labels::formatGrouped(activityTextBuffer, ACTIVITYTEXT_SIZE,
                               health.activityMinutes);
    activityText.invalidate();

    if (health.bpm == 0u) {
        // The service has already applied the trust gate, so a zero here means
        // "no reading worth showing" rather than a rate of zero.
        Unicode::strncpy(hrTextBuffer, kNoReading, HRTEXT_SIZE);
    } else {
        Unicode::snprintf(hrTextBuffer, HRTEXT_SIZE, "%u",
                          static_cast<unsigned>(health.bpm));
    }
    hrText.invalidate();
}
