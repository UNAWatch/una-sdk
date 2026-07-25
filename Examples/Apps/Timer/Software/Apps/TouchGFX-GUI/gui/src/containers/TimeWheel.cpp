#include <gui/containers/TimeWheel.hpp>
#include <touchgfx/Color.hpp>
#include <texts/TextKeysAndLanguages.hpp>

static constexpr int kMenuAnimationSteps = 4;

namespace {
// Teal while the AM/PM field is active, light grey once settled (matches the
// wheels' selected/idle colours).
const touchgfx::colortype kActiveColor = touchgfx::Color::getColorFromRGB(0, 128, 128);
const touchgfx::colortype kValueColor  = touchgfx::Color::getColorFromRGB(192, 192, 192);
}

TimeWheel::TimeWheel()
{
}

void TimeWheel::initialize()
{
    TimeWheelBase::initialize();

    // AM/PM suffix (shown only in 12-hour mode; content/colour set by updateAmPm).
    mAmPmBuffer[0] = 'A'; mAmPmBuffer[1] = 'M'; mAmPmBuffer[2] = 0;
    mAmPm.setTypedText(touchgfx::TypedText(T_TMP_SEMIBOLD_20_L));
    mAmPm.setWildcard(mAmPmBuffer);
    mAmPm.setLinespacing(0);
    mAmPm.setColor(kValueColor);
    mAmPm.setVisible(false);
    add(mAmPm);
}

void TimeWheel::applyLayout()
{
    if (mIs12Hour) {
        // Shift HH:MM left just enough (its empty outer margins clip harmlessly)
        // to leave room for the AM/PM suffix on the right.
        hoursWheel.setPosition(-16, 41, 94, 143);
        hoursInactive.setPosition(-16, 25, 94, 76);
        semicolon.setPosition(78, 25, 12, 76);
        minutesWheel.setPosition(90, 41, 94, 143);
        minutesInactive.setPosition(90, 25, 94, 76);

        // Single suffix, baseline-aligned to the digits (matches the display).
        mAmPm.setPosition(168, 60, 32, 30);
        mAmPm.setVisible(true);
    } else {
        // Original full-width layout from the generated base.
        hoursWheel.setPosition(0, 41, 94, 143);
        hoursInactive.setPosition(0, 25, 94, 76);
        semicolon.setPosition(94, 25, 12, 76);
        minutesWheel.setPosition(106, 41, 94, 143);
        minutesInactive.setPosition(106, 25, 94, 76);

        mAmPm.setVisible(false);
    }

    invalidate();
}

void TimeWheel::updateAmPm()
{
    if (!mIs12Hour) {
        return;
    }

    // Show only the selected value; teal while the field is being edited.
    mAmPmBuffer[0] = mIsPm ? 'P' : 'A';
    mAmPmBuffer[1] = 'M';
    mAmPmBuffer[2] = 0;
    mAmPm.setWildcard(mAmPmBuffer);
    mAmPm.setColor(mAmPmActive ? kActiveColor : kValueColor);
    mAmPm.invalidate();
}

void TimeWheel::setFormat(bool is12Hour)
{
    mIs12Hour = is12Hour;
    hoursWheel.setNumberOfItems(is12Hour ? 12 : 24);
    hoursWheel.invalidate();

    applyLayout();
    updateAmPm();
}

void TimeWheel::setTime(uint8_t h, uint8_t m)
{
    if (h < 24 && m < 60) {
        // In 12-hour mode item index 0..11 maps to displayed hour 1..12.
        int16_t hourIndex = h;
        if (mIs12Hour) {
            mIsPm = (h >= 12);
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

        updateAmPm();
    }
}

void TimeWheel::getTime(uint8_t& h, uint8_t& m)
{
    const int16_t hourIndex = hoursWheel.getSelectedItem();
    if (mIs12Hour) {
        const uint8_t base = static_cast<uint8_t>((hourIndex + 1) % 12);   // 12 -> 0
        h = static_cast<uint8_t>(base + (mIsPm ? 12 : 0));
    } else {
        h = static_cast<uint8_t>(hourIndex);
    }
    m = minutesWheel.getSelectedItem();
}

void TimeWheel::setActiveHours()
{
    mMinutesActive = false;
    mAmPmActive = false;

    minutesWheel.setVisible(false);
    minutesWheel.invalidate();
    minutesInactive.setVisible(true);
    minutesInactive.invalidate();

    hoursWheel.setVisible(true);
    hoursWheel.invalidate();
    hoursInactive.setVisible(false);
    hoursInactive.invalidate();

    updateAmPm();
}

void TimeWheel::setActiveMinutes()
{
    mMinutesActive = true;
    mAmPmActive = false;

    minutesWheel.setVisible(true);
    minutesWheel.invalidate();
    minutesInactive.setVisible(false);
    minutesInactive.invalidate();

    hoursWheel.setVisible(false);
    hoursWheel.invalidate();
    hoursInactive.setVisible(true);
    hoursInactive.invalidate();

    updateAmPm();
}

void TimeWheel::setActiveAmPm()
{
    mMinutesActive = false;
    mAmPmActive = true;

    // Neither number is being edited: show both as settled (static) values.
    hoursWheel.setVisible(false);
    hoursWheel.invalidate();
    hoursInactive.setVisible(true);
    hoursInactive.invalidate();

    minutesWheel.setVisible(false);
    minutesWheel.invalidate();
    minutesInactive.setVisible(true);
    minutesInactive.invalidate();

    updateAmPm();
}

void TimeWheel::incValue()
{
    if (mAmPmActive) {
        mIsPm = !mIsPm;
        updateAmPm();
        return;
    }

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
        const int16_t before = hoursWheel.getSelectedItem();
        hoursWheel.animateToItem(before + 1, kMenuAnimationSteps);
        const int16_t after = hoursWheel.getSelectedItem();
        setHours(after);
        maybeFlipMeridiem(before, after);
    }
}

void TimeWheel::decValue()
{
    if (mAmPmActive) {
        mIsPm = !mIsPm;
        updateAmPm();
        return;
    }

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
        const int16_t before = hoursWheel.getSelectedItem();
        hoursWheel.animateToItem(before - 1, kMenuAnimationSteps);
        const int16_t after = hoursWheel.getSelectedItem();
        setHours(after);
        maybeFlipMeridiem(before, after);
    }
}

void TimeWheel::maybeFlipMeridiem(int16_t beforeIndex, int16_t afterIndex)
{
    if (!mIs12Hour) {
        return;
    }

    // Displayed hour = index + 1; flip only across the 11<->12 boundary.
    const int before = beforeIndex + 1;
    const int after  = afterIndex + 1;
    if ((before == 11 && after == 12) || (before == 12 && after == 11)) {
        mIsPm = !mIsPm;
        updateAmPm();
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
