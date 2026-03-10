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

void MainView::updateHR(float hr, float tl)
{
    Unicode::snprintfFloat(textHRBuffer, TEXTHR_SIZE, "%.0f", hr);
    textHR.invalidate();
    Unicode::snprintfFloat(textTRLBuffer, TEXTTRL_SIZE, "%.0f", tl);
    textTRL.invalidate();
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