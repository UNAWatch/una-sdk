#include <gui/main_screen/MainView.hpp>

MainView::MainView()
{

}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::NONE);
    buttons.setR2(Buttons::AMBER);
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::updateHR(float hr, float tl)
{
    Unicode::snprintfFloat(hrValueBuffer, HRVALUE_SIZE, "%.0f", hr);
    hrValue.invalidate();
    Unicode::snprintfFloat(trustLevelValueBuffer, TRUSTLEVELVALUE_SIZE, "%.0f", tl);
    trustLevelValue.invalidate();
}

void MainView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::R2) {
        presenter->exit();
    }
}