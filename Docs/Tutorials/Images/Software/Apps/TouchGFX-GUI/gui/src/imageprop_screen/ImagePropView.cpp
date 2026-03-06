#include <gui/imageprop_screen/ImagePropView.hpp>
#include <images/BitmapDatabase.hpp>

ImagePropView::ImagePropView()
{

}

void ImagePropView::setupScreen()
{
    ImagePropViewBase::setupScreen();

    add(menu);
    menu.setPosition(0, 0, 240, 240);
    menu.setNumberOfItems(4);

    // Assume text ids T_BITMAP, T_POSITION, T_SIZE, T_OPACITY
    // Assume bitmap ids BITMAP_BITMAP_ID, BITMAP_POSITION_ID, BITMAP_SIZE_ID, BITMAP_OPACITY_ID

    // Item 0: Bitmap
    MenuItemSelected* pS = menu.getSelectedItem(0);
    pS->config(T_BITMAP);
    pS->setIcon(BITMAP_BITMAP_ID);

    MenuItemNotSelected* pN = menu.getNotSelectedItem(0);
    pN->config(T_BITMAP);
    pN->setIcon(BITMAP_BITMAP_ID);

    // Item 1: Position
    pS = menu.getSelectedItem(1);
    pS->config(T_POSITION);
    pS->setIcon(BITMAP_POSITION_ID);

    pN = menu.getNotSelectedItem(1);
    pN->config(T_POSITION);
    pN->setIcon(BITMAP_POSITION_ID);

    // Item 2: Size
    pS = menu.getSelectedItem(2);
    pS->config(T_SIZE);
    pS->setIcon(BITMAP_SIZE_ID);

    pN = menu.getNotSelectedItem(2);
    pN->config(T_SIZE);
    pN->setIcon(BITMAP_SIZE_ID);

    // Item 3: Opacity
    pS = menu.getSelectedItem(3);
    pS->config(T_OPACITY);
    pS->setIcon(BITMAP_OPACITY_ID);

    pN = menu.getNotSelectedItem(3);
    pN->config(T_OPACITY);
    pN->setIcon(BITMAP_OPACITY_ID);

    menu.invalidate();
}

void ImagePropView::tearDownScreen()
{
    ImagePropViewBase::tearDownScreen();
}

void ImagePropView::handleKeyEvent(uint8_t key)
{
    if (key == Gui::Config::Button::L1) {
        menu.selectPrev();
    } else if (key == Gui::Config::Button::L2) {
        menu.selectNext();
    } else if (key == Gui::Config::Button::R1) {
        presenter->gotoImageMenu();
    } else if (key == Gui::Config::Button::R2) {
        presenter->back();
    }
}
