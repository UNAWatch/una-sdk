#ifndef IMAGEMENUPRESENTER_HPP
#define IMAGEMENUPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ImageMenuView;

class ImageMenuPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ImageMenuPresenter(ImageMenuView& v);

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

    virtual ~ImageMenuPresenter() {}

private:
    ImageMenuPresenter();

    ImageMenuView& view;
};

#endif // IMAGEMENUPRESENTER_HPP
