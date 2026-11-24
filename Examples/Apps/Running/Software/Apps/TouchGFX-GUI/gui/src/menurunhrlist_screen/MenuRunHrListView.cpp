#include <gui/menurunhrlist_screen/MenuRunHrListView.hpp>
#include <gui/common/RunAlertsUi.hpp>

MenuRunHrListView::MenuRunHrListView() :
    mUpdateItemCb(this, &MenuRunHrListView::updateItem),
    mUpdateCenterItemCb(this, &MenuRunHrListView::updateCenterItem)
{
}

void MenuRunHrListView::setupScreen()
{
    MenuRunHrListViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(mRows);

    menuLayout.showSensorRow(true);
    setGpsFix(mGpsFix);

    menuLayout.invalidate();
}

void MenuRunHrListView::tearDownScreen()
{
    MenuRunHrListViewBase::tearDownScreen();
}

void MenuRunHrListView::setGpsFix(bool state)
{
    mGpsFix = state;
    menuLayout.setGps(state);
}

void MenuRunHrListView::setAccessoryStatus(uint8_t state, const char* /*name*/)
{
    menuLayout.setHr(state);
}

void MenuRunHrListView::setList(RunAlerts::Field field, bool empty, uint32_t lo, uint32_t hi, uint32_t seed)
{
    menuLayout.setTitle(RunAlertsUi::title(field));
    mMin  = lo;
    mRows = static_cast<int16_t>(empty ? 1u : hi - lo + 2u);
    menuLayout.setNumberOfItems(mRows);
    menuLayout.selectItem(empty ? 0 : static_cast<uint16_t>(seed - lo + 1u));
    menuLayout.invalidate();
}

void MenuRunHrListView::formatItem(touchgfx::Unicode::UnicodeChar* buf, int16_t index)
{
    if (index == 0) {
        Unicode::snprintf(buf, kBuffSize, "%s", touchgfx::TypedText(T_TEXT_OFF).getText());
    } else {
        Unicode::snprintf(buf, kBuffSize, "%u bpm", static_cast<unsigned>(mMin + index - 1));
    }
}

void MenuRunHrListView::updateItem(MainMenuItem& item, int16_t index)
{
    if (index < 0 || index >= mRows) return;

    formatItem(mItemBuff, index);

    MenuItemConfig cfg;
    cfg.msgText = mItemBuff;
    item.apply(cfg);
}

void MenuRunHrListView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    if (index < 0 || index >= mRows) return;

    formatItem(mMainBuff, index);

    MenuItemConfig cfg;
    cfg.msgText = mMainBuff;
    item.apply(cfg);
}

void MenuRunHrListView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
    }

    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
    }

    if (key == SDK::GUI::Button::R1) {
        const uint16_t row = menuLayout.getSelectedItem();
        presenter->save(row == 0 ? 0u : mMin + row - 1u);
        application().gotoMenuRunAlertSavedScreenNoTransition();
    }

    if (key == SDK::GUI::Button::R2) {
        application().gotoMenuRunLimitsScreenNoTransition();
    }
}
