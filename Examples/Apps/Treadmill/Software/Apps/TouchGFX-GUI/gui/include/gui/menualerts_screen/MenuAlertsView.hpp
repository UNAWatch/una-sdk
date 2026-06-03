#ifndef MENUALERTSVIEW_HPP
#define MENUALERTSVIEW_HPP

#include <gui_generated/menualerts_screen/MenuAlertsViewBase.hpp>
#include <gui/menualerts_screen/MenuAlertsPresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

class MenuAlertsView : public MenuAlertsViewBase
{
public:
    MenuAlertsView();
    virtual ~MenuAlertsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setUnitsImperial(bool isImperial);
    void setDistance(Settings::Alerts::Distance::Id id);
    void setTime(Settings::Alerts::Time::Id id);
    void setPositionId(uint16_t id);
    uint16_t getPositionId();

protected:
    using Menu     = App::MenuNav::Root::Settings::Alerts;
    using Distance = App::MenuNav::Root::Settings::Alerts::Distance;
    using Time     = App::MenuNav::Root::Settings::Alerts::Time;

    static const uint16_t kBuffSize = 32;

    bool                     mUnitsImperial = false;
    Distance::Id             mDistanceId    = Distance::ID_OFF;
    Time::Id                 mTimeId        = Time::ID_OFF;

    touchgfx::Unicode::UnicodeChar mDistanceTipBuff[kBuffSize] {};
    touchgfx::Unicode::UnicodeChar mTimeTipBuff[kBuffSize]     {};
    touchgfx::colortype            mDistanceTipColor           {};
    touchgfx::colortype            mTimeTipColor               {};

    touchgfx::Callback<MenuAlertsView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuAlertsView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);
    void formatTips();

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENUALERTSVIEW_HPP
