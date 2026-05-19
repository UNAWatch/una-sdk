#include <gui/containers/TimeWheel.hpp>

static constexpr int kMenuAnimationSteps = 4;

TimeWheel::TimeWheel()
{
}

void TimeWheel::initialize()
{
    TimeWheelBase::initialize();
}

void TimeWheel::setTime(uint8_t h, uint8_t m)
{
    if (h < 24 && m < 60) {
        hoursWheel.animateToItem(h, 0);
        setHours(h);

        minutesWheel.animateToItem(m, 0);
        setMinutes(m);
    }
}

void TimeWheel::getTime(uint8_t& h, uint8_t& m)
{
    h = hoursWheel.getSelectedItem();
    m = minutesWheel.getSelectedItem();
}

void TimeWheel::setActiveHours()
{
    mMinutesActive = false;

    minutesWheel.setVisible(false);
    minutesWheel.invalidate();
    minutesInactive.setVisible(true);
    minutesInactive.invalidate();

    hoursWheel.setVisible(true);
    hoursWheel.invalidate();
    hoursInactive.setVisible(false);
    hoursInactive.invalidate();
}

void TimeWheel::setActiveMinutes()
{
    mMinutesActive = true;

    minutesWheel.setVisible(true);
    minutesWheel.invalidate();
    minutesInactive.setVisible(false);
    minutesInactive.invalidate();

    hoursWheel.setVisible(false);
    hoursWheel.invalidate();
    hoursInactive.setVisible(true);
    hoursInactive.invalidate();
}

void TimeWheel::incValue()
{
    if (mMinutesActive) {
        if (minutesWheel.getNumberOfItems() <= 1) {
            return;
        }
        int16_t p = minutesWheel.getSelectedItem() + 1;
        minutesWheel.animateToItem(p, kMenuAnimationSteps);
        setMinutes(minutesWheel.getSelectedItem());
    }
    else {
        if (hoursWheel.getNumberOfItems() <= 1) {
            return;
        }
        int16_t p = hoursWheel.getSelectedItem() + 1;
        hoursWheel.animateToItem(p, kMenuAnimationSteps);
        setHours(hoursWheel.getSelectedItem());
    }
}

void TimeWheel::decValue()
{
    if (mMinutesActive) {
        if (minutesWheel.getNumberOfItems() <= 1) {
            return;
        }
        int16_t p = minutesWheel.getSelectedItem() - 1;
        minutesWheel.animateToItem(p, kMenuAnimationSteps);
        setMinutes(minutesWheel.getSelectedItem());
    }
    else {
        if (hoursWheel.getNumberOfItems() <= 1) {
            return;
        }
        int16_t p = hoursWheel.getSelectedItem() - 1;
        hoursWheel.animateToItem(p, kMenuAnimationSteps);
        setHours(hoursWheel.getSelectedItem());
    }
}

void TimeWheel::hoursWheelUpdateItem(TimeWheelHoursItem& item, int16_t itemIndex)
{
    item.setValue(itemIndex);
}

void TimeWheel::hoursWheelUpdateCenterItem(TimeWheelHoursCenterItem& item, int16_t itemIndex)
{
    item.setValue(itemIndex);
}

void TimeWheel::minutesWheelUpdateItem(TimeWheelMinutesItem& item, int16_t itemIndex)
{
    item.setValue(itemIndex);
}

void TimeWheel::minutesWheelUpdateCenterItem(TimeWheelMinutesCenterItem& item, int16_t itemIndex)
{
    item.setValue(itemIndex);
}

void TimeWheel::setHours(int16_t h)
{
    Unicode::snprintf(hoursInactiveBuffer, HOURSINACTIVE_SIZE, "%02d", h);
    hoursInactive.invalidate();
}

void TimeWheel::setMinutes(int16_t m)
{
    Unicode::snprintf(minutesInactiveBuffer, MINUTESINACTIVE_SIZE, "%02d", m);
    minutesInactive.invalidate();
}
