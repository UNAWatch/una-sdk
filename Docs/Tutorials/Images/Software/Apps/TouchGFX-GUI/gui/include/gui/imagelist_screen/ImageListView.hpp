#ifndef IMAGELISTVIEW_HPP
#define IMAGELISTVIEW_HPP

#include <gui_generated/imagelist_screen/ImageListViewBase.hpp>
#include <gui/imagelist_screen/ImageListPresenter.hpp>
#include <vector>
#include <string>

class ImageListView : public ImageListViewBase
{
public:
    ImageListView();
    virtual ~ImageListView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void updateImageList(const std::vector<std::string>& filenames);
    virtual void handleKeyEvent(uint8_t key) override;

protected:
    std::vector<std::string> mFilenames;
};

#endif // IMAGELISTVIEW_HPP
