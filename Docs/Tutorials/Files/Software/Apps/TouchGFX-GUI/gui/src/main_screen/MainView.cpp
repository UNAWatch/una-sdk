#include <gui/main_screen/MainView.hpp>
#include <touchgfx/Texts.hpp>

MainView::MainView()
    : mDecimalCounter(1.0f)
    , mActivityType(CustomMessage::ActivityType::RUNNING)
    , mDisplayMode(CustomMessage::DisplayMode::SIMPLE)
    , stgSel(StgType::STG1)
{

}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    buttons.setL1(ButtonsSet::WHITE);
    buttons.setL2(ButtonsSet::WHITE);
    buttons.setR1(ButtonsSet::AMBER);
    buttons.setR2(ButtonsSet::WHITE);

    // Request initial settings
    presenter->requestSettings();

    // Initial display
    updateSettingsDisplay(mDecimalCounter, mActivityType, mDisplayMode);
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::updateSettingsDisplay(int32_t decimalCounter, CustomMessage::ActivityType activityType, CustomMessage::DisplayMode displayMode)
{
    mDecimalCounter = decimalCounter;
    mActivityType = activityType;
    mDisplayMode = displayMode;

    // Update setting1: decimal counter
    Unicode::snprintf(setting1Buffer, SETTING1_SIZE, "%i", decimalCounter);
    setting1.setColor(touchgfx::Color::getColorFromRGB(stgSel == StgType::STG1 ? 255 : 255, stgSel == StgType::STG1 ? 0 : 255, stgSel == StgType::STG1 ? 0 : 255));
    setting1.resizeToCurrentText();
    setting1.invalidate();

    // Update setting2: activity type
    const char* activityStr = "*";
    switch (activityType) {
        case CustomMessage::ActivityType::RUNNING: activityStr = "RUNNING"; break;
        case CustomMessage::ActivityType::CYCLING: activityStr = "CYCLING"; break;
        case CustomMessage::ActivityType::SWIMMING: activityStr = "SWIMMING"; break;
        case CustomMessage::ActivityType::WALKING: activityStr = "WALKING"; break;
    }
    Unicode::strncpy(setting2Buffer, activityStr, SETTING2_SIZE - 1);
    setting2.setColor(touchgfx::Color::getColorFromRGB(stgSel == StgType::STG2 ? 255 : 255, stgSel == StgType::STG2 ? 0 : 255, stgSel == StgType::STG2 ? 0 : 255));
    setting2.resizeToCurrentText();
    setting2.invalidate();

    // Update setting3: display mode
    const char* displayStr = "-";
    switch (displayMode) {
        case CustomMessage::DisplayMode::SIMPLE: displayStr = "SIMPLE"; break;
        case CustomMessage::DisplayMode::DETAILED: displayStr = "DETAILED"; break;
        case CustomMessage::DisplayMode::COMPACT: displayStr = "COMPACT"; break;
    }
    Unicode::strncpy(setting3Buffer, displayStr, SETTING3_SIZE - 1);
    setting3.setColor(touchgfx::Color::getColorFromRGB(stgSel == StgType::STG3 ? 255 : 255, stgSel == StgType::STG3 ? 0 : 255, stgSel == StgType::STG3 ? 0 : 255));
    setting3.resizeToCurrentText();
    setting3.invalidate();
}

void MainView::handleKeyEvent(uint8_t key)
{
    if (key == Gui::Config::Button::L1) {
        // Change value of selected setting
        switch (stgSel) {
            case StgType::STG1: {
                mDecimalCounter++;
                break;
            }
            case StgType::STG2: {
                // Cycle activity type
                int current = static_cast<int>(mActivityType);
                current = (current + 1) % CustomMessage::ActivityType::_MAX_ACTIVITY_TYPE;
                mActivityType = static_cast<CustomMessage::ActivityType>(current);
                break;
            }
            case StgType::STG3: {
                // Cycle display mode
                int current = static_cast<int>(mDisplayMode);
                current = (current + 1) % CustomMessage::DisplayMode::_MAX_DISPLAY_MODE;
                mDisplayMode = static_cast<CustomMessage::DisplayMode>(current);
                break;
            }
        }
    }

    if (key == Gui::Config::Button::L2) {
        // Change value of selected setting (decrement)
        switch (stgSel) {
            case StgType::STG1: {
                mDecimalCounter--;
                break;
            }
            case StgType::STG2: {
                // Cycle activity type
                int current = static_cast<int>(mActivityType);
                current = (current - 1 + CustomMessage::ActivityType::_MAX_ACTIVITY_TYPE) % CustomMessage::ActivityType::_MAX_ACTIVITY_TYPE; // decrement
                mActivityType = static_cast<CustomMessage::ActivityType>(current);
                break;
            }
            case StgType::STG3: {
                // Cycle display mode
                int current = static_cast<int>(mDisplayMode);
                current = (current - 1 + CustomMessage::DisplayMode::_MAX_DISPLAY_MODE) % CustomMessage::DisplayMode::_MAX_DISPLAY_MODE; // decrement
                mDisplayMode = static_cast<CustomMessage::DisplayMode>(current);
                break;
            }
        }
    }

    if (key == Gui::Config::Button::R1) {
        // Save current settings
        presenter->saveSettings(mDecimalCounter, mActivityType, mDisplayMode);

        // Switch selection
        int current = static_cast<int>(stgSel);
        current = (current + 1) % StgType::_MAX_STG;
        stgSel = static_cast<StgType>(current);
    }

    if (key == Gui::Config::Button::R2) {
        presenter->saveSettings(mDecimalCounter, mActivityType, mDisplayMode);
        presenter->exit();
    }

    updateSettingsDisplay(mDecimalCounter, mActivityType, mDisplayMode);
}