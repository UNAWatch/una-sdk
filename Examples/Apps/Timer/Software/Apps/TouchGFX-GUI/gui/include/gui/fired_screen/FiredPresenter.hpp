#ifndef FIREDPRESENTER_HPP
#define FIREDPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class FiredView;

/**
 * @brief Presenter for the Fired screen (countdown reached zero).
 *
 * The service is playing the alert; the two actions stop it and either restart
 * the timer (repeat) or end it (stop).
 */
class FiredPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    FiredPresenter(FiredView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~FiredPresenter() {}

    /** @brief The alert stays up until acknowledged -- do not close on idle. */
    virtual void onIdleTimeout() override {}

    /** @brief Silence the alert and re-arm the same countdown, paused. */
    void repeat() { model->repeatTimer(); }
    /** @brief Silence the alert and end the countdown. */
    void done()   { model->stopTimer();   }
    /** @brief Leave the GUI entirely. */
    void exit()   { model->exitApp();     }

    /** @brief True when the fire arrived while the GUI was closed. */
    bool firedFromBackground() const { return model->firedFromBackground(); }

    /** @brief Duration of the timer that fired, in seconds. */
    uint16_t getDurationSec() const { return model->getDurationSec(); }

private:
    FiredPresenter();

    FiredView& view;
};

#endif // FIREDPRESENTER_HPP
