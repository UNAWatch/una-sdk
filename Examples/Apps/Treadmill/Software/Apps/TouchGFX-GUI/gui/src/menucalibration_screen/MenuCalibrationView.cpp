#include <gui/menucalibration_screen/MenuCalibrationView.hpp>

MenuCalibrationView::MenuCalibrationView() :
    mUpdateItemCb(this, &MenuCalibrationView::updateItem),
    mUpdateCenterItemCb(this, &MenuCalibrationView::updateCenterItem)
{
}

void MenuCalibrationView::setupScreen()
{
    MenuCalibrationViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setTitle(T_TEXT_CALIBRATION_UC);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(Menu::ID_COUNT);

    // Treadmill has no GPS: no acquisition status line.
    menuLayout.setInfoMsg(TYPED_TEXT_INVALID);

    menuLayout.invalidate();
}

void MenuCalibrationView::tearDownScreen()
{
    MenuCalibrationViewBase::tearDownScreen();
}

void MenuCalibrationView::setPositionId(uint16_t id)
{
    menuLayout.selectItem(id);
}

uint16_t MenuCalibrationView::getPositionId()
{
    return menuLayout.getSelectedItem();
}

void MenuCalibrationView::updateItem(MainMenuItem& item, int16_t index)
{
    MenuItemConfig cfg;
    cfg.style = MenuItemConfig::SIMPLE;
    switch (index) {
    case Menu::ID_VIEW_DATA:  cfg.msgId = T_TEXT_VIEW_DATA;  break;
    case Menu::ID_CLEAR_DATA: cfg.msgId = T_TEXT_CLEAR_DATA; break;
    default: return;
    }
    item.apply(cfg);
}

void MenuCalibrationView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    MenuItemConfig cfg;
    cfg.style = MenuItemConfig::SIMPLE;
    switch (index) {
    case Menu::ID_VIEW_DATA:  cfg.msgId = T_TEXT_VIEW_DATA;  break;
    case Menu::ID_CLEAR_DATA: cfg.msgId = T_TEXT_CLEAR_DATA; break;
    default: return;
    }
    item.apply(cfg);
}

void MenuCalibrationView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
    }

    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
    }

    if (key == SDK::GUI::Button::R1) {
        switch (menuLayout.getSelectedItem()) {
        case Menu::ID_VIEW_DATA:
            application().gotoCalibrationDataScreenNoTransition();
            break;
        case Menu::ID_CLEAR_DATA:
            application().gotoCalibrationClearConfirmScreenNoTransition();
            break;
        default:
            break;
        }
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoMenuSettingsScreenNoTransition();
    }
}
