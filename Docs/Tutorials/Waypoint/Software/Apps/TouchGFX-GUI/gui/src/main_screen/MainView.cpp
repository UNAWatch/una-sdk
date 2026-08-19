#include <gui/main_screen/MainView.hpp>

#include <cstdio>
#include <cstring>

namespace {

/// Two seconds. The frame rate is 10 Hz, so deriving the tick count from
/// milliseconds avoids guessing at it -- 60 ticks would have been six seconds.
constexpr int kNoticeTicks = GUI_CONFIG_MS_2_TICKS(2000);

/**
 * @brief   Distance as the shortest string that still reads correctly.
 *
 * Kept short on purpose: the row is 200 px wide, and a waypoint left at its
 * default can easily be hundreds of kilometres away, so "412.3 km" would not
 * fit at the size this row is drawn in. Precision that cannot be displayed is
 * not worth the characters.
 */
void formatDistance(float metres, char* out, size_t outSize)
{
    if (metres < 1000.0f) {
        std::snprintf(out, outSize, "%d m", static_cast<int>(metres));
    } else if (metres < 10000.0f) {
        std::snprintf(out, outSize, "%.1f km",
                      static_cast<double>(metres) / 1000.0);
    } else {
        std::snprintf(out, outSize, "%d km", static_cast<int>(metres / 1000.0f));
    }
}

/// Eight-point compass label for a bearing in degrees.
const char* compassPoint(float bearingDeg)
{
    static const char* kPoints[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
    int index = static_cast<int>((bearingDeg + 22.5f) / 45.0f);
    if (index < 0) {
        index = 0;
    }
    return kPoints[index % 8];
}

} // namespace

MainView::MainView()
{

}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    buttons.setL1(ButtonsSet::NONE);
    buttons.setL2(ButtonsSet::NONE);
    buttons.setR1(ButtonsSet::WHITE);   // save the current position
    buttons.setR2(ButtonsSet::WHITE);   // back

    // Every wildcard is written here, so the placeholder from the text database
    // can never reach the screen.
    showNav(CustomMessage::NavState {});
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::handleTickEvent()
{
    MainViewBase::handleTickEvent();

    if (mNoticeTicks > 0) {
        --mNoticeTicks;
    }
}

void MainView::showNav(const CustomMessage::NavState& nav)
{
    char text[32];

    // The name comes straight from the configuration the user typed on their
    // phone. An unset name is the field's default, never empty.
    touchgfx::Unicode::strncpy(nameTextBuffer,
                               nav.waypointName[0] != '\0' ? nav.waypointName : "--",
                               NAMETEXT_SIZE);
    nameText.invalidate();

    if (!nav.hasFix) {
        setDistance("--");
    } else if (nav.arrived) {
        setDistance("HERE");
    } else {
        formatDistance(nav.distanceM, text, sizeof(text));
        setDistance(text);
    }

    if (!nav.hasFix || nav.arrived) {
        touchgfx::Unicode::strncpy(bearingTextBuffer, "--", BEARINGTEXT_SIZE);
    } else {
        std::snprintf(text, sizeof(text), "%s %d", compassPoint(nav.bearingDeg),
                      static_cast<int>(nav.bearingDeg));
        touchgfx::Unicode::strncpy(bearingTextBuffer, text, BEARINGTEXT_SIZE);
    }
    bearingText.invalidate();

    if (mNoticeTicks > 0) {
        return;     // leave the save confirmation up
    }

    if (!nav.targetIsConfigured) {
        // The app is navigating to its compiled-in default because nobody has
        // set a target yet. Saying so beats pointing confidently at Trafalgar
        // Square.
        setStatus("Set a target");
    } else if (!nav.hasFix) {
        setStatus("Waiting for GPS");
    } else if (nav.arrived) {
        setStatus("Arrived");
    } else {
        setStatus("R1 saves here");
    }
}

void MainView::showTargetSaved(CustomMessage::SaveOutcome outcome,
                               float latitude, float longitude)
{
    (void)latitude;
    (void)longitude;

    // A failed write is not the GPS's fault: say which went wrong, or the user
    // goes looking for sky when the volume is full.
    switch (outcome) {
    case CustomMessage::SaveOutcome::Saved:
        setStatus("Target saved");
        break;
    case CustomMessage::SaveOutcome::NoFix:
        setStatus("No fix yet");
        break;
    case CustomMessage::SaveOutcome::WriteFailed:
        setStatus("Save failed");
        break;
    }
    mNoticeTicks = kNoticeTicks;
}

void MainView::setDistance(const char* text)
{
    // Shorter formatting is not enough on its own: an antipodal target still
    // needs "20015 km". Pick a size the string actually fits in rather than
    // letting the row clip it, which is what a fixed 40 px font did.
    const size_t length = std::strlen(text);
    const touchgfx::TypedTextId typography =
            (length <= 6) ? T_TMP_SEMIBOLD_40
                          : ((length <= 8) ? T_TMP_SEMIBOLD_30 : T_TMP_SEMIBOLD_25);

    distanceText.setTypedText(touchgfx::TypedText(typography));
    touchgfx::Unicode::strncpy(distanceTextBuffer, text, DISTANCETEXT_SIZE);
    distanceText.invalidate();
}

void MainView::setStatus(const char* text)
{
    touchgfx::Unicode::strncpy(statusTextBuffer, text, STATUSTEXT_SIZE);
    statusText.invalidate();
}

void MainView::handleKeyEvent(uint8_t key)
{
    if (key == Gui::Config::Button::R1) {
        presenter->saveTargetHere();
    }

    if (key == Gui::Config::Button::R2) {
        presenter->exit();
    }
}
