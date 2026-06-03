#ifndef MENUINTERVALSRUNTIMEVIEW_HPP
#define MENUINTERVALSRUNTIMEVIEW_HPP

#include <gui_generated/menuintervalsruntime_screen/MenuIntervalsRunTimeViewBase.hpp>
#include <gui/menuintervalsruntime_screen/MenuIntervalsRunTimePresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

class MenuIntervalsRunTimeView : public MenuIntervalsRunTimeViewBase
{
public:
    MenuIntervalsRunTimeView();
    virtual ~MenuIntervalsRunTimeView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setTime(uint32_t totalSeconds);

protected:
    using Menu = App::MenuNav::Root::Intervals::TimePicker;

    enum Stage { STAGE_MIN = 0, STAGE_SEC };

    static const uint16_t kBuffSize = 12;

    Stage    mStage   = STAGE_MIN;
    uint16_t mMinutes = 0;
    uint16_t mSeconds = 0;

    touchgfx::Unicode::UnicodeChar mMainBuff[kBuffSize] {};
    touchgfx::Unicode::UnicodeChar mItemBuff[kBuffSize] {};

    // Stage MIN: "min" label on LEFT, time value on RIGHT
    // Stage SEC: time value on LEFT, "sec" label on RIGHT
    CenterItemLayout mCenterLayoutMin {};
    CenterItemLayout mCenterLayoutSec {};

    touchgfx::Callback<MenuIntervalsRunTimeView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuIntervalsRunTimeView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void enterStage(Stage stage);
    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENUINTERVALSRUNTIMEVIEW_HPP
