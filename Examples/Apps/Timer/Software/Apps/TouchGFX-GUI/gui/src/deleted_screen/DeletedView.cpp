#include <gui/deleted_screen/DeletedView.hpp>

void DeletedView::setupScreen()
{
    DeletedViewBase::setupScreen();

    // Placeholder title so the skeleton screens are distinguishable while
    // navigation is wired up. Real per-screen content lands in the next stage.
    title.set("Deleted");
}

void DeletedView::tearDownScreen()
{
    DeletedViewBase::tearDownScreen();
}
