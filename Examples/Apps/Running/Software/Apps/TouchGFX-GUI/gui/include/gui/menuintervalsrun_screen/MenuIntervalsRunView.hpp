#ifndef MENUINTERVALSRUNVIEW_HPP
#define MENUINTERVALSRUNVIEW_HPP

#include <gui_generated/menuintervalsrun_screen/MenuIntervalsRunViewBase.hpp>
#include <gui/menuintervalsrun_screen/MenuIntervalsRunPresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

class MenuIntervalsRunView : public MenuIntervalsRunViewBase
{
public:
    MenuIntervalsRunView();
    virtual ~MenuIntervalsRunView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setPositionId(uint16_t id);
    uint16_t getPositionId();

    void setRunValue(const Settings::Intervals& iv, bool isImperial);

protected:
    using Menu = App::MenuNav::Root::Intervals::Metric;

    static const uint16_t kBuffSize = 16;
    touchgfx::Unicode::UnicodeChar mTimeTipBuff[kBuffSize] {};
    touchgfx::Unicode::UnicodeChar mDistanceTipBuff[kBuffSize] {};

    MenuItemConfig mItems[Menu::ID_COUNT] {};
    MenuItemConfig mCenterItems[Menu::ID_COUNT] {};

    touchgfx::Callback<MenuIntervalsRunView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuIntervalsRunView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void setupItems();
    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
    void onConfirm();
};

#endif // MENUINTERVALSRUNVIEW_HPP
