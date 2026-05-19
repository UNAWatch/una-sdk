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
 *   timeMenu.setTime(alarm.timeHours, alarm.timeMinutes);
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

    /** @brief Pre-set the wheels to the given time. */
    void setTime(uint8_t h, uint8_t m);
    /** @brief Read the currently selected time. */
    void getTime(uint8_t& h, uint8_t& m);

    /** @brief Show the hours wheel and hide the minutes wheel. */
    void setActiveHours();
    /** @brief Show the minutes wheel and hide the hours wheel. */
    void setActiveMinutes();

    /** @brief Increment the value of the currently active wheel by one step. */
    void incValue();
    /** @brief Decrement the value of the currently active wheel by one step. */
    void decValue();

protected:
    bool mMinutesActive{};  ///< true while the minutes wheel is active

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
