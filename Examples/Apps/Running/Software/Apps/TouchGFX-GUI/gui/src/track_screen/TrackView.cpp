#include <gui/track_screen/TrackView.hpp>
#include <SDK/Utils/Utils.hpp>
#include <cstring>

using FaceId = App::MenuNav::TrackView::Id;

TrackView::TrackView()
{

}

void TrackView::setupScreen()
{
    TrackViewBase::setupScreen();

    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::NONE);
    buttons.setR2(Buttons::AMBER);

    scrollIndicator.setConfig(ScrollIndicator::kSmall);
}

void TrackView::tearDownScreen()
{
    TrackViewBase::tearDownScreen();
}

void TrackView::setIntervalsMode(bool mode)
{
    mIntervalsMode = mode;
    uint16_t count = mIntervalsMode ? FaceId::ID_COUNT : (FaceId::ID_COUNT - 1);
    scrollIndicator.setCount(count);
}

void TrackView::setPositionId(uint16_t id)
{
    const uint16_t minId = mIntervalsMode ? static_cast<uint16_t>(FaceId::ID_INTERVALS)
                                          : static_cast<uint16_t>(FaceId::ID_TRACK1);
    if (id < minId) {
        id = minId;
    }
    if (id >= FaceId::ID_COUNT) {
        id = FaceId::ID_COUNT - 1;
    }
    mCurrentFaceId = id;

    trackFaceIntervals.setVisible(false);
    trackFaceTotal.setVisible(false);
    trackFaceLap.setVisible(false);
    trackFaceStatus.setVisible(false);

    // Visual index: in normal mode shift down by 1 because ID_INTERVALS is not shown.
    const uint16_t visualIdx = mIntervalsMode ? id : (id - 1u);
    scrollIndicator.setActiveId(visualIdx);

    switch (id) {
        case FaceId::ID_INTERVALS:  trackFaceIntervals.setVisible(true);  break;
        case FaceId::ID_TRACK1:     trackFaceTotal.setVisible(true);      break;
        case FaceId::ID_TRACK2:     trackFaceLap.setVisible(true);        break;
        case FaceId::ID_TRACK3:     trackFaceStatus.setVisible(true);     break;
        default: break;
    }

    trackFaceIntervals.invalidate();
    trackFaceTotal.invalidate();
    trackFaceLap.invalidate();
    trackFaceStatus.invalidate();
}

uint16_t TrackView::getPositionId()
{
    return mCurrentFaceId;
}

void TrackView::setConfig(bool isImperial, SDK::Message::TimeFormat timeFormat, const uint8_t* thresholds, uint8_t thresholdCount)
{
    mIsImperial        = isImperial;
    mTimeFormat        = timeFormat;
    mHrThresholdCount  = thresholdCount < App::Config::kHrThresholdsCount ? 
                            thresholdCount : static_cast<uint8_t>(App::Config::kHrThresholdsCount);
    memcpy(mHrThresholds, thresholds, mHrThresholdCount);
}

void TrackView::setTrackData(const Track::Data& data)
{
    auto paceConv = [this](float secPerM) -> float {
        if (secPerM < 1e-6f) return 0.0f;
        const float secPerKm = secPerM * 1000.0f;
        return mIsImperial ? secPerKm / SDK::Utils::kmToMiles(1.0f) : secPerKm;
    };

    auto distConv = [this](float metres) -> float {
        const float km = metres / 1000.0f;
        return mIsImperial ? SDK::Utils::kmToMiles(km) : km;
    };

    trackFaceTotal.setPace(paceConv(data.pace));
    trackFaceTotal.setDistance(distConv(data.distance), mIsImperial);
    trackFaceTotal.setTimer(data.totalTime);

    trackFaceLap.setPace(paceConv(data.lapPace));
    trackFaceLap.setDistance(distConv(data.lapDistance));
    trackFaceLap.setTimer(data.lapTime);
    trackFaceLap.setHR(data.hr, mHrThresholds, mHrThresholdCount);

    if (mIntervalsMode) {
        const Track::IntervalsData& iv = data.intervals;

        trackFaceIntervals.setPhase(iv.phase, iv.repeat, iv.totalRepeats);

        if (iv.metric == Track::IntervalsMetric::DISTANCE) {
            trackFaceIntervals.setPhaseDistance(distConv(iv.distRemaining), mIsImperial);
        } else {
            trackFaceIntervals.setPhaseTime(iv.phaseTimerSec, iv.metric);
        }

        trackFaceIntervals.setPace(paceConv(data.pace));
        trackFaceIntervals.setHR(data.hr);
    }

    mHrSource = data.hrSource;
    updateHrIcon();
}

void TrackView::setTime(uint8_t h, uint8_t m)
{
    trackFaceStatus.setTime(h, m, mTimeFormat);
}

void TrackView::setBatteryLevel(uint8_t level)
{
    trackFaceStatus.setBatteryLevel(level);
}

void TrackView::handleKeyEvent(uint8_t key)
{
    const uint16_t minId = mIntervalsMode ? static_cast<uint16_t>(FaceId::ID_INTERVALS)
                                          : static_cast<uint16_t>(FaceId::ID_TRACK1);

    if (key == SDK::GUI::Button::L1) {
        uint16_t p = mCurrentFaceId;
        if (p <= minId) {
            p = FaceId::ID_COUNT - 1;
        } else {
            p--;
        }
        setPositionId(p);
    }

    if (key == SDK::GUI::Button::L2) {
        uint16_t p = mCurrentFaceId + 1u;
        if (p >= FaceId::ID_COUNT) {
            p = minId;
        }
        setPositionId(p);
    }

    if (key == SDK::GUI::Button::R1) {
        application().gotoTrackActionScreenNoTransition();
    }

    if (key == SDK::GUI::Button::R2) {
        // In an intervals workout the lap button advances to the next phase, on
        // ANY face — laps are phase-driven, not manual. Gate on the workout mode,
        // not the currently shown face (which the user can scroll away from). When
        // the workout completes the Service drops intervalsMode, so R2 then records
        // a manual lap. A free (non-intervals) run always records a manual lap.
        if (mIntervalsMode) {
            presenter->intervalsNextPhase();
        } else {
            presenter->saveLap();
            application().gotoTrackLapScreenNoTransition();
        }
    }
}

void TrackView::setGpsFix(bool state)
{
    trackFaceStatus.setGps(SDK::Gui::SensorStatusRow::gpsState(state));
}

void TrackView::setAccessoryStatus(uint8_t state)
{
    mAccessoryState = state;
    updateHrIcon();
}

void TrackView::updateHrIcon()
{
    // In-activity: icon follows the live HR source, not the raw link state.
    trackFaceStatus.setHr(SDK::Gui::SensorStatusRow::hrStateFromSource(
            mAccessoryState, mHrSource));
}
