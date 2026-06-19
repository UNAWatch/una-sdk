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

void MainView::setAccessoryStatus(uint8_t state, const char* name)
{
    // Placeholder: repurpose the menu title to surface external-HR link status
    // (SDK::Accessory::State values). On CONNECTED show the device name so the
    // user can tell which strap linked. The real visual is a dedicated indicator
    // widget once the Designer's UX lands — kept code-only here, no texts regen.
    const char* label = nullptr;
    switch (state) {
        case 2: label = "HR: Searching";  break;  // SEARCHING
        case 3: label = "HR: Connecting"; break;  // CONNECTING
        case 4:                                    // CONNECTED -> show device name
            label = (name != nullptr && name[0] != '\0') ? name : "HR: Connected";
            break;
        case 5: label = "HR: Lost";       break;  // LOST
        default: break;                            // UNAVAILABLE / IDLE
    }
    if (label != nullptr) {
        menuLayout.setTitle(label);
    } else {
        menuLayout.setTitle(T_TEXT_APP_NAME_UC);
    }
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
    if (index < 0 || index >= static_cast<int16_t>(Menu::ID_COUNT)) {
        return;
    }
    item.apply(mItems[index]);
}

void MainView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    if (index < 0 || index >= static_cast<int16_t>(Menu::ID_COUNT)) {
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
    if (idx < 0 || idx >= static_cast<int16_t>(Menu::ID_COUNT)) {
        return;
    }

    switch (idx) {
    case Menu::ID_START:
        presenter->startTrack();
        application().gotoTrackScreenNoTransition();
        break;

    case Menu::ID_SETTINGS:
        application().gotoMenuSettingsScreenNoTransition();
        break;

    default:
        break;
    };
}

void MainView::updateBackground(int16_t index)
{
    switch (index) {
        case Menu::ID_START:
            menuLayout.setBackground(SDK::GUI::Color::TEAL_DARK);
            menuLayout.getButtons().setR1(Buttons::AMBER);
            break;

        default:
            menuLayout.setBackground(SDK::GUI::Color::TEAL_DARK);
            menuLayout.getButtons().setR1(Buttons::AMBER);
            break;
    }
}
