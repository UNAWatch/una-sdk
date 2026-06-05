#ifndef BINLISTITEM_HPP
#define BINLISTITEM_HPP

#include <gui_generated/containers/BinListItemBase.hpp>

/**
 * @brief ScrollList item for the Calibration "View Data" screen: one cadence
 *        bin's range (e.g. "80 - 84 SPM") and its fill-toward-validity data
 *        percentage (e.g. "70%"), the latter colour-coded by the caller.
 *
 * Values are pre-formatted by CalibrationDataView::scrollListUpdateItem(). An
 * empty (zero-terminated) buffer clears a field, used for padding rows.
 */
class BinListItem : public BinListItemBase
{
public:
    BinListItem();
    virtual ~BinListItem() {}

    virtual void initialize();

    /** @brief Write the cadence-range column (e.g. "80 - 84 SPM"). */
    void setLabel(const touchgfx::Unicode::UnicodeChar* text);

    /** @brief Write the data-percentage column (e.g. "70%") in @p color. */
    void setData(const touchgfx::Unicode::UnicodeChar* text, touchgfx::colortype color);
};

#endif // BINLISTITEM_HPP
