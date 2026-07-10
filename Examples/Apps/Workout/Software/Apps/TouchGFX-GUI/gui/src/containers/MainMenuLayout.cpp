#include <gui/containers/MainMenuLayout.hpp>
#include <images/BitmapDatabase.hpp>

MainMenuLayout::MainMenuLayout()
{
}

void MainMenuLayout::initialize()
{
    MainMenuLayoutBase::initialize();

    // Default button states
    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);

    // Pre-activity sensor-status row in the old infoText slot. Hidden until a
    // screen opts in (value-entry pickers reuse this space, so they don't).
    // HR-only app: no GPS icon.
    add(mSensorRow);
    mSensorRow.setPosition(0, 52, 240, 24);
    mSensorRow.setIcons(touchgfx::BITMAP_INVALID, touchgfx::BITMAP_INVALID,
                        BITMAP_SENSORHRDARK_ID, BITMAP_SENSORHRLIGHT_ID);
    mSensorRow.setVisible(false);
}

// ---- Delegated wheel operations --------------------------------------------

void MainMenuLayout::setNumberOfItems(int16_t numberOfItems)
{
    menu.setNumberOfItems(numberOfItems);
}

int16_t MainMenuLayout::getNumberOfItems()
{
    return menu.getNumberOfItems();
}

void MainMenuLayout::selectNext()
{
    menu.selectNext();
}

void MainMenuLayout::selectPrev()
{
    menu.selectPrev();
}

void MainMenuLayout::selectItem(int16_t itemIndex)
{
    menu.selectItem(itemIndex);
}

uint16_t MainMenuLayout::getSelectedItem()
{
    return menu.getSelectedItem();
}

void MainMenuLayout::invalidate()
{
    menu.invalidate();
}

void MainMenuLayout::setUpdateItemCallback(
    touchgfx::GenericCallback<MainMenuItem &, int16_t> &cb)
{
    menu.setUpdateItemCallback(cb);
}

void MainMenuLayout::setUpdateCenterItemCallback(
    touchgfx::GenericCallback<MainMenuCenterItem &, int16_t> &cb)
{
    menu.setUpdateCenterItemCallback(cb);
}

void MainMenuLayout::setCenterItemLayout(const CenterItemLayout &layout)
{
    menu.setCenterItemLayout(layout);
}

void MainMenuLayout::setItemLayout(const ItemLayout &layout)
{
    menu.setItemLayout(layout);
}

void MainMenuLayout::setAnimationSteps(int16_t steps)
{
    menu.setAnimationSteps(steps);
}

void MainMenuLayout::setAnimationMiddleCallback(touchgfx::GenericCallback<int16_t> &cb)
{
    menu.setAnimationMiddleCallback(cb);
}

void MainMenuLayout::setAnimationEndedCallback(touchgfx::GenericCallback<int16_t> &cb)
{
    menu.setAnimationEndedCallback(cb);
}

// ---- Chrome ----------------------------------------------------------------

void MainMenuLayout::setTitle(TypedTextId msgId)
{
    title.set(msgId);
}

void MainMenuLayout::setTitle(const char* text)
{
    title.set(text);
    title.invalidate();
}

void MainMenuLayout::showTitle(bool state)
{
    title.setVisible(state);
    title.invalidate();
}

void MainMenuLayout::setBackground(touchgfx::colortype color)
{
    background.setColor(color);
}

void MainMenuLayout::showBackground(bool state)
{
    background.setVisible(state);
    background.invalidate();
}

void MainMenuLayout::setInfoMsg(TypedTextId msgId)
{
    if (msgId != TYPED_TEXT_INVALID) {
        touchgfx::Unicode::snprintf(infoTextBuffer, INFOTEXT_SIZE, "%s",
            touchgfx::TypedText(msgId).getText());
        infoText.setVisible(true);
    } else {
        infoText.setVisible(false);
    }
    infoText.invalidate();
}

void MainMenuLayout::setInfoMsgColor(touchgfx::colortype color)
{
    infoText.setColor(color);
    infoText.invalidate();
}

// ---- Sensor status row -----------------------------------------------------

void MainMenuLayout::showSensorRow(bool show)
{
    // The row occupies the old infoText slot; keep them mutually exclusive
    // so a stray setInfoMsg() can never overlap the sensor icons.
    if (show) {
        infoText.setVisible(false);
        infoText.invalidate();
    }
    mSensorRow.setVisible(show);
    mSensorRow.invalidate();
}

void MainMenuLayout::setGps(bool fix)
{
    mSensorRow.setGps(SDK::Gui::SensorStatusRow::gpsState(fix));
}

void MainMenuLayout::setHr(uint8_t accessoryState)
{
    mSensorRow.setHr(SDK::Gui::SensorStatusRow::hrState(accessoryState));
}

// ---- Getters ---------------------------------------------------------------

Buttons &MainMenuLayout::getButtons()
{
    return buttons;
}

MainMenu &MainMenuLayout::getMenu()
{
    return menu;
}
