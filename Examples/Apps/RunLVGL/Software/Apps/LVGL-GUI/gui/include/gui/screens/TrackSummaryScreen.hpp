/**
 ******************************************************************************
 * @file    TrackSummaryScreen.hpp
 * @brief   Activity summary: map, overview, heart rate and a paged lap list,
 *          browsed with L1/L2.
 *
 * Port of the Run app's TrackSummaryView and its SummaryFace* containers.
 * Reached from the action menu while paused (R2 returns there) or after a
 * save (R1 leaves the app).
 ******************************************************************************
 */

#ifndef TRACK_SUMMARY_SCREEN_HPP
#define TRACK_SUMMARY_SCREEN_HPP

#include <memory>
#include <vector>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class TrackSummaryScreen : public Screen
{
public:
    explicit TrackSummaryScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    enum Face : uint8_t { FACE_MAP = 0, FACE_OVERVIEW, FACE_HEARTRATE, FACE_LAPS };

    static constexpr uint8_t kLapPageSize = 3;   ///< rows advanced per L1/L2 press
    static constexpr uint8_t kLapVisible  = 5;   ///< rows on screen at once
    static constexpr uint8_t kLapColumns  = 5;   ///< index, distance, units, time, pace

    void buildFaceMap();
    void buildFaceOverview();
    void buildFaceHeartRate();
    void buildFaceLaps();

    void setSummary(const ActivitySummary& s);
    void showFace(uint8_t face);
    void updateIndicator();
    void fillLapRows();
    void backToTrack();

    lv_obj_t* mFaces[4] = {};

    // Map + overview headers
    lv_obj_t* mMapDistance      = nullptr;
    lv_obj_t* mMapUnits         = nullptr;
    lv_obj_t* mOverviewDistance = nullptr;
    lv_obj_t* mOverviewUnits    = nullptr;
    lv_obj_t* mAvgPace          = nullptr;
    lv_obj_t* mTimer            = nullptr;
    std::unique_ptr<Widgets::Map> mMap;

    // Heart rate
    lv_obj_t* mMaxHr = nullptr;
    lv_obj_t* mAvgHr = nullptr;

    // Laps
    lv_obj_t* mLapsCount = nullptr;
    lv_obj_t* mLapsTotal = nullptr;
    lv_obj_t* mLapRows[kLapVisible][kLapColumns] = {};
    const std::vector<LapSummary>* mLaps = nullptr;

    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<Widgets::ScrollIndicator> mIndicator;
    std::unique_ptr<Widgets::Title>           mTitles[4];

    uint8_t mFace       = FACE_MAP;
    bool    mIsImperial = false;
    bool    mPaused     = false;
    uint8_t mLapPages   = 0;
    uint8_t mLapPage    = 0;
};

#endif // TRACK_SUMMARY_SCREEN_HPP
