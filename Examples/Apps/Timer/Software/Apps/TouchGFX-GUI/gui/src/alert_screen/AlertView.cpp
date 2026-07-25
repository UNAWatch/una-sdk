#include <gui/alert_screen/AlertView.hpp>

void AlertView::setupScreen()
{
    AlertViewBase::setupScreen();

    // Placeholder title so the skeleton screens are distinguishable while
    // navigation is wired up. Real per-screen content lands in the next stage.
    title.set("Alert");
}

void AlertView::tearDownScreen()
{
    AlertViewBase::tearDownScreen();
}
