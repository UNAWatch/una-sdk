#ifndef MENUINTERVALSRESTDISTANCEVIEW_HPP
#define MENUINTERVALSRESTDISTANCEVIEW_HPP

#include <gui_generated/menuintervalsrestdistance_screen/MenuIntervalsRestDistanceViewBase.hpp>
#include <gui/menuintervalsrestdistance_screen/MenuIntervalsRestDistancePresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

class MenuIntervalsRestDistanceView : public MenuIntervalsRestDistanceViewBase
{
public:
    MenuIntervalsRestDistanceView();
    virtual ~MenuIntervalsRestDistanceView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setDistance(float meters, bool isImperial);

protected:
    using Menu = App::MenuNav::Root::Intervals::DistancePicker;

    enum Stage { STAGE_WHOLE = 0, STAGE_FRAC };

    static const uint16_t kBuffSize = 12;

    Stage    mStage      = STAGE_WHOLE;
    bool     mIsImperial = false;
    uint16_t mWhole      = 0;
    uint16_t mFracIdx    = 0;

    touchgfx::Unicode::UnicodeChar mMainBuff[kBuffSize] {};
    touchgfx::Unicode::UnicodeChar mItemBuff[kBuffSize] {};

    CenterItemLayout mCenterLayoutWhole {};
    CenterItemLayout mCenterLayoutFrac  {};

    touchgfx::Callback<MenuIntervalsRestDistanceView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuIntervalsRestDistanceView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void enterStage(Stage stage);
    void formatValue(touchgfx::Unicode::UnicodeChar* buf, uint16_t whole, uint16_t fracIdx);
    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENUINTERVALSRESTDISTANCEVIEW_HPP
