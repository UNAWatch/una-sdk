#include <gui/menudistance_screen/MenuDistanceView.hpp>

MenuDistanceView::MenuDistanceView() :
    mUpdateItemCb(this, &MenuDistanceView::updateItem),
    mUpdateCenterItemCb(this, &MenuDistanceView::updateCenterItem)
{
}

void MenuDistanceView::setupScreen()
{
    MenuDistanceViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setTitle(T_TEXT_DISTANCE_UC);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(Menu::ID_COUNT);
    menuLayout.invalidate();
}

void MenuDistanceView::tearDownScreen()
{
    MenuDistanceViewBase::tearDownScreen();
}

void MenuDistanceView::setDistanceUnits(Menu::Id id, bool isImperial)
{
    mIsImperial = isImperial;
    menuLayout.selectItem(static_cast<uint16_t>(id));
    menuLayout.invalidate();
}

void MenuDistanceView::formatItem(touchgfx::Unicode::UnicodeChar* buf, int16_t index)
{
    if (index == 0) {
        Unicode::snprintf(buf, kBuffSize, "%s",
            touchgfx::TypedText(T_TEXT_OFF_UC).getText());
    } else if (mIsImperial) {
        touchgfx::TypedTextId txt = Menu::kValues[index] > 1 ? T_TEXT_MILES : T_TEXT_MILE;
        Unicode::snprintf(buf, kBuffSize, "%d %s",
            Menu::kValues[index], touchgfx::TypedText(txt).getText());
    } else {
        Unicode::snprintf(buf, kBuffSize, "%d %s",
            Menu::kValues[index], touchgfx::TypedText(T_TEXT_KM).getText());
    }
}

void MenuDistanceView::updateItem(MainMenuItem& item, int16_t index)
{
    if (index < 0 || index >= static_cast<int16_t>(Menu::ID_COUNT)) return;

    formatItem(mItemBuff, index);

    MenuItemConfig cfg;
    cfg.msgText = mItemBuff;
    item.apply(cfg);
}

void MenuDistanceView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    if (index < 0 || index >= static_cast<int16_t>(Menu::ID_COUNT)) return;

    formatItem(mMainBuff, index);

    MenuItemConfig cfg;
    cfg.msgText = mMainBuff;
    item.apply(cfg);
}

void MenuDistanceView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
    }

    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
    }

    if (key == SDK::GUI::Button::R1) {
        presenter->saveDistance(static_cast<Menu::Id>(menuLayout.getSelectedItem()));
        application().gotoMenuDistanceSavedScreenNoTransition();
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoMenuAlertsScreenNoTransition();
    }
}
