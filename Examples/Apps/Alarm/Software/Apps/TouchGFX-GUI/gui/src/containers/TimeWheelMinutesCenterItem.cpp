#include <gui/containers/TimeWheelMinutesCenterItem.hpp>

TimeWheelMinutesCenterItem::TimeWheelMinutesCenterItem()
{
}

void TimeWheelMinutesCenterItem::initialize()
{
    TimeWheelMinutesCenterItemBase::initialize();
}

void TimeWheelMinutesCenterItem::setValue(int16_t v)
{
    Unicode::snprintf(textBuffer, TEXT_SIZE, "%02d", v);
}
