/**
 ******************************************************************************
 * @file    TracksScreen.cpp
 * @brief   Settings > Tracks (see TracksScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/TracksScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"

#include <cstdio>

#include "gui/TrackText.hpp"

TracksScreen::TracksScreen(Model& model)
    : Screen(model)
{
}

void TracksScreen::build()
{
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, "TRACKS");
    // With no tracks, RunLVGL's empty workout list in its place,
    // all in white (RunLVGL's lines are grey).
    mEmpty[0] = Theme::label(mRoot, Theme::Font::SemiBold25, "No tracks yet", 0, 86, 240);
    mEmpty[1] = Theme::label(mRoot, Theme::Font::Medium18, "Run on a track", 0, 131, 240);
    mEmpty[2] = Theme::label(mRoot, Theme::Font::Medium18, "to save one", 0, 154, 240);
    for (lv_obj_t* l : mEmpty) {
        Theme::setHidden(l, true);
    }
}

void TracksScreen::onShow()
{
    mModel.resetIdleTimer();
    fill();                       // what the model has, until the fresh list comes
    mModel.requestTrackList();
}

void TracksScreen::onTrackList()
{
    fill();
}

void TracksScreen::fill()
{
    const Model::Tracks& t = mModel.getTracks();
    const uint16_t keep = mMenu ? mMenu->selected() : 0;
    mCount = t.received ? t.count : 0;
    for (lv_obj_t* l : mEmpty) {
        Theme::setHidden(l, !(t.received && mCount == 0));
    }
    for (uint8_t i = 0; i < mCount; ++i) {
        const CustomMessage::TrackList::Entry& e = t.tracks[i];
        std::snprintf(mTexts[i], sizeof(mTexts[i]), "Track %u", static_cast<unsigned>(e.number));
        char date[12];
        TrackText::shortDate(date, sizeof(date), e.lastUsedS, t.tzOffsetS);
        const unsigned uses = e.uses;
        if (date[0] != '\0') {
            std::snprintf(mTips[i], sizeof(mTips[i]), "%s, %u %s", date, uses, uses == 1 ? "run" : "runs");
        } else {
            std::snprintf(mTips[i], sizeof(mTips[i]), "%u %s", uses, uses == 1 ? "run" : "runs");
        }
        // As RunLVGL's workout list: SemiBold 25 with the hint under it,
        // white when selected and teal around it.
        mItems[i] = { WheelMenu::Item::Style::Tip, mTexts[i], nullptr, &poppins_semibold_25, mTips[i],
                      SDK::GUI::Color::TEAL };
    }
    // The wheel is built again only when the list changes length (the first
    // list arriving, or a track forgotten); otherwise its texts are
    // refreshed. A rebuild deletes the old wheel's objects first, so two
    // wheels' worth never sit in LVGL's pool at once.
    const uint16_t rows = mCount;
    if (rows == 0) {
        // No wheel until there is a track (the empty lines, or nothing while
        // the list is on its way).
        mMenu.reset();
        if (mWheelBox != nullptr) {
            lv_obj_delete(mWheelBox);
            mWheelBox = nullptr;
        }
    } else if (!mMenu || mMenu->count() != rows) {
        // The SDK wheel leaves its objects to its parent when it is
        // destroyed, so it has a parent of its own, deleted with it. Made
        // last, it is drawn over the title and buttons.
        mMenu.reset();
        if (mWheelBox != nullptr) {
            lv_obj_delete(mWheelBox);
        }
        mWheelBox = Theme::container(mRoot, 0, 0, 240, 240);
        mMenu = std::make_unique<WheelMenu>(mWheelBox, mItems, rows);
        mMenu->setCircular(false);
        mMenu->select(keep < rows ? keep : 0);
    } else {
        mMenu->refresh();
    }
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  mCount > 0 ? Widgets::Buttons::AMBER : Widgets::Buttons::NONE, Widgets::Buttons::WHITE);
}

void TracksScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: if (mMenu) mMenu->prev(); break;
        case Btn::L2: if (mMenu) mMenu->next(); break;
        case Btn::R1:
            if (mMenu && mCount > 0) {
                mModel.setChosenTrack(mModel.getTracks().tracks[mMenu->selected()].number);
                ScreenManager::instance().goTo(ScreenId::MenuTrackDetail);
            }
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::MenuSettings);
            break;
        default:
            break;
    }
}
