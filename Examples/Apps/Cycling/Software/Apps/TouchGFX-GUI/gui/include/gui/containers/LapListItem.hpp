#ifndef LAPLISTITEM_HPP
#define LAPLISTITEM_HPP

#include <gui_generated/containers/LapListItemBase.hpp>

/**
 * @brief ScrollList item container displaying a single lap's index, distance, units, time and pace.
 *
 * Fields are split across five independent text widgets to allow precise
 * per-column alignment in the Designer.  All values must be pre-formatted by
 * the caller (typically @c SummaryFaceLaps::scrollListUpdateItem) before being
 * written via the setters below.
 *
 * Passing an empty (zero-terminated) buffer to any setter clears that field --
 * this is used for padding rows beyond the last real lap.
 */
class LapListItem : public LapListItemBase
{
public:
    LapListItem();
    virtual ~LapListItem() {}

    virtual void initialize();

    /** @brief Write the lap index column (e.g. "3."). */
    void setIndex   (const touchgfx::Unicode::UnicodeChar* text);

    /** @brief Write the distance value column (e.g. "3.52"). */
    void setDistance(const touchgfx::Unicode::UnicodeChar* text);

    /** @brief Write the distance unit column (e.g. "km" or "mi"). */
    void setUnits   (const touchgfx::Unicode::UnicodeChar* text);

    /** @brief Write the lap duration column (e.g. "4:12"). */
    void setTime    (const touchgfx::Unicode::UnicodeChar* text);

    /** @brief Write the lap speed column (e.g. "17.9"). */
    void setSpeed    (const touchgfx::Unicode::UnicodeChar* text);

protected:
};

#endif // LAPLISTITEM_HPP
