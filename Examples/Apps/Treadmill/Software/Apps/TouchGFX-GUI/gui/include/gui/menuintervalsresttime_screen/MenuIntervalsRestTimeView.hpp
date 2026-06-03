#ifndef MENUINTERVALSRESTTIMEVIEW_HPP
#define MENUINTERVALSRESTTIMEVIEW_HPP

#include <gui_generated/menuintervalsresttime_screen/MenuIntervalsRestTimeViewBase.hpp>
#include <gui/menuintervalsresttime_screen/MenuIntervalsRestTimePresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

class MenuIntervalsRestTimeView : public MenuIntervalsRestTimeViewBase
{
public:
    MenuIntervalsRestTimeView();
    virtual ~MenuIntervalsRestTimeView() {}
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

    CenterItemLayout mCenterLayoutMin {};
    CenterItemLayout mCenterLayoutSec {};

    touchgfx::Callback<MenuIntervalsRestTimeView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuIntervalsRestTimeView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void enterStage(Stage stage);
    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENUINTERVALSRESTTIMEVIEW_HPP
