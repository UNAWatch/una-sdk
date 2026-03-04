#ifndef IMAGEVIEWERPRESENTER_HPP
#define IMAGEVIEWERPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ImageViewerView;

class ImageViewerPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ImageViewerPresenter(ImageViewerView& v);

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

    virtual void gotoImageMenu();
    virtual void back();

    virtual ~ImageViewerPresenter() {}

private:
    ImageViewerPresenter();

    ImageViewerView& view;
};

#endif // IMAGEVIEWERPRESENTER_HPP
