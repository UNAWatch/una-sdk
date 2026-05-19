#include <gui/containers/TimeWheelHoursItem.hpp>

TimeWheelHoursItem::TimeWheelHoursItem()
{
}

void TimeWheelHoursItem::initialize()
{
    TimeWheelHoursItemBase::initialize();
}

void TimeWheelHoursItem::setValue(int16_t v)
{
    Unicode::snprintf(textBuffer, TEXT_SIZE, "%02d", v);
}
