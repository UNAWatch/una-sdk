#ifndef DELETEDPRESENTER_HPP
#define DELETEDPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class DeletedView;

/**
 * @brief Presenter for the Deleted screen -- skeleton placeholder.
 */
class DeletedPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    DeletedPresenter(DeletedView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~DeletedPresenter() {}

private:
    DeletedPresenter();

    DeletedView& view;
};

#endif // DELETEDPRESENTER_HPP
