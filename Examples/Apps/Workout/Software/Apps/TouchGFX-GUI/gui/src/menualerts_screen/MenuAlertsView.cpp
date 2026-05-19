#include <gui/menualerts_screen/MenuAlertsView.hpp>

MenuAlertsView::MenuAlertsView() :
    mUpdateItemCb(this, &MenuAlertsView::updateItem),
    mUpdateCenterItemCb(this, &MenuAlertsView::updateCenterItem)
{
}

void MenuAlertsView::setupScreen()
{
    MenuAlertsViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setTitle(T_TEXT_LAP_ALERTS_UC);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(Menu::ID_COUNT);

    formatTips();
    menuLayout.invalidate();
}

void MenuAlertsView::tearDownScreen()
{
    MenuAlertsViewBase::tearDownScreen();
}

void MenuAlertsView::setTime(Time::Id id)
{
    mTimeId = id;
    formatTips();
    menuLayout.invalidate();
}

void MenuAlertsView::setPositionId(uint16_t id)
{
    menuLayout.selectItem(id);
}

uint16_t MenuAlertsView::getPositionId()
{
    return menuLayout.getSelectedItem();
}

void MenuAlertsView::formatTips()
{
    if (mTimeId == Time::ID_OFF) {
        Unicode::snprintf(mTimeTipBuff, kBuffSize, "%s",
            touchgfx::TypedText(T_TEXT_OFF_UC).getText());
        mTimeTipColor = SDK::GUI::Color::TEAL;
    } else {
        Unicode::snprintf(mTimeTipBuff, kBuffSize, "%d %s",
            Time::kValues[mTimeId], touchgfx::TypedText(T_TEXT_MIN).getText());
        mTimeTipColor = SDK::GUI::Color::YELLOW_DARK;
    }
}

void MenuAlertsView::updateItem(MainMenuItem& item, int16_t index)
{
    MenuItemConfig cfg;
    cfg.style = MenuItemConfig::TIP;

    switch (index) {
    case Menu::ID_TIME:
        cfg.msgId    = T_TEXT_TIME;
        cfg.tipText  = mTimeTipBuff;
        cfg.tipColor = mTimeTipColor;
        break;
    default:
        return;
    }

    item.apply(cfg);
}

void MenuAlertsView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    MenuItemConfig cfg;
    cfg.style = MenuItemConfig::TIP;

    switch (index) {
    case Menu::ID_TIME:
        cfg.msgId   = T_TEXT_TIME;
        cfg.tipText = mTimeTipBuff;
        break;
    default:
        return;
    }

    item.apply(cfg);
}

void MenuAlertsView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
    }

    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
    }

    if (key == SDK::GUI::Button::R1) {
        uint32_t id = menuLayout.getSelectedItem();

        switch (id) {
        case Menu::ID_TIME:
            application().gotoMenuTimeScreenNoTransition();
            break;
        default:
            break;
        }
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoMenuSettingsScreenNoTransition();
    }
}
