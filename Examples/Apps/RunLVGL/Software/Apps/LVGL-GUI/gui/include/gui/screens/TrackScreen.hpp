/**
 ******************************************************************************
 * @file    TrackScreen.hpp
 * @brief   The live activity screen: three faces (totals, lap, status) the
 *          user pages through with L1/L2. R1 opens the action menu, R2 marks
 *          a lap.
 *
 * Port of the Run app's TrackView/TrackPresenter and its TrackFace* containers.
 * The intervals face is M2 work; the face set here is the free-run one.
 ******************************************************************************
 */

#ifndef TRACK_SCREEN_HPP
#define TRACK_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackScreen : public Screen
{
public:
    explicit TrackScreen(Model& model);

    // Screen
    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;

    // ModelListener
    void onTrackData(const Track::Data& data) override;
    void onBatteryLevel(uint8_t level) override;
    void onTime(uint8_t hour, uint8_t minute, uint8_t sec) override;
    void onLapChanged(uint8_t lapEnd) override;
    void onGpsFix(bool acquired) override;
    void onAccessoryStatus(uint8_t state, const char* name) override;

protected:
    void build() override;

private:
    using FaceId = App::MenuNav::TrackView::Id;

    void buildFaceTotal();
    void buildFaceLap();
    void buildFaceStatus();
    void showFace(uint16_t id);
    void setTime(uint8_t h, uint8_t m);
    void updateHrIcon();

    // Face containers (240 x 240, one visible at a time)
    lv_obj_t* mFaceTotal  = nullptr;
    lv_obj_t* mFaceLap    = nullptr;
    lv_obj_t* mFaceStatus = nullptr;

    // Totals face
    lv_obj_t* mPaceValue     = nullptr;
    lv_obj_t* mDistanceValue = nullptr;
    lv_obj_t* mDistanceUnits = nullptr;
    lv_obj_t* mTimerValue    = nullptr;

    // Lap face
    lv_obj_t* mHrValue       = nullptr;
    lv_obj_t* mLapPaceValue  = nullptr;
    lv_obj_t* mLapDistValue  = nullptr;
    lv_obj_t* mLapTimerValue = nullptr;
    std::unique_ptr<Widgets::HeartRateZone> mHrZone;

    // Status face
    lv_obj_t* mDayTime  = nullptr;
    lv_obj_t* mMeridiem = nullptr;
    lv_obj_t* mPercent  = nullptr;
    std::unique_ptr<Widgets::Battery>         mBattery;
    std::unique_ptr<Widgets::SensorStatusRow> mSensorRow;

    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<Widgets::ScrollIndicator> mIndicator;

    uint16_t mFaceId        = FaceId::ID_TRACK1;
    bool     mIsImperial    = false;
    bool     mIs12Hour      = false;
    uint8_t  mHrThresholds[App::Config::kHrThresholdsCount] = {};
    uint8_t  mHrThresholdCount = 0;
    uint8_t  mAccessoryState   = 0;
    uint8_t  mHrSource         = 0;
};

#endif // TRACK_SCREEN_HPP
