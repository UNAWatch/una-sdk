#include <gui/main_screen/MainView.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Unicode.hpp>

MainView::MainView()
{

}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    menu1.setNumberOfItems(3);
    menu1.getNotSelectedItem(0)->config(T_COUNTER);
    menu1.getNotSelectedItem(1)->config(T_INCREASE);
    menu1.getNotSelectedItem(2)->config(T_DECREASE);
    menu1.invalidate();

    buttons.setL1(ButtonsSet::NONE);
    buttons.setL2(ButtonsSet::NONE);
    buttons.setR1(ButtonsSet::NONE);
    buttons.setR2(ButtonsSet::AMBER);
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::handleKeyEvent(uint8_t key)
{
    if (key == Gui::Config::Button::L1) {
        menu1.selectPrev();
    }

    if (key == Gui::Config::Button::L2) {
        menu1.selectNext();
    }

    if (key == Gui::Config::Button::R1) {
        int selected = menu1.getSelectedItem();
        touchgfx::Unicode::UnicodeChar buffer[32];
        switch (selected) {
        case 0:
            // Display counter
            touchgfx::Unicode::snprintf(buffer, 32, "%d", counter);
            menu1.setInfoMsgUnicode(buffer);
            break;
        case 1:
            // Increment counter
            counter++;
            touchgfx::Unicode::snprintf(buffer, 32, "%d", counter);
            menu1.setInfoMsgUnicode(buffer);
            break;
        case 2:
            // Decrement counter
            counter--;
            touchgfx::Unicode::snprintf(buffer, 32, "%d", counter);
            menu1.setInfoMsgUnicode(buffer);
            break;
        }
    }

    if (key == Gui::Config::Button::R2) {
        if (lastKeyPressed == key) presenter->exit();
    }
    lastKeyPressed = key;
}