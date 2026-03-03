#ifndef IMAGELISTPRESENTER_HPP
#define IMAGELISTPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ImageListView;

class ImageListPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ImageListPresenter(ImageListView& v);

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

    virtual ~ImageListPresenter() {}

private:
    ImageListPresenter();

    ImageListView& view;
};

#endif // IMAGELISTPRESENTER_HPP
