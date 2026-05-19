#include <gui/main_screen/MainView.hpp>
#include <images/BitmapDatabase.hpp>


MainView::MainView() :
    mUpdateItemCb(this, &MainView::updateItem),
    mUpdateCenterItemCb(this, &MainView::updateCenterItem),
    mpAnimationMiddleCb(this, &MainView::onAnimationMiddle)
{
}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    setupItems();
    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setTitle(T_TEXT_APP_NAME_UC);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setAnimationMiddleCallback(mpAnimationMiddleCb);
    menuLayout.setNumberOfItems(Menu::ID_COUNT);
    menuLayout.invalidate();

    updateBackground(menuLayout.getSelectedItem());
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::setGpsFix(bool state)
{
    mGpsFix = state;

    if (mGpsFix) {
        menuLayout.setInfoMsg(T_TEXT_SIGNAL_ACQUIRED);
    } else {
        menuLayout.setInfoMsg(T_TEXT_ACQUIRING_SIGNAL);
    }

    updateBackground(menuLayout.getSelectedItem());
}

void MainView::setPositionId(uint16_t id)
{
    menuLayout.selectItem(id);
    updateBackground(menuLayout.getSelectedItem());
}

uint16_t MainView::getPositionId()
{
    return menuLayout.getSelectedItem();
}

void MainView::setupItems()
{
    // START
    mCenterItems[Menu::ID_START].msgId     = T_TEXT_START;
    mCenterItems[Menu::ID_START].msgIdType = T_TMP_SEMIBOLD_35;

    mItems[Menu::ID_START].msgId = T_TEXT_START;


    // SETTINGS
    mCenterItems[Menu::ID_SETTINGS].msgId = T_TEXT_SETTINGS;

    mItems[Menu::ID_SETTINGS].msgId = T_TEXT_SETTINGS;
}

void MainView::updateItem(MainMenuItem& item, int16_t index)
{
    if (index < 0 || index >= Menu::ID_COUNT) {
        return;
    }
    item.apply(mItems[index]);
}

void MainView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    if (index < 0 || index >= Menu::ID_COUNT) {
        return;
    }
    item.apply(mCenterItems[index]);
}

void MainView::onAnimationMiddle(int16_t index)
{
    updateBackground(index);
}

void MainView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L1) {
        menuLayout.selectPrev();
    }

    if (key == SDK::GUI::Button::L2) {
        menuLayout.selectNext();
    }

    if (key == SDK::GUI::Button::R1) {
        onConfirm();
    }

    if (key == SDK::GUI::Button::R2) {
        presenter->exitApp();
    }
}

void MainView::onConfirm()
{
    int16_t idx = static_cast<int16_t>(menuLayout.getSelectedItem());
    if (idx < 0 || idx >= Menu::ID_COUNT) {
        return;
    }

    switch (idx) {
    case Menu::ID_START:
        if (mGpsFix) {
            presenter->startTrack();
            application().gotoTrackScreenNoTransition();
        } else {
            application().gotoTrackStartConfirmationScreenNoTransition();
        }
        break;

    case Menu::ID_SETTINGS:
        application().gotoMenuSettingsScreenNoTransition();
        break;

    default:
        break;
    }
}

void MainView::updateBackground(int16_t index)
{
    switch (index) {
        case Menu::ID_START:
            if (mGpsFix) {
                menuLayout.setBackground(SDK::GUI::Color::TEAL_DARK);
                menuLayout.getButtons().setR1(Buttons::AMBER);
            } else {
                menuLayout.setBackground(SDK::GUI::Color::GRAY_DARK);
                menuLayout.getButtons().setR1(Buttons::NONE);
            }
            break;

        default:
            menuLayout.setBackground(SDK::GUI::Color::TEAL_DARK);
            menuLayout.getButtons().setR1(Buttons::AMBER);
            break;
    }
}
