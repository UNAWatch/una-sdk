#include <gui/menusettings_screen/MenuSettingsView.hpp>

MenuSettingsView::MenuSettingsView() :
    mUpdateItemCb(this, &MenuSettingsView::updateItem),
    mUpdateCenterItemCb(this, &MenuSettingsView::updateCenterItem)
{
}

void MenuSettingsView::setupScreen()
{
    MenuSettingsViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setTitle(T_TEXT_SETTINGS_UC);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(Menu::ID_COUNT);

    setGpsFix(mGpsFix);

    menuLayout.invalidate();
}

void MenuSettingsView::tearDownScreen()
{
    MenuSettingsViewBase::tearDownScreen();
}

void MenuSettingsView::setGpsFix(bool state)
{
    mGpsFix = state;
    menuLayout.setInfoMsg(state ? T_TEXT_SIGNAL_ACQUIRED : T_TEXT_ACQUIRING_SIGNAL);
}

void MenuSettingsView::setAutoPause(bool state)
{
    mAutoPause = state;
    menuLayout.invalidate();
}

void MenuSettingsView::setPhoneNotif(bool state)
{
    mPhoneNotif = state;
    menuLayout.invalidate();
}

void MenuSettingsView::setPositionId(uint16_t id)
{
    menuLayout.selectItem(id);
}

uint16_t MenuSettingsView::getPositionId()
{
    return menuLayout.getSelectedItem();
}

void MenuSettingsView::updateItem(MainMenuItem& item, int16_t index)
{
    MenuItemConfig cfg;

    switch (index) {
    case Menu::ID_ALERTS:
        cfg.style = MenuItemConfig::SIMPLE;
        cfg.msgId = T_TEXT_LAP_ALERTS;
        break;
    case Menu::ID_AUTO_PAUSE:
        cfg.style    = MenuItemConfig::TIP;
        cfg.msgId    = T_TEXT_AUTO_PAUSE;
        cfg.tipId    = mAutoPause ? T_TEXT_ON_UC : T_TEXT_OFF_UC;
        cfg.tipColor = mAutoPause ? SDK::GUI::Color::YELLOW_DARK
                                  : SDK::GUI::Color::TEAL;
        break;
    case Menu::ID_PHONE_NOTIF:
        cfg.style    = MenuItemConfig::TIP;
        cfg.msgId    = T_TEXT_PHONE_NOTIF_DOT;
        cfg.tipId    = mPhoneNotif ? T_TEXT_ON_UC : T_TEXT_OFF_UC;
        cfg.tipColor = mPhoneNotif ? SDK::GUI::Color::YELLOW_DARK
                                   : SDK::GUI::Color::TEAL;
        break;
    default:
        return;
    }

    item.apply(cfg);
}

void MenuSettingsView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    MenuItemConfig cfg;

    switch (index) {
    case Menu::ID_ALERTS:
        cfg.style = MenuItemConfig::SIMPLE;
        cfg.msgId = T_TEXT_LAP_ALERTS;
        break;
    case Menu::ID_AUTO_PAUSE:
        cfg.style       = MenuItemConfig::TOGGLE;
        cfg.msgId       = T_TEXT_AUTO_BR_PAUSE;
        cfg.msgIdType   = T_TMP_SEMIBOLD_25;
        cfg.toggleState = mAutoPause;
        break;
    case Menu::ID_PHONE_NOTIF:
        cfg.style       = MenuItemConfig::TOGGLE;
        cfg.msgId       = T_TEXT_PHONE_BR_NOTIF_DOT;
        cfg.msgIdType   = T_TMP_SEMIBOLD_25;
        cfg.toggleState = mPhoneNotif;
        break;
    default:
        return;
    }

    item.apply(cfg);
}

void MenuSettingsView::handleKeyEvent(uint8_t key)
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
        case Menu::ID_ALERTS:
            application().gotoMenuAlertsScreenNoTransition();
            break;
        case Menu::ID_AUTO_PAUSE:
            mAutoPause = !mAutoPause;
            setAutoPause(mAutoPause);
            presenter->saveAutoPause(mAutoPause);
            break;
        case Menu::ID_PHONE_NOTIF:
            mPhoneNotif = !mPhoneNotif;
            setPhoneNotif(mPhoneNotif);
            presenter->savePhoneNotif(mPhoneNotif);
            break;
        default:
            break;
        }
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoMainScreenNoTransition();
    }
}
