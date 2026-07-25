#ifndef TIMEWHEEL_HPP
#define TIMEWHEEL_HPP

#include <gui_generated/containers/TimeWheelBase.hpp>

/**
 * @brief Composite container with two scroll wheels for time entry (HH:MM).
 *
 * Contains a hours wheel (00-23) and a minutes wheel (00-59) that share the
 * same layout. Only one wheel is visible at a time; the screen switches
 * between them by calling setActiveHours() / setActiveMinutes().
 *
 * Typical usage in a screen:
 * @code
 *   timeMenu.setTime(timer.timeHours, timer.timeMinutes);
 *   timeMenu.setActiveHours();   // start editing hours
 *   // user presses L1/L2 -> timeMenu.decValue() / incValue()
 *   timeMenu.setActiveMinutes(); // advance to minutes editing
 *   timeMenu.getTime(h, m);      // read result
 * @endcode
 */
class TimeWheel : public TimeWheelBase
{
public:
    TimeWheel();
    virtual ~TimeWheel() {}

    virtual void initialize();

    /**
     * @brief Select 12- or 24-hour mode for the hours wheel.
     *
     * 12-hour shows 1-12 (wrapping) plus an in-screen AM/PM field; 24-hour shows
     * 0-23. Call before setTime(). getTime() always returns the folded 0-23 hour.
     */
    void setFormat(bool is12Hour);

    /** @brief Pre-set the wheels to the given time (@p h is 0-23). */
    void setTime(uint8_t h, uint8_t m);
    /** @brief Read the currently selected time as a 0-23 hour (AM/PM folded in). */
    void getTime(uint8_t& h, uint8_t& m);

    /** @brief Make the hours field the active (editable) one. */
    void setActiveHours();
    /** @brief Make the minutes field the active (editable) one. */
    void setActiveMinutes();
    /** @brief Make the AM/PM field active (12-hour mode only). */
    void setActiveAmPm();

    /** @brief Increment the value of the currently active wheel by one step. */
    void incValue();
    /** @brief Decrement the value of the currently active wheel by one step. */
    void decValue();

protected:
    bool mMinutesActive{};  ///< true while the minutes wheel is active
    bool mAmPmActive{};     ///< true while the AM/PM field is active (12-hour)
    bool mIs12Hour{};       ///< true while the hours wheel shows 1-12
    bool mIsPm{};           ///< current AM/PM selection (12-hour)

    // AM/PM suffix shown to the right of HH:MM in 12-hour mode (mirrors the
    // watch-face display: a single small label baseline-aligned to the digits).
    // Hand-added so no scroll-wheel/designer change is needed; it shows the
    // selected value and turns teal while the field is active.
    touchgfx::TextAreaWithOneWildcard mAmPm;
    static const uint16_t AMPM_SIZE = 3;   // "AM"/"PM" + NUL
    touchgfx::Unicode::UnicodeChar mAmPmBuffer[AMPM_SIZE];

    /** @brief Position the columns for the current format (12h adds AM/PM). */
    void applyLayout();
    /** @brief Recolour/refresh the AM/PM label for the current state. */
    void updateAmPm();
    /**
     * @brief Flip AM/PM when the hour crosses 11<->12 (12-hour mode).
     *
     * Matches a real clock: 11 AM -> 12 PM -> 1 PM, and 11 PM -> 12 AM. Crossing
     * 12<->1 does not change the meridiem. @p beforeIndex / @p afterIndex are
     * hours-wheel item indices (displayed hour = index + 1).
     */
    void maybeFlipMeridiem(int16_t beforeIndex, int16_t afterIndex);

    virtual void hoursWheelUpdateItem(TimeWheelHoursItem& item, int16_t itemIndex) override;
    virtual void hoursWheelUpdateCenterItem(TimeWheelHoursCenterItem& item, int16_t itemIndex) override;
    virtual void minutesWheelUpdateItem(TimeWheelMinutesItem& item, int16_t itemIndex) override;
    virtual void minutesWheelUpdateCenterItem(TimeWheelMinutesCenterItem& item, int16_t itemIndex) override;

    /** @brief Scroll the hours wheel to @p h and update the stored value. */
    void setHours(int16_t h);
    /** @brief Scroll the minutes wheel to @p m and update the stored value. */
    void setMinutes(int16_t m);
};

#endif // TIMEWHEEL_HPP
