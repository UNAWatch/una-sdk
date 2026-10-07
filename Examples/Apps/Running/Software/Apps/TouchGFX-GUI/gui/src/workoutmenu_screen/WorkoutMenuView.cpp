#include <gui/workoutmenu_screen/WorkoutMenuView.hpp>
#include <gui/common/WorkoutUi.hpp>

WorkoutMenuView::WorkoutMenuView() :
    mUpdateItemCb(this, &WorkoutMenuView::updateItem),
    mUpdateCenterItemCb(this, &WorkoutMenuView::updateCenterItem)
{
}

void WorkoutMenuView::setupScreen()
{
    WorkoutMenuViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setNumberOfItems(kCount);
    menuLayout.getButtons().setR1(Buttons::AMBER);
    menuLayout.getButtons().setR2(Buttons::WHITE);
    menuLayout.invalidate();
}

void WorkoutMenuView::tearDownScreen()
{
    WorkoutMenuViewBase::tearDownScreen();
}

void WorkoutMenuView::setWorkout(const SDK::Workout::Library::Entry& entry, const char* group, bool imperial)
{
    menuLayout.setTitle(group);

    WorkoutUi::toText(entry.name, mName, sizeof(mName) / sizeof(mName[0]));
    WorkoutUi::fitWidth(mName, T_TMP_ITALIC_18, 170);
    menuLayout.setInfoText(mName);

    char totals[32];
    WorkoutUi::formatTotals(entry, imperial, totals, sizeof(totals));
    touchgfx::Unicode::strncpy(mTotals, totals, sizeof(mTotals) / sizeof(mTotals[0]) - 1);
    menuLayout.invalidate();
}

void WorkoutMenuView::setPositionId(uint16_t id)
{
    menuLayout.selectItem(id < kCount ? id : 0);
}

uint16_t WorkoutMenuView::getPositionId()
{
    return menuLayout.getSelectedItem();
}

void WorkoutMenuView::updateItem(MainMenuItem& item, int16_t index)
{
    if (index < 0 || index >= kCount) {
        return;
    }
    MenuItemConfig cfg;
    cfg.msgId = index == Menu::ID_START ? T_TEXT_START : T_TEXT_DETAILS;
    item.apply(cfg);
}

void WorkoutMenuView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    if (index < 0 || index >= kCount) {
        return;
    }
    MenuItemConfig cfg;
    cfg.msgId = index == Menu::ID_START ? T_TEXT_START : T_TEXT_DETAILS;
    if (index == Menu::ID_START && mTotals[0] != 0) {
        cfg.style   = MenuItemConfig::TIP;
        cfg.tipText = mTotals;
    }
    item.apply(cfg);
}

void WorkoutMenuView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
    }
    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
    }
    if (key == SDK::GUI::Button::R1) {
        if (menuLayout.getSelectedItem() == Menu::ID_START) {
            presenter->start();
            application().gotoMainScreenNoTransition();
        } else {
            presenter->showDetails();
            application().gotoWorkoutDetailsScreenNoTransition();
        }
    }
    if (key == SDK::GUI::Button::R2) {
        application().gotoWorkoutListScreenNoTransition();
    }
}
