#include <gui/menurunalerts_screen/MenuRunAlertsView.hpp>

MenuRunAlertsView::MenuRunAlertsView() :
    mUpdateItemCb(this, &MenuRunAlertsView::updateItem),
    mUpdateCenterItemCb(this, &MenuRunAlertsView::updateCenterItem)
{
}

void MenuRunAlertsView::setupScreen()
{
    MenuRunAlertsViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setTitle(T_TEXT_RUN_ALERTS_UC);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(Menu::ID_COUNT);

    menuLayout.showSensorRow(true);
    setGpsFix(mGpsFix);

    menuLayout.invalidate();
}

void MenuRunAlertsView::tearDownScreen()
{
    MenuRunAlertsViewBase::tearDownScreen();
}

void MenuRunAlertsView::setGpsFix(bool state)
{
    mGpsFix = state;
    menuLayout.setGps(state);
}

void MenuRunAlertsView::setAccessoryStatus(uint8_t state, const char* /*name*/)
{
    menuLayout.setHr(state);
}

void MenuRunAlertsView::setPositionId(uint16_t id)
{
    menuLayout.selectItem(id);
}

uint16_t MenuRunAlertsView::getPositionId()
{
    return menuLayout.getSelectedItem();
}

void MenuRunAlertsView::refresh()
{
    const Settings::RunAlerts& ra = presenter->runAlerts();
    mRunWalkOn = ra.runWalkEn;
    mBackInTap = ra.backInTap;
    RunAlerts::paceRangeText(ra, presenter->isImperial(), mPaceTip, sizeof(mPaceTip));
    RunAlerts::hrRangeText(ra, mHrTip, sizeof(mHrTip));
    menuLayout.invalidate();
}

void MenuRunAlertsView::config(MenuItemConfig& cfg, int16_t index, bool center) const
{
    // As Lap Alerts: OFF in teal, a setting in amber (the centre row keeps
    // its own hint colour).
    const auto tip = [&](bool on, const char* text) {
        cfg.style = MenuItemConfig::TIP;
        if (on && text != nullptr) {
            cfg.tipChar = text;
        } else {
            cfg.tipId = on ? T_TEXT_ON_UC : T_TEXT_OFF_UC;
        }
        if (!center) {
            cfg.tipColor = on ? SDK::GUI::Color::YELLOW_DARK : SDK::GUI::Color::TEAL;
        }
    };

    switch (index) {
    case Menu::ID_RUN_WALK:
        cfg.msgId = T_TEXT_RUN_WALK;
        tip(mRunWalkOn, nullptr);
        break;
    case Menu::ID_PACE:
        cfg.msgId = T_TEXT_PACE;
        tip(mPaceTip[0] != '\0', mPaceTip);
        break;
    case Menu::ID_HEART_RATE:
        cfg.msgId = T_TEXT_HEART_RATE;
        tip(mHrTip[0] != '\0', mHrTip);
        break;
    case Menu::ID_BACK_IN:
        if (center) {
            cfg.style       = MenuItemConfig::TOGGLE;
            cfg.msgId       = T_TEXT_BACK_BR_IN_RANGE;
            cfg.msgIdType   = T_TMP_SEMIBOLD_25;
            cfg.toggleState = mBackInTap;
        } else {
            cfg.msgId = T_TEXT_BACK_IN_RANGE;
            tip(mBackInTap, nullptr);
        }
        break;
    default:
        break;
    }
}

void MenuRunAlertsView::updateItem(MainMenuItem& item, int16_t index)
{
    MenuItemConfig cfg;
    config(cfg, index, false);
    item.apply(cfg);
}

void MenuRunAlertsView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    MenuItemConfig cfg;
    config(cfg, index, true);
    item.apply(cfg);
}

void MenuRunAlertsView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
    }

    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
    }

    if (key == SDK::GUI::Button::R1) {
        switch (menuLayout.getSelectedItem()) {
        case Menu::ID_RUN_WALK:
            application().gotoMenuRunWalkScreenNoTransition();
            break;
        case Menu::ID_PACE:
            presenter->openLimits(RunAlerts::Field::PaceFast);
            application().gotoMenuRunLimitsScreenNoTransition();
            break;
        case Menu::ID_HEART_RATE:
            presenter->openLimits(RunAlerts::Field::HrHigh);
            application().gotoMenuRunLimitsScreenNoTransition();
            break;
        case Menu::ID_BACK_IN:
            presenter->toggleBackInTap();
            refresh();
            break;
        default:
            break;
        }
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoMenuSettingsScreenNoTransition();
    }
}
