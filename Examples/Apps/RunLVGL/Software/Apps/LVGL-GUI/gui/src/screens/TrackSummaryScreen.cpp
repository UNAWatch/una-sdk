/**
 ******************************************************************************
 * @file    TrackSummaryScreen.cpp
 * @brief   Activity summary (see TrackSummaryScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/TrackSummaryScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"
#include "gui/Format.hpp"
#include "gui/Strings.hpp"

using namespace SDK::GUI;
using F = Theme::Font;

TrackSummaryScreen::TrackSummaryScreen(Model& model)
    : Screen(model)
{
}

void TrackSummaryScreen::build()
{
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::NONE, Widgets::Buttons::NONE);
    mIndicator = std::make_unique<Widgets::ScrollIndicator>(mRoot, Widgets::ScrollIndicator::kSmall);
    mIndicator->setCount(3);

    buildFaceLaps();
    buildFaceHeartRate();
    buildFaceOverview();
    buildFaceMap();
}

void TrackSummaryScreen::buildFaceMap()
{
    lv_obj_t* f = mFaces[FACE_MAP] = Theme::container(mRoot, 0, 0, 240, 240);
    mMapUnits    = Theme::label(f, F::Medium18, Strings::kKm, 178, 73, 45, LV_TEXT_ALIGN_LEFT);
    mMapDistance = Theme::label(f, F::SemiBold40, Strings::kNoValue, 60, 51, 120);
    Theme::image(f, &img_runningman_30x30, 33, 62);
    mMap = std::make_unique<Widgets::Map>(f, 45, 90);
    mTitles[FACE_MAP] = std::make_unique<Widgets::Title>(f, "MAP");
}

void TrackSummaryScreen::buildFaceOverview()
{
    lv_obj_t* f = mFaces[FACE_OVERVIEW] = Theme::container(mRoot, 0, 0, 240, 240);
    mTimer = Theme::label(f, F::SemiBold25, "0:00:00", 95, 173, 117);
    Theme::label(f, F::Medium18, "TIMER", 43, 179, 51);
    Theme::hline(f, 25, 166, 190);
    // Wider than the design's 78 px box (same centre) so "22:00" fits.
    mAvgPace = Theme::label(f, F::SemiBold25, Strings::kNoValue, 121, 127, 100);
    Theme::label(f, F::Medium18, "AVG PACE", 39, 133, 100);
    Theme::hline(f, 25, 120, 190);
    mOverviewUnits    = Theme::label(f, F::Medium18, Strings::kKm, 178, 73, 45, LV_TEXT_ALIGN_LEFT);
    mOverviewDistance = Theme::label(f, F::SemiBold40, Strings::kNoValue, 60, 51, 120);
    Theme::image(f, &img_runningman_30x30, 33, 62);
    mTitles[FACE_OVERVIEW] = std::make_unique<Widgets::Title>(f, "SUMMARY");
}

void TrackSummaryScreen::buildFaceHeartRate()
{
    lv_obj_t* f = mFaces[FACE_HEARTRATE] = Theme::container(mRoot, 0, 0, 240, 240);
    Theme::label(f, F::SemiBold20, "MAX HR", 81, 118, 78);
    mMaxHr = Theme::label(f, F::SemiBold40, Strings::kNoValue, 70, 77, 100);
    Theme::hline(f, 25, 147, 190);
    Theme::label(f, F::SemiBold20, "AVG HR", 81, 191, 75);
    mAvgHr = Theme::label(f, F::SemiBold40, Strings::kNoValue, 70, 150, 100);
    Theme::image(f, &img_heart_30x30, 105, 48);
    mTitles[FACE_HEARTRATE] = std::make_unique<Widgets::Title>(f, "HEART RATE");
}

void TrackSummaryScreen::buildFaceLaps()
{
    lv_obj_t* f = mFaces[FACE_LAPS] = Theme::container(mRoot, 0, 0, 240, 240);
    mLapsCount = Theme::label(f, F::SemiBold30, "0 LAPS", 20, 40, 200);
    Theme::image(f, &img_clock_16x19, 54, 75);
    mLapsTotal = Theme::label(f, F::Regular18, "Total: 0:00:00", 73, 75, 118);
    Theme::hline(f, 25, 100, 190);
    Theme::label(f, F::Regular16, "Distance", 33, 104, 80, LV_TEXT_ALIGN_LEFT);
    Theme::label(f, F::Regular16, "Time", 119, 104, 45, LV_TEXT_ALIGN_LEFT, Color::CYAN_PURE);
    Theme::label(f, F::Regular16, "Pace", 169, 104, 44, LV_TEXT_ALIGN_LEFT);

    // Five visible rows, refilled on each page, like the TouchGFX ScrollList's
    // recycled drawables. Columns: index, distance, units, time, pace.
    lv_obj_t* list = Theme::container(f, 25, 126, 190, 90);
    for (uint8_t r = 0; r < kLapVisible; ++r) {
        const int32_t y = r * 18;
        mLapRows[r][0] = Theme::label(list, F::Regular14, "", 0,   y, 21, LV_TEXT_ALIGN_RIGHT);
        mLapRows[r][1] = Theme::label(list, F::Regular14, "", 21,  y, 40, LV_TEXT_ALIGN_RIGHT);
        mLapRows[r][2] = Theme::label(list, F::Regular14, "", 64,  y, 28, LV_TEXT_ALIGN_LEFT);
        mLapRows[r][3] = Theme::label(list, F::Regular14, "", 98,  y, 45, LV_TEXT_ALIGN_LEFT, Color::CYAN_PURE);
        mLapRows[r][4] = Theme::label(list, F::Regular14, "", 145, y, 45, LV_TEXT_ALIGN_LEFT);
    }
    mTitles[FACE_LAPS] = std::make_unique<Widgets::Title>(f, "LAPS");
}

void TrackSummaryScreen::onShow()
{
    mModel.resetIdleTimer();
    mIsImperial = mModel.isUnitsImperial();
    mPaused     = mModel.isTrackPaused();
    if (mModel.isTrackSummaryAvailable()) {
        setSummary(mModel.getTrackSummary());
    }
    // While paused the way back is R2 (to the action menu); after a save R1 leaves.
    if (mPaused) {
        mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                      Widgets::Buttons::NONE, Widgets::Buttons::AMBER);
    } else {
        mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                      Widgets::Buttons::AMBER, Widgets::Buttons::NONE);
    }
    showFace(FACE_MAP);
}

void TrackSummaryScreen::setSummary(const ActivitySummary& s)
{
    char buf[24];
    const float dist = Fmt::distUnits(s.distance, mIsImperial);
    Fmt::distanceTotal(buf, sizeof(buf), dist);
    lv_label_set_text(mMapDistance, buf);
    lv_label_set_text(mOverviewDistance, buf);
    lv_label_set_text(mMapUnits, Fmt::units(mIsImperial));
    lv_label_set_text(mOverviewUnits, Fmt::units(mIsImperial));

    Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(s.paceAvg, mIsImperial));
    lv_label_set_text(mAvgPace, buf);
    Fmt::hms(buf, sizeof(buf), s.time);
    lv_label_set_text(mTimer, buf);

    Fmt::heartRate(buf, sizeof(buf), s.hrMax);
    lv_label_set_text(mMaxHr, buf);
    Fmt::heartRate(buf, sizeof(buf), s.hrAvg);
    lv_label_set_text(mAvgHr, buf);

    mMap->setMap(s.map);

    mLaps = &s.laps;
    lv_label_set_text_fmt(mLapsCount, "%lu LAPS", static_cast<unsigned long>(s.laps.size()));
    std::time_t total = 0;
    for (const auto& lap : s.laps) {
        total += lap.duration;
    }
    char t[16];
    Fmt::hms(t, sizeof(t), total);
    snprintf(buf, sizeof(buf), "Total: %s", t);
    lv_label_set_text(mLapsTotal, buf);

    mLapPage  = 0;
    mLapPages = s.laps.empty() ? 0 : static_cast<uint8_t>((s.laps.size() + kLapPageSize - 1) / kLapPageSize);
    mIndicator->setCount(static_cast<uint16_t>(3 + mLapPages));
    fillLapRows();
}

void TrackSummaryScreen::fillLapRows()
{
    const size_t first = static_cast<size_t>(mLapPage) * kLapPageSize;
    for (uint8_t r = 0; r < kLapVisible; ++r) {
        const size_t idx = first + r;
        if (!mLaps || idx >= mLaps->size()) {
            for (uint8_t c = 0; c < kLapColumns; ++c) {
                lv_label_set_text(mLapRows[r][c], "");
            }
            continue;
        }
        const LapSummary& lap = (*mLaps)[idx];
        char buf[16];
        snprintf(buf, sizeof(buf), "%lu.", static_cast<unsigned long>(idx + 1));
        lv_label_set_text(mLapRows[r][0], buf);
        const float dist = Fmt::distUnits(lap.distance, mIsImperial);
        Fmt::fixed(buf, sizeof(buf), dist, dist < 100.0f ? 2 : 1);
        lv_label_set_text(mLapRows[r][1], buf);
        lv_label_set_text(mLapRows[r][2], Fmt::units(mIsImperial));
        Fmt::shortTime(buf, sizeof(buf), lap.duration);
        lv_label_set_text(mLapRows[r][3], buf);
        Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(lap.paceAvg, mIsImperial));
        lv_label_set_text(mLapRows[r][4], buf);
    }
}

void TrackSummaryScreen::showFace(uint8_t face)
{
    mFace = face;
    for (uint8_t i = 0; i < 4; ++i) {
        if (i == face) {
            lv_obj_remove_flag(mFaces[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(mFaces[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    updateIndicator();
}

void TrackSummaryScreen::updateIndicator()
{
    const uint8_t id = mFace < FACE_LAPS ? mFace : static_cast<uint8_t>(FACE_LAPS + mLapPage);
    mIndicator->setActive(id);
}

void TrackSummaryScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L2:
            if (mFace == FACE_LAPS) {
                if (mLapPage + 1 < mLapPages) {
                    ++mLapPage;
                    fillLapRows();
                    updateIndicator();
                }
            } else if (mFace < FACE_HEARTRATE || mLapPages > 0) {
                showFace(mFace + 1);
            }
            break;

        case Btn::L1:
            if (mFace == FACE_LAPS) {
                if (mLapPage > 0) {
                    --mLapPage;
                    fillLapRows();
                    updateIndicator();
                } else {
                    showFace(mFace - 1);
                }
            } else if (mFace > FACE_MAP) {
                showFace(mFace - 1);
            }
            break;

        case Btn::R1:
            if (!mPaused) {
                backToTrack();
            }
            break;

        case Btn::R2:
            if (mPaused) {
                backToTrack();
            }
            break;

        default:
            break;
    }
}

void TrackSummaryScreen::backToTrack()
{
    if (mModel.isTrackPaused()) {
        ScreenManager::instance().goTo(ScreenId::TrackAction);
    } else {
        mModel.exitApp();
    }
}
