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
    menu1.getSelectedItem(0)->config(T_COUNTER);
    menu1.getSelectedItem(1)->config(T_INCREASE);
    menu1.getSelectedItem(2)->config(T_DECREASE);
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
        auto* counter_nosel_item = menu1.getNotSelectedItem(0);
        auto* counter_sel_item = menu1.getSelectedItem(0);

        touchgfx::Unicode::UnicodeChar buffer[32];
        switch (selected) {
        case 0:
            // Reset counter
            counter = 0;
            touchgfx::Unicode::snprintf(buffer, 32, "Counter: %d", counter);
            counter_nosel_item->config(buffer);
            counter_sel_item->config(buffer);
            break;
        case 1:
            counter++;
            touchgfx::Unicode::snprintf(buffer, 32, "Counter: %d", counter);
            counter_nosel_item->config(buffer);
            counter_sel_item->config(buffer);
            // Increment counter
            break;
        case 2:
            // Decrement counter
            counter--;
            touchgfx::Unicode::snprintf(buffer, 32, "Counter: %d", counter);
            counter_nosel_item->config(buffer);
            counter_sel_item->config(buffer);
            break;
        }
        menu1.invalidate();
        counter_nosel_item->invalidate();
        counter_sel_item->invalidate();
    }

    if (key == Gui::Config::Button::R2) {
        if (lastKeyPressed == key) presenter->exit();
    }
    lastKeyPressed = key;
}