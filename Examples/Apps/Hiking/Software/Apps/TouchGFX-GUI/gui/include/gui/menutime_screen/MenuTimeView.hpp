#ifndef MENUTIMEVIEW_HPP
#define MENUTIMEVIEW_HPP

#include <gui_generated/menutime_screen/MenuTimeViewBase.hpp>
#include <gui/menutime_screen/MenuTimePresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

class MenuTimeView : public MenuTimeViewBase
{
public:
    MenuTimeView();
    virtual ~MenuTimeView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setTime(Settings::Alerts::Time::Id id);

protected:
    using Menu = App::MenuNav::Root::Settings::Alerts::Time;

    static const uint16_t kBuffSize = 32;

    touchgfx::Unicode::UnicodeChar mMainBuff[kBuffSize] {};
    touchgfx::Unicode::UnicodeChar mItemBuff[kBuffSize] {};

    touchgfx::Callback<MenuTimeView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuTimeView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENUTIMEVIEW_HPP
