#include <gui/main_screen/MainView.hpp>
#include <SDK/GUI/Button.hpp>
#include <images/BitmapDatabase.hpp>

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

    orbitMenu.setHeight(152);          // visible/clip area, centred on screen
    orbitMenu.setCenterOffset(0);
    orbitMenu.setAnimationSteps(kAnimSteps);
    orbitMenu.setCircularMinItems(3);

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

    updateTitle(0);
}

void MainView::buildEntries(const std::vector<Timer>& presets,
                            const std::vector<Timer>& recents)
{
    mItems.clear();
    mLabels.clear();
    mEntries.clear();

    mItems.push_back({ Item::NEW, {}, false });
    for (const auto& p : presets) {
        mItems.push_back({ Item::VALUE, p, false });
    }
    for (const auto& r : recents) {
        mItems.push_back({ Item::VALUE, r, true });
    }

    // Stable label strings (Entry::label points into this vector).
    mLabels.reserve(mItems.size());
    for (const auto& it : mItems) {
        mLabels.push_back(it.kind == Item::NEW ? std::string("New")
                                               : formatValue(it.timer.durationSec));
    }

    mEntries.assign(mItems.size(), OrbitMenu::Entry{});
    for (size_t i = 0; i < mItems.size(); ++i) {
        mEntries[i].label = mLabels[i].c_str();
        if (mItems[i].kind == Item::NEW) {
            // New carries the plus icon; values are icon-less (centred labels).
            mEntries[i].icon60 = BITMAP_CIRCLEPLUS_50X50_ID;
            mEntries[i].icon30 = BITMAP_CIRCLEPLUS_50X50_ID;
        }
    }
}

void MainView::updateTitle(int16_t index)
{
    if (index < 0 || index >= static_cast<int16_t>(mItems.size())) {
        return;
    }
    title.set(mItems[index].isRecent ? "RECENT" : "TIMER");
}

void MainView::handleKeyEvent(uint8_t key)
{
    if (mItems.empty()) {
        return;
    }

    if (key == SDK::GUI::Button::L1) {
        orbitMenu.selectPrev();
        scrollIndicator.animateToId(scrollIndicator.getActiveId() - 1, kAnimSteps);
        updateTitle(orbitMenu.getSelected());
    }
    else if (key == SDK::GUI::Button::L2) {
        orbitMenu.selectNext();
        scrollIndicator.animateToId(scrollIndicator.getActiveId() + 1, kAnimSteps);
        updateTitle(orbitMenu.getSelected());
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
    int16_t idx = orbitMenu.getSelected();
    if (idx < 0 || idx >= static_cast<int16_t>(mItems.size())) {
        return;
    }

    const Item& cur = mItems[idx];
    if (cur.kind == Item::NEW) {
        presenter->editNew();
        application().gotoEditScreenNoTransition();
    }
    else {
        presenter->selectTimer(cur.timer);
        application().gotoMenuScreenNoTransition();
    }
}
