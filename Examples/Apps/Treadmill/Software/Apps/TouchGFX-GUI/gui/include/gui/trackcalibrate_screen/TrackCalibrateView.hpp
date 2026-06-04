#ifndef TRACKCALIBRATEVIEW_HPP
#define TRACKCALIBRATEVIEW_HPP

#include <gui_generated/trackcalibrate_screen/TrackCalibrateViewBase.hpp>
#include <gui/trackcalibrate_screen/TrackCalibratePresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

/**
 * Post-run "Calibrate & Save" distance entry (§5). A two-stage whole.fraction
 * km/mi picker (same widget as the intervals distance picker), seeded with the
 * estimated distance. R1 confirms (applies the value); R2 skips / steps back.
 */
class TrackCalibrateView : public TrackCalibrateViewBase
{
public:
    TrackCalibrateView();
    virtual ~TrackCalibrateView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /// Seed the picker from a distance in metres, in the user's display units.
    void setDistance(float meters, bool isImperial);

protected:
    using Menu = App::MenuNav::Root::Intervals::CalibratePicker;

    enum Stage { STAGE_WHOLE = 0, STAGE_FRAC };

    static const uint16_t kBuffSize = 12;

    Stage    mStage      = STAGE_WHOLE;
    bool     mIsImperial = false;
    uint16_t mWhole      = 0;   ///< Whole km or mi
    uint16_t mFracIdx    = 0;   ///< Fraction index (0..99), value = fracIdx * 0.01 units

    touchgfx::Unicode::UnicodeChar mMainBuff[kBuffSize] {};
    touchgfx::Unicode::UnicodeChar mItemBuff[kBuffSize] {};

    CenterItemLayout mCenterLayoutWhole {};
    CenterItemLayout mCenterLayoutFrac  {};

    touchgfx::Callback<TrackCalibrateView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<TrackCalibrateView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void enterStage(Stage stage);
    void formatValue(touchgfx::Unicode::UnicodeChar* buf, uint16_t whole, uint16_t fracIdx);
    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // TRACKCALIBRATEVIEW_HPP
