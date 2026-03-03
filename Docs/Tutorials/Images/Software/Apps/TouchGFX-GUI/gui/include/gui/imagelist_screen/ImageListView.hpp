#ifndef IMAGELISTVIEW_HPP
#define IMAGELISTVIEW_HPP

#include <gui_generated/imagelist_screen/ImageListViewBase.hpp>
#include <gui/imagelist_screen/ImageListPresenter.hpp>

class ImageListView : public ImageListViewBase
{
public:
    ImageListView();
    virtual ~ImageListView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // IMAGELISTVIEW_HPP
