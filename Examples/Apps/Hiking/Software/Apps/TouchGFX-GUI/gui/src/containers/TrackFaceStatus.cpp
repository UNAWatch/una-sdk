#include <gui/containers/TrackFaceStatus.hpp>
#include <images/BitmapDatabase.hpp>

TrackFaceStatus::TrackFaceStatus()
{}

void TrackFaceStatus::initialize()
{
    TrackFaceStatusBase::initialize();

    // Sensor-status row (GPS + external HR) above the time of day.
    add(mSensorRow);
    mSensorRow.setPosition(0, 20, 240, 24);
    mSensorRow.setIcons(BITMAP_SENSORGPSDARK_ID, BITMAP_SENSORGPSLIGHT_ID,
                        BITMAP_SENSORHRDARK_ID, BITMAP_SENSORHRLIGHT_ID);
}

void TrackFaceStatus::setTime(uint8_t h, uint8_t m, SDK::Message::TimeFormat format)
{
    switch (format) {
        case SDK::Message::TimeFormat::Hour12: {
            const uint8_t h12 = (h % 12 == 0) ? 12 : (h % 12);
            Unicode::snprintf(dayTimeValueBuffer, DAYTIMEVALUE_SIZE, "%u:%02u", h12, m);
        } break;
        case SDK::Message::TimeFormat::Military:
            Unicode::snprintf(dayTimeValueBuffer, DAYTIMEVALUE_SIZE, "%02u%02u", h, m);
            break;
        case SDK::Message::TimeFormat::Hour24:
        default:
            Unicode::snprintf(dayTimeValueBuffer, DAYTIMEVALUE_SIZE, "%u:%02u", h, m);
            break;
    }
    dayTimeValue.invalidate();
}

void TrackFaceStatus::setBatteryLevel(uint8_t level)
{
    battery.setLevel(level);

    Unicode::snprintf(percentValueBuffer, PERCENTVALUE_SIZE, "%u%s",
        level, touchgfx::TypedText(T_TEXT_PERCENT).getText());
    percentValue.invalidate();
}
