#ifndef LAPLISTITEM_HPP
#define LAPLISTITEM_HPP

#include <gui_generated/containers/LapListItemBase.hpp>

/**
 * @class LapListItem
 * @brief One row of the lap list: the lap number and how long that lap took.
 */
class LapListItem : public LapListItemBase
{
public:
    LapListItem();
    virtual ~LapListItem() {}

    virtual void initialize();

    /**
     * @brief Fill the row for a lap.
     * @param number     Lap number as shown to the user, one based.
     * @param durationMs Duration of that lap.
     */
    void setLap(uint8_t number, uint32_t durationMs);

    /**
     * @brief Blank the row.
     *
     * The list holds one more drawable than it can show, so a spare is always
     * bound to an index past the end of the data while scrolling.
     */
    void clear();

protected:
};

#endif // LAPLISTITEM_HPP
