#include <gui/main_screen/MainView.hpp>
#include <SDK/GUI/Button.hpp>

#include <cstdio>

static constexpr int16_t kAnimSteps = 4;

static std::string formatValue(uint16_t sec)
{
    char b[8];
    std::snprintf(b, sizeof(b), "%02u:%02u", sec / 60u, sec % 60u);
    return std::string(b);
}

MainView::MainView()
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
    orbitMenu.setAnimationSteps(kAnimSteps);
    orbitMenu.setCircularMinItems(3);      // wrap through New at the ends
    const OrbitMenu::Anchors anchors = { {  0, 22,  87 },
                                         { 58, 45,  92 },
                                         { 96, 71, 108 } };
    orbitMenu.setAnchors(anchors);

    scrollIndicator.setConfig(ScrollIndicator::kSmall);

    touchgfx::Unicode::snprintf(newTextBuffer, NEWTEXT_SIZE, "New");
    newText.setWildcard(newTextBuffer);
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
    for (const auto& r : recents) {
        mItems.push_back({ Item::VALUE, r, true });
    }

    mLabels.reserve(mItems.size());
    for (const auto& it : mItems) {
        mLabels.push_back(it.kind == Item::NEW ? std::string("New")
                                               : formatValue(it.timer.durationSec));
    }

    // Icon-less entries: New shows as plain "New" text in the wheel (its centred
    // face is the static overlay); values are centred numbers.
    mEntries.assign(mItems.size(), OrbitMenu::Entry{});
    for (size_t i = 0; i < mItems.size(); ++i) {
        mEntries[i].label = mLabels[i].c_str();
    }
}

void MainView::moveSelection(bool forward)
{
    const int16_t n = static_cast<int16_t>(mItems.size());
    if (n <= 1) {
        return;
    }

    const int16_t next = forward
        ? static_cast<int16_t>((orbitMenu.getSelected() + 1) % n)
        : static_cast<int16_t>((orbitMenu.getSelected() - 1 + n) % n);

    // The wheel animates in every direction (the 60px font now carries the "New"
    // letters, so leaving New shows the "New" text scrolling away).
    if (forward) {
        orbitMenu.selectNext();
    } else {
        orbitMenu.selectPrev();
    }

    // Scroll indicator: landing on New swaps its static face in instantly, so
    // jump the indicator to match; otherwise animate in sync with the wheel.
    if (next == 0) {
        scrollIndicator.setActiveId(0);
    } else {
        scrollIndicator.animateToId(next, kAnimSteps);
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

    const bool isRecent =
        sel >= 0 && sel < static_cast<int16_t>(mItems.size()) && mItems[sel].isRecent;
    title.set(isRecent ? "RECENT" : "TIMER");
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
