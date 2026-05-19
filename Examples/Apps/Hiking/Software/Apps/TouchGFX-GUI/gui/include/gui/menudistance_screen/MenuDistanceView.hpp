#ifndef MENUDISTANCEVIEW_HPP
#define MENUDISTANCEVIEW_HPP

#include <gui_generated/menudistance_screen/MenuDistanceViewBase.hpp>
#include <gui/menudistance_screen/MenuDistancePresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

class MenuDistanceView : public MenuDistanceViewBase
{
public:
    MenuDistanceView();
    virtual ~MenuDistanceView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setDistanceUnits(Settings::Alerts::Distance::Id id, bool isImperial);

protected:
    using Menu = App::MenuNav::Root::Settings::Alerts::Distance;

    static const uint16_t kBuffSize = 32;

    bool mIsImperial = false;

    touchgfx::Unicode::UnicodeChar mMainBuff[kBuffSize] {};
    touchgfx::Unicode::UnicodeChar mItemBuff[kBuffSize] {};

    touchgfx::Callback<MenuDistanceView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuDistanceView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void formatItem(touchgfx::Unicode::UnicodeChar* buf, int16_t index);
    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENUDISTANCEVIEW_HPP
