#include <gui/main_screen/MainView.hpp>
#include <SDK/GUI/Button.hpp>

static void formatValue(touchgfx::Unicode::UnicodeChar* buf, uint16_t size, uint16_t sec)
{
    touchgfx::Unicode::snprintf(buf, size, "%02d:%02d", sec / 60, sec % 60);
}

MainView::MainView()
{
}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    buttons.setL1(Buttons::WHITE);
    buttons.setL2(Buttons::WHITE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);

    show();
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::setLists(const std::vector<Timer>& presets,
                        const std::vector<Timer>& recents)
{
    rebuildItems(presets, recents);

    if (mIndex >= mItems.size()) {
        mIndex = 0;
    }

    show();
}

void MainView::rebuildItems(const std::vector<Timer>& presets,
                            const std::vector<Timer>& recents)
{
    mItems.clear();

    mItems.push_back({ Item::NEW, {}, false });

    for (const auto& p : presets) {
        mItems.push_back({ Item::VALUE, p, false });
    }

    if (!recents.empty()) {
        mItems.push_back({ Item::DIVIDER, {}, false });
        for (const auto& r : recents) {
            mItems.push_back({ Item::VALUE, r, true });
        }
    }
}

size_t MainView::step(size_t index, bool forward) const
{
    size_t n = mItems.size();
    if (n == 0) {
        return 0;
    }

    size_t i = index;
    do {
        i = forward ? (i + 1) % n : (i + n - 1) % n;
    } while (mItems[i].kind == Item::DIVIDER);

    return i;
}

void MainView::fillLabel(touchgfx::TextAreaWithOneWildcard& area,
                         touchgfx::Unicode::UnicodeChar* buffer, uint16_t bufSize,
                         size_t index)
{
    const Item& it = mItems[index];
    switch (it.kind) {
        case Item::NEW:     touchgfx::Unicode::snprintf(buffer, bufSize, "New");    break;
        case Item::DIVIDER: touchgfx::Unicode::snprintf(buffer, bufSize, "Recent"); break;
        case Item::VALUE:   formatValue(buffer, bufSize, it.timer.durationSec);     break;
    }
    area.setWildcard(buffer);
    area.invalidate();
}

void MainView::show()
{
    if (mItems.empty()) {
        return;
    }

    const size_t n   = mItems.size();
    const Item&  cur = mItems[mIndex];
    const bool   isNew = cur.kind == Item::NEW;

    title.set((cur.kind == Item::VALUE && cur.isRecent) ? "RECENT" : "TIMER");

    icon.setVisible(isNew);
    newText.setVisible(isNew);
    currentValue.setVisible(!isNew);
    prevValue.setVisible(!isNew);
    nextValue.setVisible(!isNew);

    if (isNew) {
        touchgfx::Unicode::snprintf(newTextBuffer, NEWTEXT_SIZE, "New");
        newText.setWildcard(newTextBuffer);
    } else {
        formatValue(currentValueBuffer, CURRENTVALUE_SIZE, cur.timer.durationSec);
        currentValue.setWildcard(currentValueBuffer);

        fillLabel(prevValue, prevValueBuffer, PREVVALUE_SIZE, (mIndex + n - 1) % n);
        fillLabel(nextValue, nextValueBuffer, NEXTVALUE_SIZE, (mIndex + 1) % n);
    }

    icon.invalidate();
    newText.invalidate();
    currentValue.invalidate();
    prevValue.invalidate();
    nextValue.invalidate();
}

void MainView::handleKeyEvent(uint8_t key)
{
    if (mItems.empty()) {
        return;
    }

    if (key == SDK::GUI::Button::L1) {
        mIndex = step(mIndex, false);
        show();
    }
    else if (key == SDK::GUI::Button::L2) {
        mIndex = step(mIndex, true);
        show();
    }
    else if (key == SDK::GUI::Button::R1) {
        const Item& cur = mItems[mIndex];
        if (cur.kind == Item::NEW) {
            presenter->editNew();
            application().gotoEditScreenNoTransition();
        }
        else if (cur.kind == Item::VALUE) {
            presenter->selectTimer(cur.timer);
            application().gotoMenuScreenNoTransition();
        }
    }
    else if (key == SDK::GUI::Button::R2) {
        presenter->exitApp();
    }
}
