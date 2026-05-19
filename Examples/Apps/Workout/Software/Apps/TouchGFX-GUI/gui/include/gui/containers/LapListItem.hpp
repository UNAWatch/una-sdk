#ifndef LAPLISTITEM_HPP
#define LAPLISTITEM_HPP

#include <gui_generated/containers/LapListItemBase.hpp>

/**
 * @brief ScrollList item rendering one row of the lap table.
 *
 * Columns (left to right): lap index, lap time, lap average HR, lap max HR.
 * Each setter takes pre-formatted text so the caller (typically
 * @c SummaryFaceLaps::scrollListUpdateItem) controls formatting and unit
 * decisions.  Passing an empty (zero-terminated) buffer clears that field --
 * used for padding rows beyond the last real lap.
 */
class LapListItem : public LapListItemBase
{
public:
    LapListItem();
    virtual ~LapListItem() {}

    virtual void initialize();

    /** @brief Lap index column (e.g. "3."). */
    void setIndex(const touchgfx::Unicode::UnicodeChar* text);

    /** @brief Lap time column (e.g. "4:12"). */
    void setTime (const touchgfx::Unicode::UnicodeChar* text);

    /** @brief Lap average HR column (e.g. "128" or "---"). */
    void setAvgHR(const touchgfx::Unicode::UnicodeChar* text);

    /** @brief Lap max HR column (e.g. "145" or "---"). */
    void setMaxHR(const touchgfx::Unicode::UnicodeChar* text);

protected:
};

#endif // LAPLISTITEM_HPP
