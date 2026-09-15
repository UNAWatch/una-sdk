/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   Waypoint's one screen: name, distance, bearing and status.
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"

#include <cstdio>
#include <cstring>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "gui/Assets.hpp"

namespace Draw = SDK::LVGL::Draw;
namespace Color = SDK::GUI::Color;

namespace
{
/// How long a save confirmation stays up before the status line resumes.
constexpr uint32_t kNoticeMs = 2000;

/**
 * @brief   Distance as the shortest string that still reads correctly.
 *
 * The row is 200 px wide, and a waypoint left at its default can easily be
 * hundreds of kilometres away, so "412.3 km" would not fit at the size this
 * row is drawn in.
 */
void formatDistance(float metres, char* out, size_t outSize)
{
    if (metres < 1000.0f) {
        std::snprintf(out, outSize, "%d m", static_cast<int>(metres));
    } else if (metres < 10000.0f) {
        std::snprintf(out, outSize, "%.1f km", static_cast<double>(metres) / 1000.0);
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

MainScreen::MainScreen(Model& model)
    : mModel(model)
{
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &MainScreen::keyEventCb, LV_EVENT_KEY, this);

    // Four centred rows at the TouchGFX design's positions.
    mName     = Draw::label(mRoot, &poppins_semibold_20, "", 40, 40, 160);
    mDistance = Draw::label(mRoot, &poppins_semibold_40, "", 20, 74, 200);
    mBearing  = Draw::label(mRoot, &poppins_semibold_25, "", 20, 128, 200);
    mStatus   = Draw::label(mRoot, &poppins_regular_18, "", 40, 170, 160);

    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::NONE, SDK::LVGL::Buttons::NONE,
                  SDK::LVGL::Buttons::WHITE,    // save the current position
                  SDK::LVGL::Buttons::WHITE);   // back

    bind(&mModel);
    mModel.bind(this);

    // The service sends an update as soon as the GUI starts; until then draw
    // what the model has, so no placeholder reaches the screen.
    showNav(mModel.nav());
}

MainScreen::~MainScreen()
{
    if (mNotice) {
        lv_timer_delete(mNotice);
    }
    mModel.bind(nullptr);
    lv_obj_delete(mRoot);
}

void MainScreen::keyEventCb(lv_event_t* e)
{
    auto* self = static_cast<MainScreen*>(lv_event_get_user_data(e));
    self->onKey(static_cast<uint8_t>(lv_event_get_key(e)));
}

void MainScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::R1:
            mModel.saveTargetHere();
            break;
        case Btn::R2:
            mModel.exitApp();
            break;
        default:
            break;
    }
}

void MainScreen::onNavUpdate(const CustomMessage::NavState& nav)
{
    showNav(nav);
}

void MainScreen::showNav(const CustomMessage::NavState& nav)
{
    char text[32];

    // The name comes straight from the configuration the user typed on their
    // phone. An unset name is the field's default, never empty.
    lv_label_set_text(mName, nav.waypointName[0] != '\0' ? nav.waypointName : "--");

    if (!nav.hasFix) {
        setDistance("--");
    } else if (nav.arrived) {
        setDistance("HERE");
    } else {
        formatDistance(nav.distanceM, text, sizeof(text));
        setDistance(text);
    }

    if (!nav.hasFix || nav.arrived) {
        lv_label_set_text(mBearing, "--");
    } else {
        std::snprintf(text, sizeof(text), "%s %d", compassPoint(nav.bearingDeg), static_cast<int>(nav.bearingDeg));
        lv_label_set_text(mBearing, text);
    }

    if (mNotice) {
        return;     // leave the save confirmation up
    }

    if (!nav.targetIsConfigured) {
        // Navigating to the compiled-in default because nobody has set a
        // target yet; say so rather than pointing confidently at it.
        setStatus("Set a target");
    } else if (!nav.hasFix) {
        setStatus("Waiting for GPS");
    } else if (nav.arrived) {
        setStatus("Arrived");
    } else {
        setStatus("R1 saves here");
    }
}

void MainScreen::onTargetSaved(CustomMessage::SaveOutcome outcome, float latitude, float longitude)
{
    (void)latitude;
    (void)longitude;

    // A failed write is not the GPS's fault: say which went wrong.
    switch (outcome) {
        case CustomMessage::SaveOutcome::Saved:       setStatus("Target saved"); break;
        case CustomMessage::SaveOutcome::NoFix:       setStatus("No fix yet");   break;
        case CustomMessage::SaveOutcome::WriteFailed: setStatus("Save failed");  break;
    }

    // Hold the confirmation for two seconds, then let the status line resume.
    if (mNotice) {
        lv_timer_reset(mNotice);
    } else {
        mNotice = lv_timer_create(&MainScreen::noticeEndCb, kNoticeMs, this);
    }
}

void MainScreen::noticeEndCb(lv_timer_t* t)
{
    auto* self = static_cast<MainScreen*>(lv_timer_get_user_data(t));
    lv_timer_delete(self->mNotice);
    self->mNotice = nullptr;
    self->showNav(self->mModel.nav());
}

void MainScreen::setDistance(const char* text)
{
    // Pick a face the string fits in: an antipodal target still needs
    // "20015 km", which a fixed 40 px face would clip.
    const size_t     length = std::strlen(text);
    const lv_font_t* font   = (length <= 6) ? &poppins_semibold_40
                            : (length <= 8) ? &poppins_semibold_30
                                            : &poppins_semibold_25;
    lv_obj_set_style_text_font(mDistance, font, LV_PART_MAIN);
    lv_label_set_text(mDistance, text);
}

void MainScreen::setStatus(const char* text)
{
    lv_label_set_text(mStatus, text);
}
