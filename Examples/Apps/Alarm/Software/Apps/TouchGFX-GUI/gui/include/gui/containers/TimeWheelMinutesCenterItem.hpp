#ifndef TIMEWHEELMINUTESCENTERITEM_HPP
#define TIMEWHEELMINUTESCENTERITEM_HPP

#include <gui_generated/containers/TimeWheelMinutesCenterItemBase.hpp>

/**
 * @brief Center (selected) item in the minutes scroll wheel.
 *
 * Displays the currently selected minute value (00-59) with selection styling.
 */
class TimeWheelMinutesCenterItem : public TimeWheelMinutesCenterItemBase
{
public:
    TimeWheelMinutesCenterItem();
    virtual ~TimeWheelMinutesCenterItem() {}

    virtual void initialize() override;

    /** @brief Display @p v formatted as two digits (e.g. 5 -> "05"). */
    void setValue(int16_t v);
};

#endif // TIMEWHEELMINUTESCENTERITEM_HPP
