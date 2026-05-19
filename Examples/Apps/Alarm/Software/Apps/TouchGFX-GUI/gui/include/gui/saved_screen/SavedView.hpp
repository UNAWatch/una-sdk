#ifndef SAVEDVIEW_HPP
#define SAVEDVIEW_HPP

#include <gui_generated/saved_screen/SavedViewBase.hpp>
#include <gui/saved_screen/SavedPresenter.hpp>
#include <gui/containers/CountdownTimer.hpp>
#include <touchgfx/Callback.hpp>

/**
 * @brief Confirmation screen shown briefly after an alarm is saved.
 *
 * Displays the alarm index and auto-dismisses after kDismissTicks.
 */
class SavedView : public SavedViewBase
{
public:
    SavedView();
    virtual ~SavedView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /** @brief Show the number of the alarm that was just saved (1-based). */
    void setAlarmId(size_t id);

protected:
    /** @brief Fired by mDismissTimer when the display timeout elapses. */
    void onDismiss();

    CountdownTimer                 mDismissTimer;  ///< Auto-dismiss countdown
    touchgfx::Callback<SavedView>  mDismissCb;
};

#endif // SAVEDVIEW_HPP
