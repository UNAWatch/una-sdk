#include <gui/containers/TrackFaceStatus.hpp>

TrackFaceStatus::TrackFaceStatus()
{
}

void TrackFaceStatus::initialize()
{
    TrackFaceStatusBase::initialize();
}

void TrackFaceStatus::setTime(uint8_t h, uint8_t m)
{
    Unicode::snprintf(dayTimeValueBuffer, DAYTIMEVALUE_SIZE, "%u:%02u", h, m);
    dayTimeValue.invalidate();
}

void TrackFaceStatus::setBatteryLevel(uint8_t level)
{
    battery.setLevel(level);

    Unicode::snprintf(percentValueBuffer, PERCENTVALUE_SIZE, "%u%s",
        level, touchgfx::TypedText(T_TEXT_PERCENT).getText());
    percentValue.invalidate();
}
