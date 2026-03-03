#ifndef IMAGEPROPVIEW_HPP
#define IMAGEPROPVIEW_HPP

#include <gui_generated/imageprop_screen/ImagePropViewBase.hpp>
#include <gui/imageprop_screen/ImagePropPresenter.hpp>

class ImagePropView : public ImagePropViewBase
{
public:
    ImagePropView();
    virtual ~ImagePropView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // IMAGEPROPVIEW_HPP
