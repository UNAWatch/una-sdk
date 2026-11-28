/**
 ******************************************************************************
 * @file    TrackDetailScreen.cpp
 * @brief   One stored track, and Forget (see TrackDetailScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/TrackDetailScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include <cstdio>

#include "gui/TrackText.hpp"

TrackDetailScreen::TrackDetailScreen(Model& model)
    : Screen(model)
{
}

void TrackDetailScreen::build()
{
    // RunLVGL's look. The title, the last use as one white line
    // under it, Forget alone on the wheel's teal lens (held with R1, as
    // Discard is; the R1 hint is red), and the runs under the lens.
    std::snprintf(mTitleText, sizeof(mTitleText), "TRACK %u", static_cast<unsigned>(mModel.chosenTrack()));
    mItem    = { WheelMenu::Item::Style::Simple, "Forget" };
    mMenu    = std::make_unique<WheelMenu>(mRoot, &mItem, 1);
    mMenu->setCircular(false);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::RED, Widgets::Buttons::WHITE);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, mTitleText);
    mUsed    = Theme::label(mRoot, Theme::Font::Italic18, "", 35, 57, 170);
    mRuns    = Theme::label(mRoot, Theme::Font::Medium18, "", 30, 166, 180);
}

void TrackDetailScreen::onShow()
{
    mModel.resetIdleTimer();
    const Model::Tracks& t = mModel.getTracks();
    for (uint8_t i = 0; i < t.count; ++i) {
        const CustomMessage::TrackList::Entry& e = t.tracks[i];
        if (e.number != mModel.chosenTrack()) {
            continue;
        }
        char date[12];
        char buf[32];
        TrackText::shortDate(date, sizeof(date), e.lastUsedS, t.tzOffsetS);
        if (date[0] != '\0') {
            std::snprintf(buf, sizeof(buf), "Last used %s", date);
        } else {
            std::snprintf(buf, sizeof(buf), "Date not known");
        }
        lv_label_set_text(mUsed, buf);
        std::snprintf(buf, sizeof(buf), "%u %s", static_cast<unsigned>(e.uses), e.uses == 1 ? "run" : "runs");
        lv_label_set_text(mRuns, buf);
    }
}

void TrackDetailScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        // Forget starts a hold-to-confirm the moment R1 goes down, as Discard does.
        case Btn::R1_PRESS:
            mModel.setHoldConfirmMode(Model::HoldConfirmMode::Forget);
            ScreenManager::instance().goTo(ScreenId::TrackHoldConfirm);
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::MenuTracks);
            break;
        default:
            break;
    }
}
