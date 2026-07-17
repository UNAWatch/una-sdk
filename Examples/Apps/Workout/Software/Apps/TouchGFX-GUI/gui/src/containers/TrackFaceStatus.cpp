#include <gui/containers/TrackFaceStatus.hpp>
#include <images/BitmapDatabase.hpp>
#include <SDK/Utils/TimeFormat.hpp>

TrackFaceStatus::TrackFaceStatus()
{
}

void TrackFaceStatus::initialize()
{
    TrackFaceStatusBase::initialize();

    // External-HR status icon above the time of day (HR-only app: no GPS icon).
    add(mSensorRow);
    mSensorRow.setPosition(0, 20, 240, 24);
    mSensorRow.setIcons(touchgfx::BITMAP_INVALID, touchgfx::BITMAP_INVALID,
                        BITMAP_SENSORHRDARK_ID, BITMAP_SENSORHRLIGHT_ID);
}

void TrackFaceStatus::setTime(uint8_t h, uint8_t m, SDK::Message::TimeFormat format)
{
    char text[8];
    SDK::Utils::formatTimeOfDay(text, sizeof(text), h, m, format);
    Unicode::strncpy(dayTimeValueBuffer, text, DAYTIMEVALUE_SIZE);
    dayTimeValue.invalidate();
}

void TrackFaceStatus::setBatteryLevel(uint8_t level)
{
    battery.setLevel(level);

    Unicode::snprintf(percentValueBuffer, PERCENTVALUE_SIZE, "%u%s",
        level, touchgfx::TypedText(T_TEXT_PERCENT).getText());
    percentValue.invalidate();
}
