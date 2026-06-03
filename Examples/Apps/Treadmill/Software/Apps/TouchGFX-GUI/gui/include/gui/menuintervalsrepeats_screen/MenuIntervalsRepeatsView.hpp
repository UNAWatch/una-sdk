#ifndef MENUINTERVALSREPEATSVIEW_HPP
#define MENUINTERVALSREPEATSVIEW_HPP

#include <gui_generated/menuintervalsrepeats_screen/MenuIntervalsRepeatsViewBase.hpp>
#include <gui/menuintervalsrepeats_screen/MenuIntervalsRepeatsPresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

class MenuIntervalsRepeatsView : public MenuIntervalsRepeatsViewBase
{
public:
    MenuIntervalsRepeatsView();
    virtual ~MenuIntervalsRepeatsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setPositionId(uint16_t id);
    uint16_t getPositionId();

protected:
    using Menu = App::MenuNav::Root::Intervals::Repeats;

    static const uint16_t kBuffSize = 8;

    touchgfx::Unicode::UnicodeChar mMainBuff[kBuffSize] {};
    touchgfx::Unicode::UnicodeChar mItemBuff[kBuffSize] {};

    touchgfx::Callback<MenuIntervalsRepeatsView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuIntervalsRepeatsView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void formatItem(touchgfx::Unicode::UnicodeChar* buf, int16_t index);
    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENUINTERVALSREPEATSVIEW_HPP
