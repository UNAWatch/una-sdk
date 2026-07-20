#include <gui/containers/TimeWheel.hpp>

static constexpr int kMenuAnimationSteps = 4;

TimeWheel::TimeWheel()
{
}

void TimeWheel::initialize()
{
    TimeWheelBase::initialize();
}

void TimeWheel::setFormat(bool is12Hour)
{
    mIs12Hour = is12Hour;
    hoursWheel.setNumberOfItems(is12Hour ? 12 : 24);
    hoursWheel.invalidate();
}

void TimeWheel::setTime(uint8_t h, uint8_t m)
{
    if (h < 24 && m < 60) {
        // In 12-hour mode item index 0..11 maps to displayed hour 1..12.
        int16_t hourIndex = h;
        if (mIs12Hour) {
            uint8_t h12 = h % 12;
            if (h12 == 0) {
                h12 = 12;
            }
            hourIndex = static_cast<int16_t>(h12 - 1);
        }

        hoursWheel.animateToItem(hourIndex, 0);
        setHours(hourIndex);

        minutesWheel.animateToItem(m, 0);
        setMinutes(m);
    }
}

void TimeWheel::getTime(uint8_t& h, uint8_t& m)
{
    const int16_t hourIndex = hoursWheel.getSelectedItem();
    h = static_cast<uint8_t>(mIs12Hour ? hourIndex + 1 : hourIndex);
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
    item.setValue(mIs12Hour ? itemIndex + 1 : itemIndex);
}

void TimeWheel::hoursWheelUpdateCenterItem(TimeWheelHoursCenterItem& item, int16_t itemIndex)
{
    item.setValue(mIs12Hour ? itemIndex + 1 : itemIndex);
}

void TimeWheel::minutesWheelUpdateItem(TimeWheelMinutesItem& item, int16_t itemIndex)
{
    item.setValue(itemIndex);
}

void TimeWheel::minutesWheelUpdateCenterItem(TimeWheelMinutesCenterItem& item, int16_t itemIndex)
{
    item.setValue(itemIndex);
}

void TimeWheel::setHours(int16_t index)
{
    // The inactive label mirrors the active wheel: display the mapped hour value.
    const int16_t value = mIs12Hour ? index + 1 : index;
    Unicode::snprintf(hoursInactiveBuffer, HOURSINACTIVE_SIZE, "%02d", value);
    hoursInactive.invalidate();
}

void TimeWheel::setMinutes(int16_t m)
{
    Unicode::snprintf(minutesInactiveBuffer, MINUTESINACTIVE_SIZE, "%02d", m);
    minutesInactive.invalidate();
}
