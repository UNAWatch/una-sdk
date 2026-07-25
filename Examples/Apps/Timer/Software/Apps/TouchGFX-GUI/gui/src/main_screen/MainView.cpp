#include <gui/main_screen/MainView.hpp>
#include <SDK/GUI/Button.hpp>

#include <cstdio>

static std::string formatValue(uint16_t sec)
{
    char b[8];
    std::snprintf(b, sizeof(b), "%02u:%02u", sec / 60u, sec % 60u);
    return std::string(b);
}

MainView::MainView()
    : mUpdateItemCb(this,       &MainView::updateItem)
    , mUpdateCenterItemCb(this, &MainView::updateCenterItem)
    , mAnimationEndedCb(this,   &MainView::onAnimationEnded)
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

    menu.setUpdateItemCallback(mUpdateItemCb);
    menu.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menu.setAnimationEndedCallback(mAnimationEndedCb);
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::setLists(const std::vector<Timer>& presets,
                        const std::vector<Timer>& recents)
{
    buildItems(presets, recents);

    menu.setNumberOfItems(static_cast<int16_t>(mItems.size()));
    menu.selectItem(0);
    menu.invalidate();

    updateTitle(0);
}

void MainView::buildItems(const std::vector<Timer>& presets,
                          const std::vector<Timer>& recents)
{
    mItems.clear();
    mLabels.clear();

    mItems.push_back({ Item::NEW, {}, false });
    for (const auto& p : presets) {
        mItems.push_back({ Item::VALUE, p, false });
    }
    for (const auto& r : recents) {
        mItems.push_back({ Item::VALUE, r, true });
    }

    // Build stable label strings (msgChar points into this vector).
    mLabels.reserve(mItems.size());
    for (const auto& it : mItems) {
        mLabels.push_back(it.kind == Item::NEW ? std::string("New")
                                               : formatValue(it.timer.durationSec));
    }

    // One config per item for both the surrounding and center renderers.
    mItemCfg.assign(mItems.size(), MenuItemConfig{});
    mCenterCfg.assign(mItems.size(), MenuItemConfig{});
    for (size_t i = 0; i < mItems.size(); ++i) {
        mItemCfg[i].style   = MenuItemConfig::SIMPLE;
        mItemCfg[i].msgChar = mLabels[i].c_str();

        mCenterCfg[i].style     = MenuItemConfig::SIMPLE;
        mCenterCfg[i].msgChar   = mLabels[i].c_str();
        mCenterCfg[i].msgIdType = (mItems[i].kind == Item::NEW) ? T_TMP_SEMIBOLD_35
                                                                : T_TMP_SEMIBOLD_60;
    }
}

void MainView::updateItem(MainMenuItem& item, int16_t index)
{
    if (index < 0 || index >= static_cast<int16_t>(mItemCfg.size())) {
        return;
    }
    item.apply(mItemCfg[index]);
}

void MainView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    if (index < 0 || index >= static_cast<int16_t>(mCenterCfg.size())) {
        return;
    }
    item.apply(mCenterCfg[index]);
}

void MainView::onAnimationEnded(int16_t index)
{
    updateTitle(index);
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
        menu.selectPrev();
    }
    else if (key == SDK::GUI::Button::L2) {
        menu.selectNext();
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
    int16_t idx = static_cast<int16_t>(menu.getSelectedItem());
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
