#ifndef TIMEWHEELHOURSCENTERITEM_HPP
#define TIMEWHEELHOURSCENTERITEM_HPP

#include <gui_generated/containers/TimeWheelHoursCenterItemBase.hpp>

/**
 * @brief Center (selected) item in the hours scroll wheel.
 *
 * Displays the currently selected hour value (00-23) with selection styling.
 */
class TimeWheelHoursCenterItem : public TimeWheelHoursCenterItemBase
{
public:
    TimeWheelHoursCenterItem();
    virtual ~TimeWheelHoursCenterItem() {}

    virtual void initialize() override;

    /** @brief Display @p v formatted as two digits (e.g. 9 -> "09"). */
    void setValue(int16_t v);
};

#endif // TIMEWHEELHOURSCENTERITEM_HPP
