#include <gui/menutime_screen/MenuTimeView.hpp>

MenuTimeView::MenuTimeView() :
    mUpdateItemCb(this, &MenuTimeView::updateItem),
    mUpdateCenterItemCb(this, &MenuTimeView::updateCenterItem)
{
}

void MenuTimeView::setupScreen()
{
    MenuTimeViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setTitle(T_TEXT_TIME_UC);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(Menu::ID_COUNT);
    menuLayout.invalidate();
}

void MenuTimeView::tearDownScreen()
{
    MenuTimeViewBase::tearDownScreen();
}

void MenuTimeView::setTime(Menu::Id id)
{
    menuLayout.selectItem(static_cast<uint16_t>(id));
}

void MenuTimeView::updateItem(MainMenuItem& item, int16_t index)
{
    if (index < 0 || index >= static_cast<int16_t>(Menu::ID_COUNT)) return;

    if (index == 0) {
        Unicode::snprintf(mItemBuff, kBuffSize, "%s",
            touchgfx::TypedText(T_TEXT_OFF_UC).getText());
    } else {
        Unicode::snprintf(mItemBuff, kBuffSize, "%d %s",
            Menu::kValues[index], touchgfx::TypedText(T_TEXT_MIN).getText());
    }

    MenuItemConfig cfg;
    cfg.msgText = mItemBuff;
    item.apply(cfg);
}

void MenuTimeView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    if (index < 0 || index >= static_cast<int16_t>(Menu::ID_COUNT)) return;

    if (index == 0) {
        Unicode::snprintf(mMainBuff, kBuffSize, "%s",
            touchgfx::TypedText(T_TEXT_OFF_UC).getText());
    } else {
        touchgfx::TypedTextId txt = Menu::kValues[index] > 1 ? T_TEXT_MINUTES_LC : T_TEXT_MINUTE_LC;
        Unicode::snprintf(mMainBuff, kBuffSize, "%d %s",
            Menu::kValues[index], touchgfx::TypedText(txt).getText());
    }

    MenuItemConfig cfg;
    cfg.msgText = mMainBuff;
    item.apply(cfg);
}

void MenuTimeView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
    }

    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
    }

    if (key == SDK::GUI::Button::R1) {
        presenter->saveTime(static_cast<Menu::Id>(menuLayout.getSelectedItem()));
        application().gotoMenuTimeSavedScreenNoTransition();
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoMenuAlertsScreenNoTransition();
    }
}
