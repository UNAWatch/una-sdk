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
    orbitMenu.setCenterOffset(17);   // re-centre after the optical label lift
    orbitMenu.setAnimationSteps(kAnimSteps);
    orbitMenu.setCircularMinItems(1000);   // bounded: New is the wrap boundary
    // Spread the rows for the large 60px centre value.
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

    mCount    = static_cast<int16_t>(mValues.size()) + 1;   // New + values
    mSelIndex = 0;                                           // start on New

    scrollIndicator.setCount(static_cast<uint16_t>(mCount));
    scrollIndicator.setActiveId(0);

    syncView();
}

void MainView::buildEntries(const std::vector<Timer>& presets,
                            const std::vector<Timer>& recents)
{
    mValues.clear();
    mLabels.clear();
    mEntries.clear();

    for (const auto& p : presets) {
        mValues.push_back({ p, false });
    }
    for (const auto& r : recents) {
        mValues.push_back({ r, true });
    }

    // Stable label strings (Entry::label points into this vector).
    mLabels.reserve(mValues.size());
    for (const auto& v : mValues) {
        mLabels.push_back(formatValue(v.timer.durationSec));
    }

    mEntries.assign(mValues.size(), OrbitMenu::Entry{});
    for (size_t i = 0; i < mValues.size(); ++i) {
        mEntries[i].label = mLabels[i].c_str();   // icon-less -> centred value
    }
}

void MainView::moveSelection(bool forward)
{
    const int16_t prev = mSelIndex;
    mSelIndex = forward ? static_cast<int16_t>((mSelIndex + 1) % mCount)
                        : static_cast<int16_t>((mSelIndex - 1 + mCount) % mCount);

    scrollIndicator.animateToId(
        forward ? static_cast<int16_t>(scrollIndicator.getActiveId() + 1)
                : static_cast<int16_t>(scrollIndicator.getActiveId() - 1),
        kAnimSteps);

    if (mSelIndex != 0) {
        const int16_t orbitIdx = static_cast<int16_t>(mSelIndex - 1);
        if (prev == 0) {
            // Entering the orbit from New: jump instantly to the correct end.
            orbitMenu.setSelected(orbitIdx);
        } else if (forward) {
            orbitMenu.selectNext();
        } else {
            orbitMenu.selectPrev();
        }
    }

    syncView();
}

void MainView::syncView()
{
    const bool isNew = mSelIndex == 0;

    orbitMenu.setVisible(!isNew);
    icon.setVisible(isNew);
    newText.setVisible(isNew);

    icon.invalidate();
    newText.invalidate();
    orbitMenu.invalidate();

    bool isRecent = (!isNew) && mValues[mSelIndex - 1].isRecent;
    title.set(isRecent ? "RECENT" : "TIMER");
}

void MainView::handleKeyEvent(uint8_t key)
{
    if (mCount <= 0) {
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
    if (mSelIndex == 0) {
        presenter->editNew();
        application().gotoEditScreenNoTransition();
        return;
    }

    const int16_t idx = static_cast<int16_t>(mSelIndex - 1);
    if (idx >= 0 && idx < static_cast<int16_t>(mValues.size())) {
        presenter->selectTimer(mValues[idx].timer);
        application().gotoMenuScreenNoTransition();
    }
}
