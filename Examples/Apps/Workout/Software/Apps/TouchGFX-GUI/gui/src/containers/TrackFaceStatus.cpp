#include <gui/containers/TrackFaceStatus.hpp>
#include <images/BitmapDatabase.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Color.hpp>

TrackFaceStatus::TrackFaceStatus()
{
}

void TrackFaceStatus::initialize()
{
    TrackFaceStatusBase::initialize();

    // Draw the time left-aligned (the generated base centres it) so the AM/PM
    // suffix can follow the digits; setTime() re-centres the whole group.
    dayTimeValue.setTypedText(touchgfx::TypedText(T_TMP_SEMIBOLD_60_L));
    dayTimeValue.setY(kTimeY);

    // AM/PM suffix, shown only in 12-hour format.
    mMeridiem.setColor(touchgfx::Color::getColorFromRGB(192, 192, 192));
    mMeridiem.setLinespacing(0);
    mMeridiem.setTypedText(touchgfx::TypedText(T_TMP_MEDIUM_18_L));
    mMeridiem.setWildcard(mMeridiemBuffer);
    mMeridiem.setVisible(false);
    add(mMeridiem);

    // External-HR status icon above the time of day (HR-only app: no GPS icon).
    add(mSensorRow);
    mSensorRow.setPosition(0, 20, 240, 24);
    mSensorRow.setIcons(touchgfx::BITMAP_INVALID, touchgfx::BITMAP_INVALID,
                        BITMAP_SENSORHRDARK_ID, BITMAP_SENSORHRLIGHT_ID);
}

void TrackFaceStatus::setTime(uint8_t h, uint8_t m, bool is12Hour)
{
    // Invalidate the old glyph rectangles before the text and positions move.
    dayTimeValue.invalidate();
    mMeridiem.invalidate();

    uint8_t hour = h;
    bool    pm   = false;
    if (is12Hour) {
        pm   = (h >= 12);
        hour = h % 12;
        if (hour == 0) {
            hour = 12;
        }
    }

    Unicode::snprintf(dayTimeValueBuffer, DAYTIMEVALUE_SIZE, "%u:%02u", hour, m);
    dayTimeValue.setWildcard(dayTimeValueBuffer);
    const uint16_t timeW = dayTimeValue.getTextWidth();

    uint16_t merW   = 0;
    uint16_t groupW = timeW;
    if (is12Hour) {
        mMeridiemBuffer[0] = pm ? 'P' : 'A';
        mMeridiemBuffer[1] = 'M';
        mMeridiemBuffer[2] = 0;
        mMeridiem.setWildcard(mMeridiemBuffer);
        merW   = mMeridiem.getTextWidth();
        groupW = static_cast<uint16_t>(timeW + kMeridiemGap + merW);
    }

    // Centre the digits (+ suffix) as one group on the 240px face.
    const int16_t groupLeft = static_cast<int16_t>((getWidth() - groupW) / 2);

    dayTimeValue.setPosition(groupLeft, kTimeY, static_cast<int16_t>(timeW + 4), kTimeH);
    dayTimeValue.invalidate();

    if (is12Hour) {
        mMeridiem.setPosition(static_cast<int16_t>(groupLeft + timeW + kMeridiemGap),
                              kMeridiemY, static_cast<int16_t>(merW + 4), kMeridiemH);
        mMeridiem.setVisible(true);
        mMeridiem.invalidate();
    } else {
        mMeridiem.setVisible(false);
    }
}

void TrackFaceStatus::setBatteryLevel(uint8_t level)
{
    battery.setLevel(level);

    Unicode::snprintf(percentValueBuffer, PERCENTVALUE_SIZE, "%u%s",
        level, touchgfx::TypedText(T_TEXT_PERCENT).getText());
    percentValue.invalidate();
}
