#include <gui/workoutlist_screen/WorkoutListView.hpp>
#include <gui/common/WorkoutUi.hpp>

#include <cstdio>

WorkoutListView::WorkoutListView() :
    mUpdateItemCb(this, &WorkoutListView::updateItem),
    mUpdateCenterItemCb(this, &WorkoutListView::updateCenterItem),
    mAnimationMiddleCb(this, &WorkoutListView::onAnimationMiddle)
{
}

void WorkoutListView::setupScreen()
{
    WorkoutListViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setTitle(T_TEXT_WORKOUTS_UC);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setAnimationMiddleCallback(mAnimationMiddleCb);
    menuLayout.getButtons().setR1(Buttons::AMBER);
    menuLayout.getButtons().setR2(Buttons::WHITE);

    // Shown instead of the list when there are no workout files.
    mEmpty1.init(T_TMP_SEMIBOLD_25_L, SDK::GUI::Color::WHITE);
    mEmpty2.init(T_TMP_MEDIUM_18_L, SDK::GUI::Color::GRAY);
    mEmpty3.init(T_TMP_MEDIUM_18_L, SDK::GUI::Color::GRAY);
    mEmpty1.setAnchor(120, 104);
    mEmpty2.setAnchor(120, 142);
    mEmpty3.setAnchor(120, 165);
    mEmpty1.setText(T_TEXT_NO_WORKOUTS);
    mEmpty2.setText(T_TEXT_COPY_FIT_FILES);
    mEmpty3.setText(T_TEXT_THE_WORKOUTS_FOLDER);
    add(mEmpty1);
    add(mEmpty2);
    add(mEmpty3);
}

void WorkoutListView::tearDownScreen()
{
    WorkoutListViewBase::tearDownScreen();
}

void WorkoutListView::setList(const SDK::Workout::Library* library, bool freeRunRow, bool imperial,
                              uint32_t today)
{
    mToday      = today;
    mLibrary    = library;
    mFreeRunRow = freeRunRow;
    mImperial   = imperial;

    const int16_t workouts = mLibrary != nullptr ? static_cast<int16_t>(mLibrary->size()) : 0;
    const int16_t rows     = static_cast<int16_t>(workouts + (mFreeRunRow ? 1 : 0));
    const bool    empty    = rows == 0;

    menuLayout.getMenu().setVisible(!empty);
    menuLayout.showBackground(!empty);
    menuLayout.getButtons().setR1(empty ? Buttons::NONE : Buttons::AMBER);
    mEmpty1.setVisible(empty);
    mEmpty2.setVisible(empty);
    mEmpty3.setVisible(empty);
    mEmpty1.invalidate();
    mEmpty2.invalidate();
    mEmpty3.invalidate();

    if (!empty) {
        menuLayout.setNumberOfItems(rows);
    }
    menuLayout.invalidate();
}

void WorkoutListView::setPositionId(uint16_t id)
{
    if (menuLayout.getMenu().isVisible()) {
        menuLayout.selectItem(id < menuLayout.getNumberOfItems() ? id : 0);
        showGroup(static_cast<int16_t>(menuLayout.getSelectedItem()));
    } else {
        menuLayout.setInfoText(nullptr);
    }
}

uint16_t WorkoutListView::getPositionId()
{
    return menuLayout.getSelectedItem();
}

int16_t WorkoutListView::workoutAt(int16_t row) const
{
    return mFreeRunRow ? static_cast<int16_t>(row - 1) : row;
}

void WorkoutListView::groupOf(int16_t row, touchgfx::Unicode::UnicodeChar* buf, uint16_t cap) const
{
    buf[0] = 0;
    const int16_t i = workoutAt(row);
    if (mLibrary == nullptr || i < 0 || static_cast<size_t>(i) >= mLibrary->size()) {
        return;
    }
    const SDK::Workout::Library::Entry& e = mLibrary->entry(static_cast<size_t>(i));
    if (e.date != 0 && e.date == mToday) {
        touchgfx::Unicode::snprintf(buf, cap, "%s", touchgfx::TypedText(T_TEXT_TODAY).getText());
    } else if (e.date != 0) {
        char date[16];
        WorkoutUi::formatDate(e.date, date, sizeof(date));
        // "SAT 10 OCT" reads as "Sat 10 Oct" in a sentence.
        for (char* p = date + 1; *p != '\0'; ++p) {
            if (p[-1] != ' ' && *p >= 'A' && *p <= 'Z') {
                *p = static_cast<char>(*p - 'A' + 'a');
            }
        }
        touchgfx::Unicode::strncpy(buf, date, static_cast<uint16_t>(cap - 1));
        buf[cap - 1] = 0;
    } else {
        touchgfx::Unicode::snprintf(buf, cap, "%s", touchgfx::TypedText(T_TEXT_LIBRARY).getText());
    }
}

void WorkoutListView::showGroup(int16_t row)
{
    if (workoutAt(row) < 0) {
        menuLayout.setInfoText(nullptr);
        return;
    }
    groupOf(row, mGroup, sizeof(mGroup) / sizeof(mGroup[0]));
    menuLayout.setInfoMsgColor(SDK::GUI::Color::TEAL);
    menuLayout.setInfoText(mGroup);
}

void WorkoutListView::onAnimationMiddle(int16_t index)
{
    showGroup(index);
}

void WorkoutListView::updateItem(MainMenuItem& item, int16_t index)
{
    MenuItemConfig cfg;
    const int16_t i = workoutAt(index);
    if (i < 0) {
        cfg.msgId = T_TEXT_FREE_RUN;
    } else if (mLibrary != nullptr && static_cast<size_t>(i) < mLibrary->size()) {
        WorkoutUi::toText(mLibrary->entry(static_cast<size_t>(i)).name, mText, sizeof(mText) / sizeof(mText[0]));
        WorkoutUi::fitWidth(mText, T_TMP_MEDIUM_18, 190);
        groupOf(index, mTip, sizeof(mTip) / sizeof(mTip[0]));
        cfg.style    = MenuItemConfig::TIP;
        cfg.msgText  = mText;
        cfg.tipText  = mTip;
        cfg.tipColor = SDK::GUI::Color::TEAL;
    } else {
        return;
    }
    item.apply(cfg);
}

void WorkoutListView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    MenuItemConfig cfg;
    cfg.style = MenuItemConfig::TIP;
    const int16_t i = workoutAt(index);
    if (i < 0) {
        cfg.msgId = T_TEXT_FREE_RUN;
        cfg.tipId = T_TEXT_CLEAR_WORKOUT;
    } else if (mLibrary != nullptr && static_cast<size_t>(i) < mLibrary->size()) {
        const SDK::Workout::Library::Entry& e = mLibrary->entry(static_cast<size_t>(i));
        WorkoutUi::toText(e.name, mText, sizeof(mText) / sizeof(mText[0]));
        WorkoutUi::fitWidth(mText, T_TMP_SEMIBOLD_25, 200);
        cfg.msgText   = mText;
        cfg.msgIdType = T_TMP_SEMIBOLD_25;

        // The totals; "!" in amber when a step runs in a simplified form.
        char totals[32];
        WorkoutUi::formatTotals(e, mImperial, totals, sizeof(totals));
        char tip[36];
        std::snprintf(tip, sizeof(tip), "%s%s", e.degraded ? "! " : "", totals);
        touchgfx::Unicode::strncpy(mTip, tip, sizeof(mTip) / sizeof(mTip[0]) - 1);
        mTip[sizeof(mTip) / sizeof(mTip[0]) - 1] = 0;
        cfg.tipText = mTip;
        if (e.degraded) {
            cfg.tipColor = SDK::GUI::Color::YELLOW_DARK;
        }
    } else {
        return;
    }
    item.apply(cfg);
}

void WorkoutListView::handleKeyEvent(uint8_t key)
{
    const bool empty = !menuLayout.getMenu().isVisible();
    if (key == SDK::GUI::Button::L1 && !empty) {
        menuLayout.selectPrev();
    }
    if (key == SDK::GUI::Button::L2 && !empty) {
        menuLayout.selectNext();
    }
    if (key == SDK::GUI::Button::R1 && !empty) {
        const int16_t i = workoutAt(static_cast<int16_t>(menuLayout.getSelectedItem()));
        if (i < 0) {
            presenter->clearWorkout();
            application().gotoMainScreenNoTransition();
        } else {
            presenter->chooseWorkout(static_cast<uint16_t>(i));
            application().gotoWorkoutMenuScreenNoTransition();
        }
    }
    if (key == SDK::GUI::Button::R2) {
        application().gotoMainScreenNoTransition();
    }
}
