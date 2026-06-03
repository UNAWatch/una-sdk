#ifndef MENUINTERVALSRUNDISTANCEVIEW_HPP
#define MENUINTERVALSRUNDISTANCEVIEW_HPP

#include <gui_generated/menuintervalsrundistance_screen/MenuIntervalsRunDistanceViewBase.hpp>
#include <gui/menuintervalsrundistance_screen/MenuIntervalsRunDistancePresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

class MenuIntervalsRunDistanceView : public MenuIntervalsRunDistanceViewBase
{
public:
    MenuIntervalsRunDistanceView();
    virtual ~MenuIntervalsRunDistanceView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setDistance(float meters, bool isImperial);

protected:
    using Menu = App::MenuNav::Root::Intervals::DistancePicker;

    enum Stage { STAGE_WHOLE = 0, STAGE_FRAC };

    static const uint16_t kBuffSize = 12;

    Stage    mStage      = STAGE_WHOLE;
    bool     mIsImperial = false;
    uint16_t mWhole      = 0;   ///< Whole km or mi
    uint16_t mFracIdx    = 0;   ///< Fraction index (0..19), value = fracIdx * 0.05 units

    touchgfx::Unicode::UnicodeChar mMainBuff[kBuffSize] {};
    touchgfx::Unicode::UnicodeChar mItemBuff[kBuffSize] {};

    // STAGE_WHOLE: unit label on LEFT,  value on RIGHT side of center
    // STAGE_FRAC:  value on LEFT side,  unit label on RIGHT
    CenterItemLayout mCenterLayoutWhole {};
    CenterItemLayout mCenterLayoutFrac  {};

    touchgfx::Callback<MenuIntervalsRunDistanceView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuIntervalsRunDistanceView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void enterStage(Stage stage);
    void formatValue(touchgfx::Unicode::UnicodeChar* buf, uint16_t whole, uint16_t fracIdx);
    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENUINTERVALSRUNDISTANCEVIEW_HPP
