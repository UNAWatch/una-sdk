#ifndef SUMMARYFACELAPS_HPP
#define SUMMARYFACELAPS_HPP

#include <gui_generated/containers/SummaryFaceLapsBase.hpp>
#include <vector>
#include <ctime>
#include "ActivitySummary.hpp"

/**
 * @brief Summary face showing individual lap metrics in a paginated scrollable list.
 *
 * One of four swipeable faces on the TrackSummary screen.  Displays up to
 * @c kVisible rows at a time; the L1/L2 buttons page through the list in
 * increments of @c kPageSize.  Unit conversion is performed internally so
 * callers only need to supply the raw @c LapSummary vector.
 *
 * Padding items are appended to the underlying ScrollList so that every page
 * transition always selects an off-screen item, guaranteeing the list scrolls
 * by exactly @c kPageSize rows per button press.
 *
 * Unlike other containers that receive pre-converted values, this container
 * stores a reference to the raw lap vector and formats each row on demand in
 * scrollListUpdateItem(), filling the four columns: lap index, lap time,
 * lap average HR, lap max HR.
 */
class SummaryFaceLaps : public SummaryFaceLapsBase
{
public:
    static constexpr uint8_t kPageSize = 3; ///< Rows advanced per L1/L2 press
    static constexpr uint8_t kVisible  = 5; ///< Rows visible on screen at once

    SummaryFaceLaps();
    virtual ~SummaryFaceLaps() {}

    virtual void initialize();

    /**
     * @brief Populate the list with lap data and reset to the first page.
     * @param laps Reference to the lap vector owned by the activity summary.
     *             The reference must remain valid for the lifetime of this container.
     */
    void setLaps(const std::vector<LapSummary>& laps);

    /** @brief Return true if there is at least one more page below the current one. */
    bool canScrollDown() const;

    /** @brief Return true if there is at least one more page above the current one. */
    bool canScrollUp()   const;

    /** @brief Advance to the next page and animate the list. No-op if already on the last page. */
    void scrollDown();

    /** @brief Return to the previous page and animate the list. No-op if already on the first page. */
    void scrollUp();

    /** @brief Return the zero-based index of the currently displayed page. */
    uint8_t getScrollPage() const { return mCurrentPage; }

    /** @brief Return the total number of pages for the current lap vector. */
    uint8_t getPageCount()  const { return mNumPages; }

protected:
    /** @brief ScrollList callback -- formats and writes data for item at @p itemIndex. */
    virtual void scrollListUpdateItem(LapListItem& item, int16_t itemIndex) override;

    const std::vector<LapSummary>* mLaps        = nullptr;
    uint8_t mCurrentPage = 0;
    uint8_t mNumPages    = 0;
};

#endif // SUMMARYFACELAPS_HPP
