#include <gui/edit_screen/EditView.hpp>

void EditView::setupScreen()
{
    EditViewBase::setupScreen();

    // Placeholder title so the skeleton screens are distinguishable while
    // navigation is wired up. Real per-screen content lands in the next stage.
    title.set("Edit");
}

void EditView::tearDownScreen()
{
    EditViewBase::tearDownScreen();
}
