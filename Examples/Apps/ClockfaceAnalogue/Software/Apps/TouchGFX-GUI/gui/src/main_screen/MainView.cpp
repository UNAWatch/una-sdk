#include <gui/main_screen/MainView.hpp>
#include <gui/common/AnalogueLabels.hpp>

#include <cmath>

// Degrees per unit, from the twelve-hour dial.
static constexpr float kDegPerMinute     = 6.0f;    ///< 360 / 60
static constexpr float kDegPerHour       = 30.0f;   ///< 360 / 12
static constexpr float kDegPerHourMinute = 0.5f;    ///< 30 / 60, the hour hand's creep

static constexpr float kDegToRad = 3.14159265f / 180.0f;

MainView::MainView()
    : mShown{ 0xFF, 0xFF, 0xFF, 0xFF }  // no reading on screen yet, so the first one draws
    , mPivotX(0.0f)
    , mPivotY(0.0f)
    , mHour{}
    , mMinute{}
{

}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    // The hub is the pivot, so moving it in the Designer moves what the hands
    // turn about without anything here having to agree with it.
    hub.getCenter(mPivotX, mPivotY);

    captureStroke(mHour[0], hourStem);
    captureStroke(mHour[1], hourBody);
    captureStroke(mMinute[0], minuteStem);
    captureStroke(mMinute[1], minuteBody);

    // The Designer leaves the face on its placeholder, so put the real reading
    // up before the first frame rather than after the first tick.
    setTime(presenter->currentTime());
    setBatteryLevel(presenter->batteryLevel());

    // The Designer keeps the icon visible so it can be positioned there; what
    // decides whether it is on screen is the state, which starts unmuted.
    setAlertsMuted(presenter->alertsMuted());
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::captureStroke(Stroke &stroke, touchgfx::Line &line)
{
    float x = 0.0f;
    float y = 0.0f;

    line.getStart(x, y);
    stroke.sx = x - mPivotX;
    stroke.sy = y - mPivotY;

    line.getEnd(x, y);
    stroke.ex = x - mPivotX;
    stroke.ey = y - mPivotY;

    stroke.line = &line;
}

void MainView::setHandAngle(Stroke *strokes, float degrees)
{
    const float a = degrees * kDegToRad;
    const float c = cosf(a);
    const float s = sinf(a);

    // Screen y grows downwards, so this is the plain rotation matrix and it
    // already turns clockwise, which is the direction a clock reads.
    for (unsigned i = 0; i < kStrokesPerHand; i++) {
        Stroke &st = strokes[i];
        if (!st.line) {
            continue;
        }

        st.line->updateStart(mPivotX + st.sx * c - st.sy * s,
                             mPivotY + st.sx * s + st.sy * c);
        st.line->updateEnd(mPivotX + st.ex * c - st.ey * s,
                           mPivotY + st.ex * s + st.ey * c);
    }
}

void MainView::setTime(const WallTime &time)
{
    if (time == mShown) {
        return;
    }

    const bool handsMoved = (time.hour != mShown.hour) || (time.minute != mShown.minute);
    const bool dayTurned  = (time.mday != mShown.mday) || (time.wday != mShown.wday);

    mShown = time;

    if (handsMoved) {
        // The hour hand creeps with the minutes instead of jumping on the hour,
        // which is what a mechanical movement does and what makes the pair read
        // as one time rather than two readings.
        const float minuteAngle = static_cast<float>(time.minute) * kDegPerMinute;
        const float hourAngle   = static_cast<float>(time.hour % 12) * kDegPerHour +
                                  static_cast<float>(time.minute) * kDegPerHourMinute;

        setHandAngle(mHour, hourAngle);
        setHandAngle(mMinute, minuteAngle);

        // Both stems sweep under the hub, so it is repainted with them.
        hub.invalidate();
    }

    if (dayTurned) {
        dayText.setTypedText(touchgfx::TypedText(App::Labels::kDayLabels[time.wday % 7]));
        dayText.invalidate();

        Unicode::snprintf(dateTextBuffer, DATETEXT_SIZE, "%u",
                          static_cast<unsigned>(time.mday));
        dateText.invalidate();
    }
}

void MainView::setBatteryLevel(uint8_t level)
{
    battery.setLevel(level);
}

void MainView::setAlertsMuted(bool muted)
{
    speakerMute.setVisible(muted);

    // invalidate(), not invalidateContent(): the latter does nothing on a
    // widget that has just been hidden, which is exactly when the area it
    // used to cover has to be repainted.
    speakerMute.invalidate();
}
