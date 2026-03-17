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
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::updateSettingsDisplay(float decimalCounter, CustomMessage::ActivityType activityType, CustomMessage::DisplayMode displayMode)
{
    mDecimalCounter = decimalCounter;
    mActivityType = activityType;
    mDisplayMode = displayMode;

    // Update setting1: decimal counter
    Unicode::snprintf(setting1Buffer, SETTING1_SIZE, "%.1f", decimalCounter);
    // setting1.setColor(colortype(stgSel == StgType::STG1 ? 0xFF0000 : 0xFF00));
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
    // setting2.setColor(colortype(stgSel == StgType::STG2 ? 0xFF0000 : 0xFF00));
    setting2.invalidate();

    // Update setting3: display mode
    const char* displayStr = "-";
    switch (displayMode) {
        case CustomMessage::DisplayMode::SIMPLE: displayStr = "SIMPLE"; break;
        case CustomMessage::DisplayMode::DETAILED: displayStr = "DETAILED"; break;
        case CustomMessage::DisplayMode::COMPACT: displayStr = "COMPACT"; break;
    }
    Unicode::strncpy(setting3Buffer, displayStr, SETTING3_SIZE - 1);
    // setting3.setColor(colortype(stgSel == StgType::STG3 ? 0xFF0000 : 0xFF00));
    setting3.invalidate();
}

void MainView::handleKeyEvent(uint8_t key)
{
    if (key == Gui::Config::Button::L1) {
        // Cycle decimal counter: 0.5, 1.0, 1.5, 2.0
        const float values[] = {0.5f, 1.0f, 1.5f, 2.0f};
        int currentIndex = -1;
        for (int i = 0; i < 4; ++i) {
            if (mDecimalCounter == values[i]) {
                currentIndex = i;
                break;
            }
        }
        if (currentIndex == -1) currentIndex = 0; // default
        currentIndex = (currentIndex + 1) % 4;
        mDecimalCounter = values[currentIndex];
        updateSettingsDisplay(mDecimalCounter, mActivityType, mDisplayMode);
    }

    if (key == Gui::Config::Button::L2) {
        // Cycle display mode
        int current = static_cast<int>(mDisplayMode);
        current = (current + 1) % 3;
        mDisplayMode = static_cast<CustomMessage::DisplayMode>(current);
        updateSettingsDisplay(mDecimalCounter, mActivityType, mDisplayMode);
    }

    if (key == Gui::Config::Button::R1) {
        // Save current settings
        presenter->saveSettings(mDecimalCounter, mActivityType, mDisplayMode);
    }

    if (key == Gui::Config::Button::R2) {
        // Cycle activity type
        int current = static_cast<int>(mActivityType);
        current = (current + 1) % 4;
        mActivityType = static_cast<CustomMessage::ActivityType>(current);
        updateSettingsDisplay(mDecimalCounter, mActivityType, mDisplayMode);
    }
}