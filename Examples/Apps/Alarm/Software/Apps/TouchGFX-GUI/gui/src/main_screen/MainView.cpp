#include <gui/main_screen/MainView.hpp>
#include <gui/common/AlarmLabels.hpp>

MainView::MainView()
{

}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);

    title.set(T_TEXT_ALARM_UC);
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}


void MainView::updateAlarmList(const std::vector<Alarm>& list)
{
    pList = &list;  // save pointer to the list

    mAlarmId = 0;

    show();
}

void MainView::setSelectedAlarm(size_t id)
{
    if (pList == nullptr) {
        return;
    }


    if (id <= pList->size()) {
        mAlarmId = id;
    }

    show();
}

void MainView::handleKeyEvent(uint8_t key)
{
    if (pList == nullptr) {
        return;
    }

    // Next alarm
    if (key == SDK::GUI::Button::L1) {
        mAlarmId++;
        if (mAlarmId > pList->size()) { // If equals to pList->size() -> New Alarm
            mAlarmId = 0;
        }
        show();
    }

    // Previous alarm
    if (key == SDK::GUI::Button::L2) {
        if (mAlarmId > 0) { // If equals to pList->size() -> New Alarm
            mAlarmId--;
        }
        else {
            mAlarmId = pList->size();
        }
        show();
    }

    if (key == SDK::GUI::Button::R1) {
        if (mAlarmId == pList->size()) {
            // Create new one
            presenter->setAlarmEditId(mAlarmId);
            application().gotoEditScreenNoTransition();
        }
        else {
            // Edit existed one
            presenter->setAlarmEditId(mAlarmId);
            application().gotoMenuScreenNoTransition();
        }
    }

    if (key == SDK::GUI::Button::R2) {
        presenter->exitApp();
    }
}

void MainView::show()
{
    if (pList == nullptr) {
        return;
    }

    buttons.setL1(pList->size() > 0 ? Buttons::WHITE : Buttons::NONE);
    buttons.setL2(pList->size() > 0 ? Buttons::WHITE : Buttons::NONE);

    if (pList->size() == 0 || mAlarmId == pList->size()) {
        // New Alarm
        textNew.setVisible(true);
        icon.setVisible(true);

        alarmText.setVisible(false);
        alarmValue.setVisible(false);
        toggle.setVisible(false);
        timeValue.setVisible(false);
        repeatText.setVisible(false);
        repeatValue.setVisible(false);
    }
    else {
        textNew.setVisible(false);
        icon.setVisible(false);

        alarmText.setVisible(true);
        alarmValue.setVisible(true);
        toggle.setVisible(true);
        timeValue.setVisible(true);
        repeatText.setVisible(true);
        repeatValue.setVisible(true);

        Unicode::snprintf(alarmValueBuffer, ALARMVALUE_SIZE, "%d", mAlarmId + 1);

        toggle.setState((*pList)[mAlarmId].on);

        Unicode::snprintf(timeValueBuffer, TIMEVALUE_SIZE, "%02d:%02d",
            (*pList)[mAlarmId].timeHours, (*pList)[mAlarmId].timeMinutes);

        Unicode::snprintf(repeatValueBuffer, REPEATVALUE_SIZE, "%s",
            touchgfx::TypedText(App::Labels::kRepeatLabels[(*pList)[mAlarmId].repeat]).getText());

        if ((*pList)[mAlarmId].repeat == Alarm::Repeat::REPEAT_NO) {
            repeatValue.setColor(SDK::GUI::Color::TEAL);
        }
        else {
            repeatValue.setColor(SDK::GUI::Color::YELLOW_DARK);
        }

    }

    textNew.invalidate();
    icon.invalidate();
    alarmText.invalidate();
    alarmValue.invalidate();
    toggle.invalidate();
    timeValue.invalidate();
    repeatText.invalidate();
    repeatValue.invalidate();
}