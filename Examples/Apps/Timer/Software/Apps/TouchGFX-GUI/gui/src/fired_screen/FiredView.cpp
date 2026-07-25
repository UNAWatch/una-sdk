#include <gui/fired_screen/FiredView.hpp>

void FiredView::setupScreen()
{
    FiredViewBase::setupScreen();

    // Placeholder title so the skeleton screens are distinguishable while
    // navigation is wired up. Real per-screen content lands in the next stage.
    title.set("Fired");
}

void FiredView::tearDownScreen()
{
    FiredViewBase::tearDownScreen();
}
