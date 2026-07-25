#include <gui/main_screen/MainView.hpp>

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    // Placeholder title so the skeleton screens are distinguishable while
    // navigation is wired up. Real per-screen content lands in the next stage.
    title.set("Main");
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}
