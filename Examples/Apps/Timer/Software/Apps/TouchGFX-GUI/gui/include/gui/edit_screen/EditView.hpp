#ifndef EDITVIEW_HPP
#define EDITVIEW_HPP

#include <gui_generated/edit_screen/EditViewBase.hpp>
#include <gui/edit_screen/EditPresenter.hpp>

/**
 * @brief Edit screen -- skeleton placeholder.
 *
 * Renders the app title and the screen name. Real widgets and logic are
 * added in the next stage.
 */
class EditView : public EditViewBase
{
public:
    EditView() {}
    virtual ~EditView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
};

#endif // EDITVIEW_HPP
