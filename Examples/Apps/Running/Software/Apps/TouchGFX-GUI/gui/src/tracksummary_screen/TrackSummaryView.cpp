#include <gui/tracksummary_screen/TrackSummaryView.hpp>
#include <SDK/GUI/Button.hpp>
#include <SDK/Utils/Utils.hpp>

TrackSummaryView::TrackSummaryView()
{
}

void TrackSummaryView::setupScreen()
{
    TrackSummaryViewBase::setupScreen();

    summaryFaceChart.initialize();
    summaryFaceChart.setXY(0, 0);
    summaryFaceChart.setVisible(false);
    add(summaryFaceChart);

    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::NONE);
    buttons.setR2(Buttons::NONE);

    scrollIndicator.setConfig(ScrollIndicator::kSmall);
    scrollIndicator.setCount(3);
    scrollIndicator.setActiveId(FACE_MAP);

    mCurrentFace = FACE_MAP;
    updateFace();
}

void TrackSummaryView::tearDownScreen()
{
    TrackSummaryViewBase::tearDownScreen();
}

void TrackSummaryView::setSummary(const ActivitySummary& s, bool isImperial, bool isPaused)
{
    mIsImperial    = isImperial;
    mTrackIsPaused = isPaused;

    auto distConv = [isImperial](float metres) -> float {
        const float km = metres / 1000.0f;
        return isImperial ? SDK::Utils::kmToMiles(km) : km;
    };

    auto paceConv = [isImperial](float secPerM) -> float {
        if (secPerM < 1e-6f) return 0.0f;
        const float secPerKm = secPerM * 1000.0f;
        return isImperial ? secPerKm / SDK::Utils::kmToMiles(1.0f) : secPerKm;
    };

    const float dist = distConv(s.distance);
    summaryFaceMap.setDistance(dist, isImperial);
    summaryFaceOverview.setDistance(dist, isImperial);
    summaryFaceOverview.setAvgPace(paceConv(s.paceAvg));
    summaryFaceOverview.setTimer(s.time);
    summaryFaceHeartRate.setMaxHR(s.hrMax);
    summaryFaceHeartRate.setAvgHR(s.hrAvg);
    summaryFaceMap.setMap(s.map);

    summaryFaceLaps.setLaps(s.laps, isImperial);
    mLapsPageCount = summaryFaceLaps.getPageCount();
    summaryFaceChart.setLaps(s.laps, isImperial);
    scrollIndicator.setCount(3 + (summaryFaceChart.hasChart() ? 1 : 0) + mLapsPageCount);

    if (isPaused) {
        buttons.setR1(Buttons::NONE);
        buttons.setR2(Buttons::AMBER);
    } else {
        buttons.setR1(Buttons::AMBER);
        buttons.setR2(Buttons::NONE);
    }
}

void TrackSummaryView::updateFace()
{
    summaryFaceMap.setVisible(mCurrentFace == FACE_MAP);
    summaryFaceOverview.setVisible(mCurrentFace == FACE_OVERVIEW);
    summaryFaceHeartRate.setVisible(mCurrentFace == FACE_HEARTRATE);
    summaryFaceChart.setVisible(mCurrentFace == FACE_CHART);
    summaryFaceLaps.setVisible(mCurrentFace == FACE_LAPS);

    summaryFaceMap.invalidate();
    summaryFaceOverview.invalidate();
    summaryFaceHeartRate.invalidate();
    summaryFaceChart.invalidate();
    summaryFaceLaps.invalidate();

    updateScrollIndicator();
}

void TrackSummaryView::updateScrollIndicator()
{
    const uint8_t chart = summaryFaceChart.hasChart() ? 1 : 0;
    const uint8_t id    = mCurrentFace <= FACE_HEARTRATE ? mCurrentFace
                        : mCurrentFace == FACE_CHART     ? static_cast<uint8_t>(FACE_CHART)
                                                         : static_cast<uint8_t>(FACE_CHART + chart + summaryFaceLaps.getScrollPage());
    scrollIndicator.animateToId(id);
}

uint8_t TrackSummaryView::nextFace(uint8_t face) const
{
    if (face < FACE_HEARTRATE) {
        return static_cast<uint8_t>(face + 1);
    }
    if (face == FACE_HEARTRATE && summaryFaceChart.hasChart()) {
        return FACE_CHART;
    }
    if (face < FACE_LAPS && mLapsPageCount > 0) {
        return FACE_LAPS;
    }
    return FACE_COUNT;
}

uint8_t TrackSummaryView::prevFace(uint8_t face) const
{
    if (face == FACE_LAPS) {
        return summaryFaceChart.hasChart() ? FACE_CHART : FACE_HEARTRATE;
    }
    return face > FACE_MAP ? static_cast<uint8_t>(face - 1) : FACE_COUNT;
}

void TrackSummaryView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L2) {
        if (mCurrentFace == FACE_LAPS) {
            if (summaryFaceLaps.canScrollDown()) {
                summaryFaceLaps.scrollDown();
                updateScrollIndicator();
            }
        } else if (nextFace(mCurrentFace) != FACE_COUNT) {
            mCurrentFace = nextFace(mCurrentFace);
            updateFace();
        }
        return;
    }

    if (key == SDK::GUI::Button::L1) {
        if (mCurrentFace == FACE_LAPS) {
            if (summaryFaceLaps.canScrollUp()) {
                summaryFaceLaps.scrollUp();
                updateScrollIndicator();
            } else {
                mCurrentFace = prevFace(mCurrentFace);
                updateFace();
            }
        } else if (prevFace(mCurrentFace) != FACE_COUNT) {
            mCurrentFace = prevFace(mCurrentFace);
            updateFace();
        }
        return;
    }

    if (key == SDK::GUI::Button::R1) {
        if (!mTrackIsPaused) presenter->backToTrack();
        return;
    }

    if (key == SDK::GUI::Button::R2) {
        if (mTrackIsPaused) presenter->backToTrack();
        return;
    }
}
