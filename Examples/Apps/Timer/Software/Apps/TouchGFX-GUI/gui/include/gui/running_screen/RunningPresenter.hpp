#ifndef RUNNINGPRESENTER_HPP
#define RUNNINGPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>
#include <Timer.hpp>

using namespace touchgfx;

class RunningView;

/**
 * @brief Presenter for the Running screen (active / paused countdown).
 *
 * Passes control actions to the model and forwards service state echoes to the
 * view; the view reads the live remaining time each tick.
 */
class RunningPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    RunningPresenter(RunningView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~RunningPresenter() {}

    /** @brief Countdown state echoed by the service changed -- re-sync the view. */
    virtual void onStateChanged() override;

    // -- Live snapshot read by the view --------------------------------------
    TimerState getState()       const { return model->getState(); }
    uint32_t   getRemainingMs() const { return model->getRemainingMs(); }

    // -- Controls ------------------------------------------------------------
    void pause()  { model->pauseTimer();  }
    void resume() { model->resumeTimer(); }
    void reset()  { model->resetTimer();  }
    void stop()   { model->stopTimer();   }

    /** @brief Leave the GUI; the service keeps counting, or exits if idle. */
    void exit()   { model->exitApp();     }

private:
    RunningPresenter();

    RunningView& view;
};

#endif // RUNNINGPRESENTER_HPP
