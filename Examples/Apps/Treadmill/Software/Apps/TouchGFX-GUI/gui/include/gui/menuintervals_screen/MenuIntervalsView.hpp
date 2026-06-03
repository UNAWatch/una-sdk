#ifndef MENUINTERVALSVIEW_HPP
#define MENUINTERVALSVIEW_HPP

#include <gui_generated/menuintervals_screen/MenuIntervalsViewBase.hpp>
#include <gui/menuintervals_screen/MenuIntervalsPresenter.hpp>

class MenuIntervalsView : public MenuIntervalsViewBase
{
public:
    MenuIntervalsView();
    virtual ~MenuIntervalsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setIntervals(const Settings::Intervals & inervals, bool isImperial);

    void setPositionId(uint16_t id);
    uint16_t getPositionId();

protected:
    using Menu = App::MenuNav::Root::Intervals;

    static const uint16_t kBuffSize = 16;
    touchgfx::Unicode::UnicodeChar mRepeatsTipBuff[kBuffSize] {};
    touchgfx::Unicode::UnicodeChar mRunTipBuff[kBuffSize] {};
    touchgfx::Unicode::UnicodeChar mRestTipBuff[kBuffSize] {};

    MenuItemConfig mItems[Menu::ID_COUNT] {};
    MenuItemConfig mCenterItems[Menu::ID_COUNT] {};

    touchgfx::Callback<MenuIntervalsView, MainMenuItem&, int16_t> mUpdateItemCb;
    touchgfx::Callback<MenuIntervalsView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void setupItems();
    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
    void onConfirm();
    void updateToggle(int16_t index, bool newState);
};

#endif // MENUINTERVALSVIEW_HPP
