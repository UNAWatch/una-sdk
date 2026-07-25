#include <gui/containers/TimeWheelHoursCenterItem.hpp>

TimeWheelHoursCenterItem::TimeWheelHoursCenterItem()
{
}

void TimeWheelHoursCenterItem::initialize()
{
    TimeWheelHoursCenterItemBase::initialize();
}

void TimeWheelHoursCenterItem::setValue(int16_t v)
{
    Unicode::snprintf(textBuffer, TEXT_SIZE, "%02d", v);
}
