#include <gui/imagelist_screen/ImageListView.hpp>
#include <vector>
#include <string>

ImageListView::ImageListView()
{

}

void ImageListView::setupScreen()
{
    ImageListViewBase::setupScreen();
}

void ImageListView::tearDownScreen()
{
    ImageListViewBase::tearDownScreen();
}

void ImageListView::updateImageList(const std::vector<std::string>& filenames)
{
    // Populate the scroll list with filenames
    // Assuming scrollList1 is the ScrollList widget (to be added in TouchGFX Designer)
    scrollList1.setNumberOfItems(filenames.size());
    for (size_t i = 0; i < filenames.size(); ++i) {
        // Set text for each item - assuming each item has a text widget
        scrollList1.itemChanged(i);
    }

    // For now, store the filenames for later use
    mFilenames = filenames;
}

void ImageListView::handleKeyEvent(uint8_t key)
{
    if (key == Gui::Config::Button::R1) { presenter->gotoImageMenu(); } else if (key == Gui::Config::Button::R2) { presenter->back(); }
}
