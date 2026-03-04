#include <gui/imageviewer_screen/ImageViewerView.hpp>

ImageViewerView::ImageViewerView()
{

}

void ImageViewerView::setupScreen()
{
    ImageViewerViewBase::setupScreen();
}

void ImageViewerView::tearDownScreen()
{
    ImageViewerViewBase::tearDownScreen();
}

void ImageViewerView::handleKeyEvent(uint8_t key)
{
    if (key == Gui::Config::Button::R1) { presenter->gotoImageMenu(); } else if (key == Gui::Config::Button::R2) { presenter->back(); }
}
