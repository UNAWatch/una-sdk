#include <gui/menurunwalk_screen/MenuRunWalkView.hpp>

MenuRunWalkView::MenuRunWalkView() :
    mUpdateItemCb(this, &MenuRunWalkView::updateItem),
    mUpdateCenterItemCb(this, &MenuRunWalkView::updateCenterItem)
{
}

void MenuRunWalkView::setupScreen()
{
    MenuRunWalkViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setTitle(T_TEXT_RUN_WALK_UC);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(Menu::ID_COUNT);

    menuLayout.showSensorRow(true);
    setGpsFix(mGpsFix);

    menuLayout.invalidate();
}

void MenuRunWalkView::tearDownScreen()
{
    MenuRunWalkViewBase::tearDownScreen();
}

void MenuRunWalkView::setGpsFix(bool state)
{
    mGpsFix = state;
    menuLayout.setGps(state);
}

void MenuRunWalkView::setAccessoryStatus(uint8_t state, const char* /*name*/)
{
    menuLayout.setHr(state);
}

void MenuRunWalkView::setPositionId(uint16_t id)
{
    menuLayout.selectItem(id);
}

uint16_t MenuRunWalkView::getPositionId()
{
    return menuLayout.getSelectedItem();
}

void MenuRunWalkView::refresh()
{
    const Settings::RunAlerts& ra = presenter->runAlerts();
    mOn = ra.runWalkEn;
    RunAlerts::fieldText(RunAlerts::Field::RunTime, ra, false, mRunTip, sizeof(mRunTip));
    RunAlerts::fieldText(RunAlerts::Field::WalkTime, ra, false, mWalkTip, sizeof(mWalkTip));
    menuLayout.invalidate();
}

void MenuRunWalkView::config(MenuItemConfig& cfg, int16_t index, bool center) const
{
    switch (index) {
    case Menu::ID_STATUS:
        // A plain on/off row: the label reads the state ("Off" / "On").
        if (center) {
            cfg.style       = MenuItemConfig::TOGGLE;
            cfg.msgId       = mOn ? T_TEXT_ON : T_TEXT_OFF;
            cfg.msgIdType   = T_TMP_SEMIBOLD_25;
            cfg.toggleState = mOn;
        } else {
            cfg.style    = MenuItemConfig::TIP;
            cfg.msgId    = mOn ? T_TEXT_ON : T_TEXT_OFF;
            cfg.tipId    = mOn ? T_TEXT_ON_UC : T_TEXT_OFF_UC;
            cfg.tipColor = mOn ? SDK::GUI::Color::YELLOW_DARK : SDK::GUI::Color::TEAL;
        }
        break;
    case Menu::ID_RUN_TIME:
    case Menu::ID_WALK_TIME:
        // As the Intervals times: "MM:SS min", in amber.
        cfg.style     = MenuItemConfig::TIP;
        cfg.msgId     = index == Menu::ID_RUN_TIME ? T_TEXT_RUN_TIME : T_TEXT_WALK_TIME;
        cfg.tipChar   = index == Menu::ID_RUN_TIME ? mRunTip : mWalkTip;
        if (center) {
            cfg.msgIdType = T_TMP_SEMIBOLD_25;
        } else {
            cfg.tipColor = SDK::GUI::Color::YELLOW_DARK;
        }
        break;
    default:
        break;
    }
}

void MenuRunWalkView::updateItem(MainMenuItem& item, int16_t index)
{
    MenuItemConfig cfg;
    config(cfg, index, false);
    item.apply(cfg);
}

void MenuRunWalkView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    MenuItemConfig cfg;
    config(cfg, index, true);
    item.apply(cfg);
}

void MenuRunWalkView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
    }

    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
    }

    if (key == SDK::GUI::Button::R1) {
        switch (menuLayout.getSelectedItem()) {
        case Menu::ID_STATUS:
            presenter->toggleRunWalk();
            refresh();
            break;
        case Menu::ID_RUN_TIME:
            presenter->edit(RunAlerts::Field::RunTime);
            application().gotoMenuRunAlertPickerScreenNoTransition();
            break;
        case Menu::ID_WALK_TIME:
            presenter->edit(RunAlerts::Field::WalkTime);
            application().gotoMenuRunAlertPickerScreenNoTransition();
            break;
        default:
            break;
        }
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoMenuRunAlertsScreenNoTransition();
    }
}
