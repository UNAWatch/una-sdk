#ifndef DELETEDPRESENTER_HPP
#define DELETEDPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class DeletedView;

/**
 * @brief Presenter for the "alarm deleted" confirmation screen.
 *
 * Deletes the selected alarm from the model on activation, displays its
 * former index, and routes away via switchToNextPriorityScreen() when the
 * auto-dismiss timer fires.
 */
class DeletedPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    DeletedPresenter(DeletedView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~DeletedPresenter() {}

    /** @brief Navigate away from this confirmation screen. */
    void exitScreen();

private:
    DeletedPresenter();

    DeletedView& view;
};

#endif // DELETEDPRESENTER_HPP
