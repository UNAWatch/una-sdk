#ifndef IMAGEVIEWERVIEW_HPP
#define IMAGEVIEWERVIEW_HPP

#include <gui_generated/imageviewer_screen/ImageViewerViewBase.hpp>
#include <gui/imageviewer_screen/ImageViewerPresenter.hpp>

class ImageViewerView : public ImageViewerViewBase
{
public:
    ImageViewerView();
    virtual ~ImageViewerView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleKeyEvent(uint8_t key) override;
protected:
};

#endif // IMAGEVIEWERVIEW_HPP
