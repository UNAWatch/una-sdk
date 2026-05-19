#ifndef TIMEWHEELMINUTESITEM_HPP
#define TIMEWHEELMINUTESITEM_HPP

#include <gui_generated/containers/TimeWheelMinutesItemBase.hpp>

/**
 * @brief Non-centered (surrounding) item in the minutes scroll wheel.
 *
 * Displays a two-digit minute value (00-59).
 */
class TimeWheelMinutesItem : public TimeWheelMinutesItemBase
{
public:
    TimeWheelMinutesItem();
    virtual ~TimeWheelMinutesItem() {}

    virtual void initialize() override;

    /** @brief Display @p v formatted as two digits (e.g. 5 -> "05"). */
    void setValue(int16_t v);
};

#endif // TIMEWHEELMINUTESITEM_HPP
