#ifndef TIMEWHEELHOURSITEM_HPP
#define TIMEWHEELHOURSITEM_HPP

#include <gui_generated/containers/TimeWheelHoursItemBase.hpp>

/**
 * @brief Non-centered (surrounding) item in the hours scroll wheel.
 *
 * Displays a two-digit hour value (00-23).
 */
class TimeWheelHoursItem : public TimeWheelHoursItemBase
{
public:
    TimeWheelHoursItem();
    virtual ~TimeWheelHoursItem() {}

    virtual void initialize() override;

    /** @brief Display @p v formatted as two digits (e.g. 9 -> "09"). */
    void setValue(int16_t v);
};

#endif // TIMEWHEELHOURSITEM_HPP
