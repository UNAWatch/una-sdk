#ifndef MENUINTERVALSRESTVIEW_HPP
#define MENUINTERVALSRESTVIEW_HPP

#include <gui_generated/menuintervalsrest_screen/MenuIntervalsRestViewBase.hpp>
#include <gui/menuintervalsrest_screen/MenuIntervalsRestPresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

class MenuIntervalsRestView : public MenuIntervalsRestViewBase
{
public:
    MenuIntervalsRestView();
    virtual ~MenuIntervalsRestView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setPositionId(uint16_t id);
    uint16_t getPositionId();

    void setRestValue(const Settings::Intervals& iv, bool isImperial);

protected:
    using Menu = App::MenuNav::Root::Intervals::Metric;

    static const uint16_t kBuffSize = 16;
    touchgfx::Unicode::UnicodeChar mTimeTipBuff[kBuffSize] {};
    touchgfx::Unicode::UnicodeChar mDistanceTipBuff[kBuffSize] {};

    MenuItemConfig mItems[Menu::ID_COUNT] {};
    MenuItemConfig mCenterItems[Menu::ID_COUNT] {};

    touchgfx::Callback<MenuIntervalsRestView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuIntervalsRestView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void setupItems();
    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
    void onConfirm();
};

#endif // MENUINTERVALSRESTVIEW_HPP
