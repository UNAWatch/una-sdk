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
    buttons.setR2(ButtonsSet::WHITE);
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::handleKeyEvent(uint8_t key)
{
    if (key == Gui::Config::Button::L1) {

    }

    if (key == Gui::Config::Button::L2) {

    }

    if (key == Gui::Config::Button::R1) {
   
    }

    if (key == Gui::Config::Button::R2) {
        presenter->exit();
    }
}