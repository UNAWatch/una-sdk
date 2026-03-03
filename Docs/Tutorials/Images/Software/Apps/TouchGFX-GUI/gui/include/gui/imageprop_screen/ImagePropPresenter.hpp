#ifndef IMAGEPROPPRESENTER_HPP
#define IMAGEPROPPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ImagePropView;

class ImagePropPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ImagePropPresenter(ImagePropView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~ImagePropPresenter() {}

private:
    ImagePropPresenter();

    ImagePropView& view;
};

#endif // IMAGEPROPPRESENTER_HPP
