#include <gui/main_screen/MainView.hpp>
#include <gui/common/AlarmLabels.hpp>
#include <gui/common/TimeFormat.hpp>
#include <touchgfx/Color.hpp>

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

    // Draw the time left-aligned so the AM/PM suffix can follow it; show()
    // re-centres the whole group. (The generated base centres timeValue.)
    timeValue.setTypedText(touchgfx::TypedText(T_TMP_SEMIBOLD_60_L));

    mMeridiem.setColor(touchgfx::Color::getColorFromRGB(192, 192, 192));
    mMeridiem.setLinespacing(0);
    mMeridiem.setTypedText(touchgfx::TypedText(T_TMP_SEMIBOLD_20_L));
    mMeridiem.setWildcard(mMeridiemBuffer);
    mMeridiem.setVisible(false);
    add(mMeridiem);
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
        mMeridiem.setVisible(false);
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

        const uint8_t h = (*pList)[mAlarmId].timeHours;
        const uint8_t m = (*pList)[mAlarmId].timeMinutes;

        timeValue.invalidate();   // clear the old glyph rect before the group moves
        mMeridiem.invalidate();

        if (mIs12Hour) {
            uint8_t h12;
            bool    pm;
            App::TimeFormat::split12(h, h12, pm);
            Unicode::snprintf(timeValueBuffer, TIMEVALUE_SIZE, "%d:%02d", h12, m);

            mMeridiemBuffer[0] = pm ? 'P' : 'A';
            mMeridiemBuffer[1] = 'M';
            mMeridiemBuffer[2] = 0;
            mMeridiem.setWildcard(mMeridiemBuffer);
        } else {
            Unicode::snprintf(timeValueBuffer, TIMEVALUE_SIZE, "%02d:%02d", h, m);
        }
        timeValue.setWildcard(timeValueBuffer);

        // Centre the time (+ AM/PM suffix) as one group on the 240px screen.
        const uint16_t timeW = timeValue.getTextWidth();
        uint16_t       merW  = 0;
        uint16_t       groupW = timeW;
        if (mIs12Hour) {
            merW   = mMeridiem.getTextWidth();
            groupW = static_cast<uint16_t>(timeW + kMeridiemGap + merW);
        }
        const int16_t groupLeft = static_cast<int16_t>((240 - groupW) / 2);

        timeValue.setPosition(groupLeft, kTimeY, static_cast<int16_t>(timeW + 4), kTimeH);
        timeValue.invalidate();

        if (mIs12Hour) {
            mMeridiem.setPosition(static_cast<int16_t>(groupLeft + timeW + kMeridiemGap),
                                  kMeridiemY, static_cast<int16_t>(merW + 4), kMeridiemH);
            mMeridiem.setVisible(true);
            mMeridiem.invalidate();
        } else {
            mMeridiem.setVisible(false);
        }

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
    mMeridiem.invalidate();
    repeatText.invalidate();
    repeatValue.invalidate();
}