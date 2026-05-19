#include <gui/containers/TimeWheelMinutesItem.hpp>

TimeWheelMinutesItem::TimeWheelMinutesItem()
{
}

void TimeWheelMinutesItem::initialize()
{
    TimeWheelMinutesItemBase::initialize();
}

void TimeWheelMinutesItem::setValue(int16_t v)
{
    Unicode::snprintf(textBuffer, TEXT_SIZE, "%02d", v);
}
