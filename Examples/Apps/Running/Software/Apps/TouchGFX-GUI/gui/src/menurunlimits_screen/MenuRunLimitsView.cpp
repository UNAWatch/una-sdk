#include <gui/menurunlimits_screen/MenuRunLimitsView.hpp>

MenuRunLimitsView::MenuRunLimitsView() :
    mUpdateItemCb(this, &MenuRunLimitsView::updateItem),
    mUpdateCenterItemCb(this, &MenuRunLimitsView::updateCenterItem)
{
}

void MenuRunLimitsView::setupScreen()
{
    MenuRunLimitsViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(kRows);

    menuLayout.showSensorRow(true);
    setGpsFix(mGpsFix);

    menuLayout.invalidate();
}

void MenuRunLimitsView::tearDownScreen()
{
    MenuRunLimitsViewBase::tearDownScreen();
}

void MenuRunLimitsView::setGpsFix(bool state)
{
    mGpsFix = state;
    menuLayout.setGps(state);
}

void MenuRunLimitsView::setAccessoryStatus(uint8_t state, const char* /*name*/)
{
    menuLayout.setHr(state);
}

void MenuRunLimitsView::setPositionId(uint16_t id)
{
    menuLayout.selectItem(id);
}

uint16_t MenuRunLimitsView::getPositionId()
{
    return menuLayout.getSelectedItem();
}

RunAlerts::Field MenuRunLimitsView::field(int16_t row) const
{
    if (mPace) {
        return row == 0 ? RunAlerts::Field::PaceFast : RunAlerts::Field::PaceSlow;
    }
    return row == 0 ? RunAlerts::Field::HrHigh : RunAlerts::Field::HrLow;
}

void MenuRunLimitsView::refresh()
{
    mPace = RunAlerts::isPace(presenter->field());
    menuLayout.setTitle(mPace ? T_TEXT_PACE_UC : T_TEXT_HEART_RATE_UC);

    const Settings::RunAlerts& ra = presenter->runAlerts();
    const bool imperial = presenter->isImperial();
    for (int16_t row = 0; row < kRows; ++row) {
        const RunAlerts::Field f = field(row);
        mOn[row] = RunAlerts::fieldValue(f, ra, imperial) > 0;
        RunAlerts::fieldText(f, ra, imperial, mTips[row], sizeof(mTips[row]));
    }
    menuLayout.invalidate();
}

void MenuRunLimitsView::config(MenuItemConfig& cfg, int16_t index, bool center) const
{
    if (index < 0 || index >= kRows) {
        return;
    }
    cfg.style = MenuItemConfig::TIP;
    if (mPace) {
        cfg.msgId = index == 0 ? T_TEXT_TOO_FAST : T_TEXT_TOO_SLOW;
    } else {
        cfg.msgId = index == 0 ? T_TEXT_HIGH : T_TEXT_LOW;
    }
    // OFF in teal, a limit in amber, as Lap Alerts.
    if (mOn[index]) {
        cfg.tipChar = mTips[index];
    } else {
        cfg.tipId = T_TEXT_OFF_UC;
    }
    if (!center) {
        cfg.tipColor = mOn[index] ? SDK::GUI::Color::YELLOW_DARK : SDK::GUI::Color::TEAL;
    }
}

void MenuRunLimitsView::updateItem(MainMenuItem& item, int16_t index)
{
    MenuItemConfig cfg;
    config(cfg, index, false);
    item.apply(cfg);
}

void MenuRunLimitsView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    MenuItemConfig cfg;
    config(cfg, index, true);
    item.apply(cfg);
}

void MenuRunLimitsView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
    }

    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
    }

    if (key == SDK::GUI::Button::R1) {
        presenter->edit(field(static_cast<int16_t>(menuLayout.getSelectedItem())));
        if (mPace) {
            application().gotoMenuRunPaceLimitScreenNoTransition();
        } else {
            application().gotoMenuRunHrListScreenNoTransition();
        }
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoMenuRunAlertsScreenNoTransition();
    }
}
