#ifndef DELETEDVIEW_HPP
#define DELETEDVIEW_HPP

#include <gui_generated/deleted_screen/DeletedViewBase.hpp>
#include <gui/deleted_screen/DeletedPresenter.hpp>

/**
 * @brief Deleted screen -- skeleton placeholder.
 *
 * Renders the app title and the screen name. Real widgets and logic are
 * added in the next stage.
 */
class DeletedView : public DeletedViewBase
{
public:
    DeletedView() {}
    virtual ~DeletedView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
};

#endif // DELETEDVIEW_HPP
