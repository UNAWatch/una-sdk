#include <gui/main_screen/MainView.hpp>

MainView::MainView()
{

}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

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
        box1.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
    }

    if (key == Gui::Config::Button::L2) {
        box1.setColor(touchgfx::Color::getColorFromRGB(0xff, 0, 0));
    }

    if (key == Gui::Config::Button::R1) {
        box1.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0xff));
    }

    if (key == Gui::Config::Button::R2) {
        box1.setColor(touchgfx::Color::getColorFromRGB(0, 0xff, 0));
        if (lastKeyPressed == key) presenter->exit();
    }
    lastKeyPressed = key;
    box1.invalidate();
}