#include <gui/menurunpacelimit_screen/MenuRunPaceLimitView.hpp>
#include <gui/common/RunAlertsUi.hpp>

MenuRunPaceLimitView::MenuRunPaceLimitView() :
    mUpdateItemCb(this, &MenuRunPaceLimitView::updateItem),
    mUpdateCenterItemCb(this, &MenuRunPaceLimitView::updateCenterItem)
{
}

void MenuRunPaceLimitView::setupScreen()
{
    MenuRunPaceLimitViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(Menu::ID_COUNT);

    menuLayout.showSensorRow(true);
    setGpsFix(mGpsFix);

    menuLayout.invalidate();
}

void MenuRunPaceLimitView::tearDownScreen()
{
    MenuRunPaceLimitViewBase::tearDownScreen();
}

void MenuRunPaceLimitView::setGpsFix(bool state)
{
    mGpsFix = state;
    menuLayout.setGps(state);
}

void MenuRunPaceLimitView::setAccessoryStatus(uint8_t state, const char* /*name*/)
{
    menuLayout.setHr(state);
}

void MenuRunPaceLimitView::refresh()
{
    const RunAlerts::Field     f        = presenter->field();
    const Settings::RunAlerts& ra       = presenter->runAlerts();
    const bool                 imperial = presenter->isImperial();
    const bool                 on       = RunAlerts::fieldValue(f, ra, imperial) > 0;

    menuLayout.setTitle(RunAlertsUi::title(f));
    // The Pace row shows the limit while it is set; nothing when Off.
    if (on) {
        RunAlerts::fieldText(f, ra, imperial, mPaceTip, sizeof(mPaceTip));
    } else {
        mPaceTip[0] = '\0';
    }
    menuLayout.selectItem(on ? Menu::ID_PACE : Menu::ID_OFF);
    menuLayout.invalidate();
}

void MenuRunPaceLimitView::config(MenuItemConfig& cfg, int16_t index, bool center) const
{
    switch (index) {
    case Menu::ID_OFF:
        cfg.style = MenuItemConfig::SIMPLE;
        cfg.msgId = T_TEXT_OFF;
        break;
    case Menu::ID_PACE:
        cfg.msgId = T_TEXT_PACE;
        if (mPaceTip[0] != '\0') {
            cfg.style   = MenuItemConfig::TIP;
            cfg.tipChar = mPaceTip;
            if (!center) {
                cfg.tipColor = SDK::GUI::Color::YELLOW_DARK;
            }
        } else {
            cfg.style = MenuItemConfig::SIMPLE;
        }
        break;
    default:
        break;
    }
}

void MenuRunPaceLimitView::updateItem(MainMenuItem& item, int16_t index)
{
    MenuItemConfig cfg;
    config(cfg, index, false);
    item.apply(cfg);
}

void MenuRunPaceLimitView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    MenuItemConfig cfg;
    config(cfg, index, true);
    item.apply(cfg);
}

void MenuRunPaceLimitView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
    }

    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
    }

    if (key == SDK::GUI::Button::R1) {
        if (menuLayout.getSelectedItem() == Menu::ID_OFF) {
            presenter->save(0);
            application().gotoMenuRunAlertSavedScreenNoTransition();
        } else if (presenter->canPick()) {
            // Nothing to pick when the other limit leaves no room (Too
            // Slow at 2:00, say): the row then does nothing.
            application().gotoMenuRunAlertPickerScreenNoTransition();
        }
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoMenuRunLimitsScreenNoTransition();
    }
}
