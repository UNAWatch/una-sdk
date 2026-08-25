#ifndef DELETEDVIEW_HPP
#define DELETEDVIEW_HPP

#include <gui_generated/deleted_screen/DeletedViewBase.hpp>
#include <gui/deleted_screen/DeletedPresenter.hpp>
#include <touchgfx/Callback.hpp>
#include <SDK/GUI/CountdownTimer.hpp>

/**
 * @brief Deleted screen: brief "Deleted" confirmation, then back to Main.
 *
 * A cross-in-circle glyph and the word "Deleted"; no buttons. A CountdownTimer
 * dismisses the screen to Main once the hold elapses.
 */
class DeletedView : public DeletedViewBase
{
public:
    DeletedView();
    virtual ~DeletedView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

private:
    void onDismiss();

    SDK::GUI::CountdownTimer   mDismissTimer;
    touchgfx::Callback<DeletedView> mDismissCb;
};

#endif // DELETEDVIEW_HPP
