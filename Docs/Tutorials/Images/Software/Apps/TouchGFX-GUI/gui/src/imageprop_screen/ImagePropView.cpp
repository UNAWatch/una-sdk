#include <gui/imageprop_screen/ImagePropView.hpp>

ImagePropView::ImagePropView()
{

}

void ImagePropView::setupScreen()
{
    ImagePropViewBase::setupScreen();
}

void ImagePropView::tearDownScreen()
{
    ImagePropViewBase::tearDownScreen();
}

void ImagePropView::handleKeyEvent(uint8_t key)
{
    if (key == Gui::Config::Button::R1) { presenter->gotoImageMenu(); } else if (key == Gui::Config::Button::R2) { presenter->back(); }
}
