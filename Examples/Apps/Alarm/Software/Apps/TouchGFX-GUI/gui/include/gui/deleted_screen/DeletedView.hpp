#ifndef DELETEDVIEW_HPP
#define DELETEDVIEW_HPP

#include <gui_generated/deleted_screen/DeletedViewBase.hpp>
#include <gui/deleted_screen/DeletedPresenter.hpp>
#include <gui/containers/CountdownTimer.hpp>
#include <touchgfx/Callback.hpp>

/**
 * @brief Confirmation screen shown briefly after an alarm is deleted.
 *
 * Displays the alarm index and auto-dismisses after kDismissTicks.
 */
class DeletedView : public DeletedViewBase
{
public:
    DeletedView();
    virtual ~DeletedView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /** @brief Show the number of the alarm that was just deleted (1-based). */
    void setAlarmId(size_t id);

protected:
    /** @brief Fired by mDismissTimer when the display timeout elapses. */
    void onDismiss();

    CountdownTimer                   mDismissTimer;  ///< Auto-dismiss countdown
    touchgfx::Callback<DeletedView>  mDismissCb;
};

#endif // DELETEDVIEW_HPP
