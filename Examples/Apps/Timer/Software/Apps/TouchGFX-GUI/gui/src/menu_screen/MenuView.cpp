#include <gui/menu_screen/MenuView.hpp>

void MenuView::setupScreen()
{
    MenuViewBase::setupScreen();

    // Placeholder title so the skeleton screens are distinguishable while
    // navigation is wired up. Real per-screen content lands in the next stage.
    title.set("Menu");
}

void MenuView::tearDownScreen()
{
    MenuViewBase::tearDownScreen();
}
