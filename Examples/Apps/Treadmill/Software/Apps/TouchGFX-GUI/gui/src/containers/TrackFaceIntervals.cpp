#include <gui/containers/TrackFaceIntervals.hpp>
#include <texts/TextKeysAndLanguages.hpp>

TrackFaceIntervals::TrackFaceIntervals()
{
}

void TrackFaceIntervals::initialize()
{
    TrackFaceIntervalsBase::initialize();
}

// =============================================================================
// Phase appearance
// =============================================================================

void TrackFaceIntervals::setPhase(Track::IntervalsPhase phase, uint8_t repeat, uint8_t totalRepeats)
{
    paceText.setVisible(false);
    hrText.setVisible(false);
    imageRun.setVisible(false);
    imageHeartRate.setVisible(false);
    imagePace.setVisible(false);
    intervalsTimer.setLineVisible(false);

    touchgfx::colortype color = mColorNeutral;
    TypedTextId titleId       = T_TEXT_WARM_UP_UC;

    switch (phase) {
    case Track::IntervalsPhase::WARM_UP:
        titleId = T_TEXT_WARM_UP_UC;
        color   = mColorNeutral;
        imageRun.setVisible(true);
        break;
    case Track::IntervalsPhase::RUN:
        titleId = T_TEXT_RUN_UC;
        color   = mColorRun;
        intervalsTimer.setLineVisible(true);
        imagePace.setVisible(true);
        paceText.setVisible(true);
        hrText.setVisible(false);
        break;
    case Track::IntervalsPhase::REST:
        titleId = T_TEXT_REST_UC;
        color   = mColorRest;
        intervalsTimer.setLineVisible(true);
        imageHeartRate.setVisible(true);
        paceText.setVisible(false);
        hrText.setVisible(true);
        break;
    case Track::IntervalsPhase::COOL_DOWN:
        titleId = T_TEXT_COOL_DOWN_UC;
        color   = mColorNeutral;
        imageRun.setVisible(true);
        break;
    }

    title.set(titleId);
    intervalsTimer.setColor(color);

    // Repeats counter -- only during RUN / REST
    const bool showRepeats = (phase == Track::IntervalsPhase::RUN || phase == Track::IntervalsPhase::REST);
    repeatsText.setVisible(showRepeats);
    if (showRepeats) {
        Unicode::snprintf(repeatsTextBuffer, REPEATSTEXT_SIZE,
            "%u/%u", repeat, totalRepeats);
    }
    repeatsText.invalidate();
    paceText.invalidate();
    imageRun.invalidate();
    imageHeartRate.invalidate();
    imagePace.invalidate();
}

// =============================================================================
// Timer area
// =============================================================================

void TrackFaceIntervals::setPhaseTime(std::time_t sec, Track::IntervalsMetric metric)
{
    intervalsTimer.setPhaseTime(sec, metric);
}

void TrackFaceIntervals::setPhaseDistance(float dist, bool isImperial)
{
    intervalsTimer.setPhaseDistance(dist, isImperial);
}

// =============================================================================
// Bottom row
// =============================================================================

void TrackFaceIntervals::setPace(float pace)
{
    if (pace < App::Display::kMinPace) {
        Unicode::snprintf(paceTextBuffer, PACETEXT_SIZE, "---");
    } else {
        auto hms = SDK::Utils::toHMS(static_cast<std::time_t>(pace));
        if (hms.h > 0) {
            Unicode::snprintf(paceTextBuffer, PACETEXT_SIZE, "%u:%02u", hms.h, hms.m);
        } else {
            Unicode::snprintf(paceTextBuffer, PACETEXT_SIZE, "%u:%02u", hms.m, hms.s);
        }
    }
    paceText.invalidate();
}

void TrackFaceIntervals::setHR(float hr)
{
    if (hr < App::Display::kMinHR) {
        Unicode::snprintf(hrTextBuffer, HRTEXT_SIZE, "---");
    } else {
        Unicode::snprintf(hrTextBuffer, HRTEXT_SIZE, "%u", static_cast<uint16_t>(hr));
    }
    hrText.invalidate();
}

void TrackFaceIntervals::setPhaseColors(touchgfx::colortype neutral,
                                        touchgfx::colortype run,
                                        touchgfx::colortype rest)
{
    mColorNeutral = neutral;
    mColorRun     = run;
    mColorRest    = rest;
}
