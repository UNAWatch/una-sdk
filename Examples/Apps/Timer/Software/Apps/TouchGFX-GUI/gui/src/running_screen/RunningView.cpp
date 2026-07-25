#include <gui/running_screen/RunningView.hpp>

void RunningView::setupScreen()
{
    RunningViewBase::setupScreen();

    // Placeholder title so the skeleton screens are distinguishable while
    // navigation is wired up. Real per-screen content lands in the next stage.
    title.set("Running");
}

void RunningView::tearDownScreen()
{
    RunningViewBase::tearDownScreen();
}
