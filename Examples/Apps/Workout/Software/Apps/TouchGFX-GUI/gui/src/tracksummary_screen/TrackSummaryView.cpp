#include <gui/tracksummary_screen/TrackSummaryView.hpp>
#include <SDK/GUI/Button.hpp>

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
    scrollIndicator.setActiveId(FACE_OVERVIEW);

    mCurrentFace = FACE_OVERVIEW;
    updateFace();
}

void TrackSummaryView::tearDownScreen()
{
    TrackSummaryViewBase::tearDownScreen();
}

void TrackSummaryView::setSummary(const ActivitySummary& s, bool isPaused)
{
    mTrackIsPaused = isPaused;

    summaryFaceOverview.setTotalTime(s.time);
    summaryFaceOverview.setAvgHR(s.hrAvg);
    summaryFaceOverview.setMaxHR(s.hrMax);

    summaryFaceCal.setTotalTime(s.time);
    summaryFaceCal.setTotalCalories(s.calories);
    summaryFaceCal.setActiveCalories(s.activeCalories);
    summaryFaceCal.setRestingCalories(s.restingCalories);

    summaryFaceZones.setZoneSummary(s);

    summaryFaceLaps.setLaps(s.laps);
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
    summaryFaceOverview.setVisible(mCurrentFace == FACE_OVERVIEW);
    summaryFaceCal.setVisible(mCurrentFace == FACE_CALORIES);
    summaryFaceZones.setVisible(mCurrentFace == FACE_ZONES);
    summaryFaceLaps.setVisible(mCurrentFace == FACE_LAPS);

    summaryFaceOverview.invalidate();
    summaryFaceCal.invalidate();
    summaryFaceZones.invalidate();
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
        } else if (mCurrentFace < FACE_ZONES || mLapsPageCount > 0) {
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
        } else if (mCurrentFace > FACE_OVERVIEW) {
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
