#ifndef MENURUNHRLISTVIEW_HPP
#define MENURUNHRLISTVIEW_HPP

#include <gui_generated/menurunhrlist_screen/MenuRunHrListViewBase.hpp>
#include <gui/menurunhrlist_screen/MenuRunHrListPresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

/**
 * HR High or HR Low (Model::runAlertField()): a list of Off, then the bpm
 * values RunAlerts::pickerBounds() allows (40 to 220, narrowed by the other
 * limit), as Intervals' Repeats list starts with Open. Rows are formatted as
 * they come into view.
 */
class MenuRunHrListView : public MenuRunHrListViewBase
{
public:
    MenuRunHrListView();
    virtual ~MenuRunHrListView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setGpsFix(bool state);
    void setAccessoryStatus(uint8_t state, const char* name);
    /// Rows for [@p lo, @p hi] (none when @p empty), opening on @p seed.
    void setList(RunAlerts::Field field, bool empty, uint32_t lo, uint32_t hi, uint32_t seed);

protected:
    static const uint16_t kBuffSize = 10;

    bool     mGpsFix = false;
    uint32_t mMin    = 0;
    int16_t  mRows   = 1;   ///< Off, then the values

    touchgfx::Unicode::UnicodeChar mMainBuff[kBuffSize] {};
    touchgfx::Unicode::UnicodeChar mItemBuff[kBuffSize] {};

    touchgfx::Callback<MenuRunHrListView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuRunHrListView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void formatItem(touchgfx::Unicode::UnicodeChar* buf, int16_t index);
    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENURUNHRLISTVIEW_HPP
