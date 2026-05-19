#include <gui/tracksummary_screen/TrackSummaryView.hpp>
#include <SDK/GUI/Button.hpp>
#include <SDK/Utils/Utils.hpp>

TrackSummaryView::TrackSummaryView()
{
}

void TrackSummaryView::setupScreen()
{
    TrackSummaryViewBase::setupScreen();

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
    scrollIndicator.setCount(3 + mLapsPageCount);

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
    summaryFaceLaps.setVisible(mCurrentFace == FACE_LAPS);

    summaryFaceMap.invalidate();
    summaryFaceOverview.invalidate();
    summaryFaceHeartRate.invalidate();
    summaryFaceLaps.invalidate();

    updateScrollIndicator();
}

void TrackSummaryView::updateScrollIndicator()
{
    const uint8_t id = (mCurrentFace < FACE_LAPS)
        ? mCurrentFace
        : static_cast<uint8_t>(FACE_LAPS + summaryFaceLaps.getScrollPage());
    scrollIndicator.animateToId(id);
}

void TrackSummaryView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L2) {
        if (mCurrentFace == FACE_LAPS) {
            if (summaryFaceLaps.canScrollDown()) {
                summaryFaceLaps.scrollDown();
                updateScrollIndicator();
            }
        } else if (mCurrentFace < FACE_HEARTRATE || mLapsPageCount > 0) {
            mCurrentFace++;
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
                mCurrentFace--;
                updateFace();
            }
        } else if (mCurrentFace > FACE_MAP) {
            mCurrentFace--;
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
