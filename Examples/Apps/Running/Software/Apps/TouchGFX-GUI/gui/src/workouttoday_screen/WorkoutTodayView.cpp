#include <gui/workouttoday_screen/WorkoutTodayView.hpp>
#include <gui/common/WorkoutUi.hpp>

namespace
{
const TypedTextId kItems[App::MenuNav::Root::Workouts::Menu::ID_COUNT] = {
    T_TEXT_START,
    T_TEXT_DETAILS,
    T_TEXT_SKIP,
};
} // namespace

WorkoutTodayView::WorkoutTodayView() :
    mUpdateItemCb(this, &WorkoutTodayView::updateItem),
    mUpdateCenterItemCb(this, &WorkoutTodayView::updateCenterItem)
{
}

void WorkoutTodayView::setupScreen()
{
    WorkoutTodayViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setTitle(T_TEXT_TODAY_UC);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(Menu::ID_COUNT);
    menuLayout.getButtons().setR1(Buttons::AMBER);
    menuLayout.getButtons().setR2(Buttons::WHITE);
    menuLayout.invalidate();
}

void WorkoutTodayView::tearDownScreen()
{
    WorkoutTodayViewBase::tearDownScreen();
}

void WorkoutTodayView::setWorkout(const SDK::Workout::Library::Entry& entry, bool imperial)
{
    WorkoutUi::toText(entry.name, mName, sizeof(mName) / sizeof(mName[0]));
    WorkoutUi::fitWidth(mName, T_TMP_ITALIC_18, 170);
    menuLayout.setInfoText(mName);

    char totals[32];
    WorkoutUi::formatTotals(entry, imperial, totals, sizeof(totals));
    touchgfx::Unicode::strncpy(mTotals, totals, sizeof(mTotals) / sizeof(mTotals[0]) - 1);
    menuLayout.invalidate();
}

void WorkoutTodayView::setPositionId(uint16_t id)
{
    menuLayout.selectItem(id);
}

uint16_t WorkoutTodayView::getPositionId()
{
    return menuLayout.getSelectedItem();
}

void WorkoutTodayView::updateItem(MainMenuItem& item, int16_t index)
{
    if (index < 0 || index >= Menu::ID_COUNT) {
        return;
    }
    MenuItemConfig cfg;
    cfg.msgId = kItems[index];
    item.apply(cfg);
}

void WorkoutTodayView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    if (index < 0 || index >= Menu::ID_COUNT) {
        return;
    }
    MenuItemConfig cfg;
    cfg.msgId = kItems[index];
    if (index == Menu::ID_START && mTotals[0] != 0) {
        cfg.style   = MenuItemConfig::TIP;
        cfg.tipText = mTotals;
    }
    item.apply(cfg);
}

void WorkoutTodayView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
    }
    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
    }
    if (key == SDK::GUI::Button::R1) {
        switch (menuLayout.getSelectedItem()) {
        case Menu::ID_START:
            presenter->start();
            application().gotoMainScreenNoTransition();
            break;
        case Menu::ID_DETAILS:
            presenter->showDetails();
            application().gotoWorkoutDetailsScreenNoTransition();
            break;
        default:
            application().gotoMainScreenNoTransition();
            break;
        }
    }
    if (key == SDK::GUI::Button::R2) {
        application().gotoMainScreenNoTransition();
    }
}
