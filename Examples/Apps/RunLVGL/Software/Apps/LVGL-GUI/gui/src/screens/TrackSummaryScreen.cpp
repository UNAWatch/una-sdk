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

#include "LapCompare.hpp"

using namespace SDK::GUI;
using F = Theme::Font;

namespace
{
// The lap pace chart: bars from kBarMinH (the fastest lap) to kBarsH (the
// slowest), the others between by their pace; the two lines under it.
constexpr int32_t kBarsX     = 37;
constexpr int32_t kBarsY     = 68;
constexpr int32_t kBarsW     = 166;
constexpr int32_t kBarsH     = 86;
constexpr int32_t kBarMinH   = 40;
constexpr int32_t kBarGap    = 3;
constexpr int32_t kFastLineY = 162;
constexpr int32_t kSlowLineY = 184;
} // namespace

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
    buildFaceChart();
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

void TrackSummaryScreen::buildFaceChart()
{
    // One object draws every bar.
    lv_obj_t* f = mFaces[FACE_CHART] = Theme::container(mRoot, 0, 0, 240, 240);
    mBars = Theme::container(f, kBarsX, kBarsY, kBarsW, kBarsH);
    lv_obj_add_event_cb(mBars, &TrackSummaryScreen::drawBars, LV_EVENT_DRAW_MAIN, this);
    mFastLine = Theme::label(f, F::Regular16, "", 25, kFastLineY, 190);
    mSlowLine = Theme::label(f, F::Regular14, "", 30, kSlowLineY, 180);
    mTitles[FACE_CHART] = std::make_unique<Widgets::Title>(f, "LAP PACE");
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
    setChart(s.laps);
    mIndicator->setCount(static_cast<uint16_t>(3 + (mHasChart ? 1 : 0) + mLapPages));
    fillLapRows();
}

void TrackSummaryScreen::setChart(const std::vector<LapSummary>& laps)
{
    // The last kMaxBars laps long enough to chart, by pace (LapCompare), each
    // with its number in the lap list.
    const size_t n = laps.size();
    size_t charted = 0;
    for (size_t i = 0; i < n; ++i) {
        charted += laps[i].distance >= LapCompare::kMinChartM && laps[i].paceAvg > 0.0f ? 1u : 0u;
    }
    const size_t skip = charted > kMaxBars ? charted - kMaxBars : 0;
    size_t seen = 0;
    mBarCount = 0;
    for (size_t i = 0; i < n && mBarCount < kMaxBars; ++i) {
        const LapSummary& l = laps[i];
        if (!(l.distance >= LapCompare::kMinChartM && l.paceAvg > 0.0f) || seen++ < skip) {
            continue;
        }
        mBarNumber[mBarCount] = static_cast<uint16_t>(i + 1);
        mBarPace[mBarCount++] = l.paceAvg;
    }
    const LapCompare::Extremes e = LapCompare::extremes(
        mBarCount, [this](size_t i) { return mBarPace[i]; }, [](size_t) { return LapCompare::kMinChartM; });
    mBarFast  = e.fastest;
    mBarSlow  = e.slowest;
    mHasChart = e.fastest >= 0;
    char buf[40];
    char p[16];
    const char* unit = mIsImperial ? "/mi" : "/km";
    if (mHasChart) {
        Fmt::pace(p, sizeof(p), Fmt::paceUnits(mBarPace[mBarFast], mIsImperial));
        snprintf(buf, sizeof(buf), "Fastest lap %u  %s %s", static_cast<unsigned>(mBarNumber[mBarFast]), p, unit);
        lv_label_set_text(mFastLine, buf);
        Fmt::pace(p, sizeof(p), Fmt::paceUnits(mBarPace[mBarSlow], mIsImperial));
        snprintf(buf, sizeof(buf), "Slowest lap %u  %s %s", static_cast<unsigned>(mBarNumber[mBarSlow]), p, unit);
        lv_label_set_text(mSlowLine, buf);
    }
    lv_obj_invalidate(mBars);
}

void TrackSummaryScreen::drawBars(lv_event_t* e)
{
    auto* self = static_cast<TrackSummaryScreen*>(lv_event_get_user_data(e));
    if (!self->mHasChart) {
        return;
    }
    lv_layer_t* layer = lv_event_get_layer(e);
    lv_area_t box;
    lv_obj_get_coords(self->mBars, &box);
    const int32_t n     = self->mBarCount;
    const int32_t pitch = kBarsW / (n < 6 ? 6 : n);
    const int32_t w     = pitch > kBarGap + 1 ? pitch - kBarGap : 1;
    const int32_t x0    = box.x1 + (kBarsW - pitch * n + kBarGap) / 2;
    const float   lo    = self->mBarPace[self->mBarFast];
    const float   hi    = self->mBarPace[self->mBarSlow];
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    for (int32_t i = 0; i < n; ++i) {
        const float   p = self->mBarPace[i];
        const int32_t h = hi > lo ? kBarMinH + static_cast<int32_t>((p - lo) * (kBarsH - kBarMinH) / (hi - lo) + 0.5f)
                                  : kBarsH;
        const uint32_t color = i == self->mBarFast ? Color::LIME_SOFT
                             : i == self->mBarSlow ? Color::SALMON : Color::TEAL;
        d.bg_color = Theme::rgb(color);
        lv_area_t a;
        a.x1 = x0 + i * pitch;
        a.x2 = a.x1 + w - 1;
        a.y2 = box.y2;
        a.y1 = box.y2 - h + 1;
        lv_draw_rect(layer, &d, &a);
    }
}

uint8_t TrackSummaryScreen::nextFace(uint8_t face) const
{
    if (face < FACE_HEARTRATE) {
        return static_cast<uint8_t>(face + 1);
    }
    if (face == FACE_HEARTRATE && mHasChart) {
        return FACE_CHART;
    }
    if (face < FACE_LAPS && mLapPages > 0) {
        return FACE_LAPS;
    }
    return FACE_COUNT;
}

uint8_t TrackSummaryScreen::prevFace(uint8_t face) const
{
    if (face == FACE_LAPS) {
        return mHasChart ? FACE_CHART : FACE_HEARTRATE;
    }
    return face > FACE_MAP ? static_cast<uint8_t>(face - 1) : FACE_COUNT;
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
    for (uint8_t i = 0; i < FACE_COUNT; ++i) {
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
    const uint8_t chart = mHasChart ? 1 : 0;
    const uint8_t id    = mFace <= FACE_HEARTRATE ? mFace
                        : mFace == FACE_CHART     ? static_cast<uint8_t>(FACE_CHART)
                                                  : static_cast<uint8_t>(FACE_CHART + chart + mLapPage);
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
            } else if (nextFace(mFace) != FACE_COUNT) {
                showFace(nextFace(mFace));
            }
            break;

        case Btn::L1:
            if (mFace == FACE_LAPS) {
                if (mLapPage > 0) {
                    --mLapPage;
                    fillLapRows();
                    updateIndicator();
                } else {
                    showFace(prevFace(mFace));
                }
            } else if (prevFace(mFace) != FACE_COUNT) {
                showFace(prevFace(mFace));
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
