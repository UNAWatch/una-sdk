#ifndef MAINPRESENTER_HPP
#define MAINPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

#include "Stopwatch.hpp"

using namespace touchgfx;

class MainView;

class MainPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MainPresenter(MainView& v);

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

    virtual ~MainPresenter() {}

    void exit() {
        model->exitApp();
    }

    /** @name Stopwatch controls
     *
     * Each returns whether the command reached the service.
     *  @{ */
    bool start() { return model->startStopwatch(); }
    bool pause() { return model->pauseStopwatch(); }
    bool lap()   { return model->lapStopwatch(); }
    bool reset() { return model->resetStopwatch(); }
    /** @} */

    const Stopwatch::State &stopwatch() const { return model->stopwatch(); }
    uint32_t elapsedMs() const { return model->elapsedMs(); }
    uint32_t nowMs() const { return model->nowMs(); }

    // ModelListener implementation
    virtual void onStopwatchChanged(const Stopwatch::State &state) override;

private:
    MainPresenter();

    MainView& view;
};

#endif // MAINPRESENTER_HPP
