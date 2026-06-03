#include <gui/menuintervalsrepeats_screen/MenuIntervalsRepeatsView.hpp>

MenuIntervalsRepeatsView::MenuIntervalsRepeatsView() :
    mUpdateItemCb(this, &MenuIntervalsRepeatsView::updateItem),
    mUpdateCenterItemCb(this, &MenuIntervalsRepeatsView::updateCenterItem)
{

}

void MenuIntervalsRepeatsView::setupScreen()
{
    MenuIntervalsRepeatsViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setTitle(T_TEXT_REPEATS_UC);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(Menu::kMaxCount);
    menuLayout.invalidate();
}

void MenuIntervalsRepeatsView::tearDownScreen()
{
    MenuIntervalsRepeatsViewBase::tearDownScreen();
}

void MenuIntervalsRepeatsView::formatItem(touchgfx::Unicode::UnicodeChar* buf, int16_t index)
{
    if (index == 0) {
        Unicode::snprintf(buf, kBuffSize, "%s", touchgfx::TypedText(T_TEXT_OPEN).getText());
    } else {
        Unicode::snprintf(buf, kBuffSize, "x%u", static_cast<uint16_t>(index));
    }
}

void MenuIntervalsRepeatsView::updateItem(MainMenuItem& item, int16_t index)
{
    if (index < 0 || index >= static_cast<int16_t>(Menu::kMaxCount)) return;

    formatItem(mItemBuff, index);

    MenuItemConfig cfg;
    cfg.msgText = mItemBuff;
    item.apply(cfg);
}

void MenuIntervalsRepeatsView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    if (index < 0 || index >= static_cast<int16_t>(Menu::kMaxCount)) return;

    formatItem(mMainBuff, index);

    MenuItemConfig cfg;
    cfg.msgText = mMainBuff;
    item.apply(cfg);
}

void MenuIntervalsRepeatsView::setPositionId(uint16_t id)
{
    menuLayout.selectItem(id);
}

uint16_t MenuIntervalsRepeatsView::getPositionId()
{
    return menuLayout.getSelectedItem();
}

void MenuIntervalsRepeatsView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
    }

    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
    }

    if (key == SDK::GUI::Button::R1) {
        uint8_t repeatsNum = static_cast<uint8_t>(menuLayout.getSelectedItem());
        presenter->saveRepeats(repeatsNum);
        application().gotoMenuIntervalsScreenNoTransition();
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoMenuIntervalsScreenNoTransition();
    }
}
