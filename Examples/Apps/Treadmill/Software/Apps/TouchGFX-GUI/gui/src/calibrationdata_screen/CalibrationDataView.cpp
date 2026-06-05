#include <gui/calibrationdata_screen/CalibrationDataView.hpp>
#include <SDK/GUI/Button.hpp>
#include <SDK/GUI/Color.hpp>

void CalibrationDataView::setupScreen()
{
    CalibrationDataViewBase::setupScreen();

    title.set(T_TEXT_DATA_UC);

    // L1/L2 page the list; R2 returns. No primary action.
    buttons.setL1(Buttons::WHITE);
    buttons.setL2(Buttons::WHITE);
    buttons.setR1(Buttons::NONE);
    buttons.setR2(Buttons::WHITE);
}

void CalibrationDataView::setData(const Model::CalibrationView& data)
{
    mData = data;
    mTop  = 0;

    // Subtitle reports how many bins are valid, e.g. "8 Bins Ready".
    Unicode::snprintf(binsTextBuffer, BINSTEXT_SIZE, "%u Bins Ready",
                      static_cast<unsigned>(mData.validBins));
    binsText.invalidate();
    // The base set the item count at construction, when mData was still empty,
    // so the rows were populated blank. Toggle the count to force the list to
    // re-request every visible item now that the data is available.
    scrollList.setNumberOfItems(0);
    scrollList.setNumberOfItems(static_cast<int16_t>(mData.binCount));
    scrollList.animateToItem(0, 0);
    scrollList.invalidate();
}

void CalibrationDataView::scrollListUpdateItem(BinListItem& item, int16_t itemIndex)
{
    static const touchgfx::Unicode::UnicodeChar kEmpty[1] = {0};

    if (itemIndex < 0 || itemIndex >= static_cast<int16_t>(mData.binCount)) {
        item.setLabel(kEmpty);
        item.setData(kEmpty, SDK::GUI::Color::WHITE);
        return;
    }

    const Model::CalibrationView::Bin& b = mData.bins[itemIndex];

    touchgfx::Unicode::UnicodeChar label[20];
    touchgfx::Unicode::UnicodeChar pct[8];
    Unicode::snprintf(label, 20, "%u - %u SPM",
        static_cast<unsigned>(b.loSpm), static_cast<unsigned>(b.hiSpm));
    // Unicode::snprintf does not emit a literal "%" from "%%", so append it.
    Unicode::snprintf(pct, 8, "%u", static_cast<unsigned>(b.pct));
    const uint16_t plen = Unicode::strlen(pct);
    if (plen + 1u < 8u) {
        pct[plen]     = static_cast<touchgfx::Unicode::UnicodeChar>('%');
        pct[plen + 1] = 0;
    }

    // 100% green (valid) / 1-99% amber (partial) / 0% red (no data).
    touchgfx::colortype color = (b.pct >= 100) ? SDK::GUI::Color::GREEN_MID
                              : (b.pct > 0)    ? SDK::GUI::Color::YELLOW_DARK
                                               : SDK::GUI::Color::RED;

    item.setLabel(label);
    item.setData(pct, color);
}

void CalibrationDataView::handleKeyEvent(uint8_t key)
{
    const int16_t count  = static_cast<int16_t>(mData.binCount);
    const int16_t maxTop = (count > kVisibleRows) ? static_cast<int16_t>(count - kVisibleRows) : 0;
    const int16_t steps  = static_cast<int16_t>(App::Config::kMenuAnimationSteps);

    // ScrollList::animateToItem only scrolls when the target row is OUTSIDE the
    // current window, bringing it to the nearest edge. To page, target a row
    // beyond the window: the bottom row of the new window (scroll down) or the
    // new top row (scroll up).
    if (key == SDK::GUI::Button::L1) {
        if (mTop > 0) {
            mTop = (mTop > kVisibleRows) ? static_cast<int16_t>(mTop - kVisibleRows) : 0;
            scrollList.animateToItem(mTop, steps);   // new top row -> top edge
        }
    } else if (key == SDK::GUI::Button::L2) {
        if (mTop < maxTop) {
            mTop = static_cast<int16_t>(mTop + kVisibleRows);
            if (mTop > maxTop) mTop = maxTop;
            const int16_t bottom = static_cast<int16_t>(mTop + kVisibleRows - 1);
            scrollList.animateToItem(bottom, steps);  // new bottom row -> bottom edge
        }
    } else if (key == SDK::GUI::Button::R2) {
        application().gotoMenuCalibrationScreenNoTransition();
    }
}
