#ifndef RINGINGPRESENTER_HPP
#define RINGINGPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class RingingView;

/**
 * @brief Presenter for the ringing alarm screen.
 *
 * Starts alarm playback immediately on activation. If the screen is dismissed
 * by the system (e.g. higher-priority event) while the alarm is still on,
 * deactivate() stops the sound automatically.
 */
class RingingPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    RingingPresenter(RingingView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~RingingPresenter() {}

    /** @brief Trigger one alarm playback cycle (sound + vibration). */
    void play();
    /** @brief Stop the alarm and navigate away. */
    void stop();
    /** @brief Snooze the alarm and navigate away. */
    void snooze();

private:
    RingingPresenter();

    RingingView& view;
};

#endif // RINGINGPRESENTER_HPP
