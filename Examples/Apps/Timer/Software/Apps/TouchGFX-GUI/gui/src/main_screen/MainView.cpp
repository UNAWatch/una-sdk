#include <gui/main_screen/MainView.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <SDK/GUI/Button.hpp>

#include <cstdio>

static std::string formatValue(uint16_t sec)
{
    char b[8];
    std::snprintf(b, sizeof(b), "%02u:%02u", sec / 60u, sec % 60u);
    return std::string(b);
}

MainView::MainView()
    : mAnimEndedCb(this, &MainView::onOrbitAnimationEnded)
{
}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    // Side scroll indicator replaces the left buttons; only select/back show.
    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);

    orbitMenu.setHeight(152);
    orbitMenu.setCenterOffset(0);
    orbitMenu.setAnimationSteps(App::Config::kMenuAnimationSteps);
    orbitMenu.setCircularMinItems(3);      // wrap through New at the ends
    const OrbitMenu::Anchors anchors = { {  0, 22,  87 },
                                         { 58, 45,  92 },
                                         { 96, 71, 108 } };
    orbitMenu.setAnchors(anchors);

    orbitMenu.setAnimationEndedCallback(mAnimEndedCb);

    scrollIndicator.setConfig(ScrollIndicator::kSmall);

}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::setLists(const std::vector<Timer>& presets,
                        const std::vector<Timer>& recents)
{
    buildEntries(presets, recents);

    orbitMenu.setItems(mEntries.data(), static_cast<int16_t>(mEntries.size()));
    orbitMenu.setSelected(0);

    scrollIndicator.setCount(static_cast<uint16_t>(mEntries.size()));
    scrollIndicator.setActiveId(0);

    updateRecentLabel();
    syncView();
}

void MainView::buildEntries(const std::vector<Timer>& presets,
                            const std::vector<Timer>& recents)
{
    mItems.clear();
    mLabels.clear();
    mEntries.clear();

    // New is entry 0 so it wraps into view as a neighbour at the list ends.
    mItems.push_back({ Item::NEW, {}, false });
    for (const auto& p : presets) {
        mItems.push_back({ Item::VALUE, p, false });
    }
    mFirstRecentIdx = recents.empty()
        ? static_cast<int16_t>(-1)
        : static_cast<int16_t>(mItems.size());   // first recent lands here
    for (const auto& r : recents) {
        mItems.push_back({ Item::VALUE, r, true });
    }

    mLabels.reserve(mItems.size());
    for (const auto& it : mItems) {
        mLabels.push_back(it.kind == Item::NEW ? std::string("New")
                                               : formatValue(it.timer.durationSec));
    }

    // Icon-less entries: New shows a localised "New" in the wheel (its centred
    // face is the static overlay); values are centred numbers (raw strings).
    mEntries.assign(mItems.size(), OrbitMenu::Entry{});
    for (size_t i = 0; i < mItems.size(); ++i) {
        if (mItems[i].kind == Item::NEW) {
            mEntries[i].labelId = T_TEXT_NEW;
        } else {
            mEntries[i].label = mLabels[i].c_str();
        }
    }
}

void MainView::moveSelection(bool forward)
{
    const int16_t n = static_cast<int16_t>(mItems.size());
    if (n <= 1) {
        return;
    }

    // Virtual (un-wrapped) target: may be -1 or n. The scroll indicator needs it
    // raw so it can pick the correct wrap direction; the wheel index wraps.
    const int16_t virtualId = static_cast<int16_t>(orbitMenu.getSelected()) + (forward ? 1 : -1);
    const int16_t next      = static_cast<int16_t>((virtualId % n + n) % n);

    // The wheel animates in every direction (the 60px font now carries the "New"
    // letters, so leaving New shows the "New" text scrolling away).
    if (forward) {
        orbitMenu.selectNext();
    } else {
        orbitMenu.selectPrev();
    }

    // Scroll indicator: landing on New swaps its static face in instantly, so
    // jump the indicator to match; otherwise animate to the virtual id so the
    // wrap slides the short way round.
    if (next == 0) {
        scrollIndicator.setActiveId(0);
    } else {
        scrollIndicator.animateToId(virtualId, App::Config::kMenuAnimationSteps);
    }

    // Entering the recents shows the value immediately; leaving keeps the value
    // through the slide and defers the swap to "Recent" to the animation end.
    if (next >= 0 && next < static_cast<int16_t>(mItems.size()) && mItems[next].isRecent) {
        updateRecentLabel();
    }

    syncView();
}

void MainView::syncView()
{
    const int16_t sel   = orbitMenu.getSelected();
    const bool    isNew = (sel == 0);

    // New centred -> static plus-icon + label face, orbit hidden.
    orbitMenu.setVisible(!isNew);
    icon.setVisible(isNew);
    newText.setVisible(isNew);

    orbitMenu.invalidate();
    icon.invalidate();
    newText.invalidate();

    const bool centeredRecent =
        sel >= 0 && sel < static_cast<int16_t>(mItems.size()) && mItems[sel].isRecent;
    title.set(centeredRecent ? T_TEXT_RECENT_UC : T_TEXT_TIMER_UC);
}

void MainView::updateRecentLabel()
{
    // The first recent doubles as the "Recent" section hint: it reads "Recent"
    // while a non-recent is centred, and its value once the recents are entered.
    if (mFirstRecentIdx < 0) {
        return;
    }
    const int16_t sel = orbitMenu.getSelected();
    const bool centeredRecent =
        sel >= 0 && sel < static_cast<int16_t>(mItems.size()) && mItems[sel].isRecent;
    if (centeredRecent) {
        orbitMenu.setEntryLabel(mFirstRecentIdx, mLabels[mFirstRecentIdx].c_str());
    } else {
        orbitMenu.setEntryLabel(mFirstRecentIdx, T_TEXT_RECENT);
    }
}

void MainView::onOrbitAnimationEnded()
{
    // Settle the "Recent" hint on the final frame: leaving the recents keeps the
    // number on screen for the whole animation and only turns it into "Recent"
    // here, so the user sees a shrinking value rather than a shrinking label.
    updateRecentLabel();
}

void MainView::handleKeyEvent(uint8_t key)
{
    if (mItems.empty()) {
        return;
    }

    if (key == SDK::GUI::Button::L1) {
        moveSelection(false);
    }
    else if (key == SDK::GUI::Button::L2) {
        moveSelection(true);
    }
    else if (key == SDK::GUI::Button::R1) {
        onConfirm();
    }
    else if (key == SDK::GUI::Button::R2) {
        presenter->exitApp();
    }
}

void MainView::onConfirm()
{
    const int16_t sel = orbitMenu.getSelected();
    if (sel < 0 || sel >= static_cast<int16_t>(mItems.size())) {
        return;
    }

    if (mItems[sel].kind == Item::NEW) {
        presenter->editNew();
        application().gotoEditScreenNoTransition();
    }
    else {
        presenter->selectTimer(mItems[sel].timer);
        application().gotoMenuScreenNoTransition();
    }
}
