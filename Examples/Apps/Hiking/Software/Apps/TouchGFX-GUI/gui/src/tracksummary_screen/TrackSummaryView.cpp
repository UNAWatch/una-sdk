#include <gui/tracksummary_screen/TrackSummaryView.hpp>

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
    mIsImperial = isImperial;
    mTrackIsPaused = isPaused;

    auto distConv = [isImperial](float metres) -> float {
        const float km = metres / 1000.0f;
        return isImperial ? SDK::Utils::kmToMiles(km) : km;
    };

    auto elevationConv = [isImperial](float metres) -> float {
        return isImperial ? SDK::Utils::metersToFeet(metres) : metres;
    };

    const float dist = distConv(s.distance);
    summaryFaceMap.setDistance(dist, isImperial);
    summaryFaceOverview.setDistance(dist, isImperial);
    summaryFaceOverview.setElevation(elevationConv(s.elevation), isImperial);
    summaryFaceOverview.setTimer(s.time);
    summaryFaceHeartRate.setMaxHR(s.hrMax);
    summaryFaceHeartRate.setAvgHR(s.hrAvg);
    summaryFaceMap.setMap(s.map);

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

    summaryFaceMap.invalidate();
    summaryFaceOverview.invalidate();
    summaryFaceHeartRate.invalidate();

    updateScrollIndicator();
}

void TrackSummaryView::updateScrollIndicator()
{
    scrollIndicator.animateToId(mCurrentFace);
}

void TrackSummaryView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L2) {
        if (mCurrentFace < FACE_HEARTRATE) {
            mCurrentFace++;
            updateFace();
        }
        return;
    }

    if (key == SDK::GUI::Button::L1) {
        if (mCurrentFace > FACE_MAP) {
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
