#include <gui/trackintervalsalert_screen/TrackIntervalsAlertView.hpp>
#include <texts/TextKeysAndLanguages.hpp>

// Auto-dismiss duration: 5 seconds
static constexpr uint16_t kDismissTicksDuration = SDK::Utils::secToTicks(5, App::Config::kFrameRate);

// Accent colors -- must match TrackFaceIntervals
static constexpr uint32_t kColorNeutral = SDK::GUI::Color::WHITE;
static constexpr uint32_t kColorRun     = SDK::GUI::Color::CYAN;
static constexpr uint32_t kColorRest    = SDK::GUI::Color::YELLOW_DARK;

// -----------------------------------------------------------------------------

TrackIntervalsAlertView::TrackIntervalsAlertView()
    : mDismissTimerCb(this, &TrackIntervalsAlertView::onDismissTimerFired)
{
}

void TrackIntervalsAlertView::setupScreen()
{
    TrackIntervalsAlertViewBase::setupScreen();

    mDismissTimer.setDuration(kDismissTicksDuration);
    mDismissTimer.setCallback(mDismissTimerCb);
    mDismissTimer.start();
}

void TrackIntervalsAlertView::tearDownScreen()
{
    mDismissTimer.stop();
    TrackIntervalsAlertViewBase::tearDownScreen();
}

// =============================================================================
// Public API
// =============================================================================

void TrackIntervalsAlertView::setPhase(Track::IntervalsPhase phase,
                                        uint8_t repeat,
                                        uint8_t totalRepeats)
{
    touchgfx::colortype color = kColorNeutral;
    TypedTextId titleId       = T_TEXT_WARM_UP_UC;

    switch (phase) {
    case Track::IntervalsPhase::WARM_UP:
        titleId = T_TEXT_WARM_UP_UC;
        color   = kColorNeutral;
        break;
    case Track::IntervalsPhase::RUN:
        titleId = T_TEXT_RUN_UC;
        color   = kColorRun;
        break;
    case Track::IntervalsPhase::REST:
        titleId = T_TEXT_REST_UC;
        color   = kColorRest;
        break;
    case Track::IntervalsPhase::COOL_DOWN:
        titleId = T_TEXT_COOL_DOWN_UC;
        color   = kColorNeutral;
        break;
    }

    title.set(titleId);
    intervalsTimer.setColor(color);
    intervalsTimer.setLineVisible(false);

    // Repeat counter -- only meaningful during RUN / REST
    const bool showRepeats = (phase == Track::IntervalsPhase::RUN ||
                              phase == Track::IntervalsPhase::REST);
    repeatsText.setVisible(showRepeats);
    if (showRepeats) {
        Unicode::snprintf(repeatsTextBuffer, REPEATSTEXT_SIZE,
                          "%u/%u", repeat, totalRepeats);
    }
    repeatsText.invalidate();
}

void TrackIntervalsAlertView::setPhaseTime(std::time_t sec)
{
    intervalsTimer.setRemainingTime(sec);
    intervalsTimer.setDescriptionVisible(false);
}

void TrackIntervalsAlertView::setPhaseDistance(float distDisplay, bool isImperial)
{
    intervalsTimer.setPhaseDistance(distDisplay, isImperial);
    intervalsTimer.setDescriptionVisible(false);
}

// =============================================================================
// Private
// =============================================================================

void TrackIntervalsAlertView::onDismissTimerFired()
{
    application().gotoTrackScreenNoTransition();
}
