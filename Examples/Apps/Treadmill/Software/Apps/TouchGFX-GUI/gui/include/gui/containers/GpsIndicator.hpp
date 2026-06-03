#ifndef GPSINDICATOR_HPP
#define GPSINDICATOR_HPP

#include <gui_generated/containers/GpsIndicatorBase.hpp>

/**
 * @brief Self-animating GPS fix indicator.
 *
 * Blinks at the configured period while searching; hides itself and
 * unregisters from the timer once a fix is acquired.
 * Call setPeriod() in setupScreen() and setAcquired() on GPS state changes.
 */
class GpsIndicator : public GpsIndicatorBase
{
public:
    GpsIndicator();
    virtual ~GpsIndicator();

    virtual void initialize();

    /** @brief Animation tick period in display ticks. */
    void setPeriod(uint32_t ticks);

    /**
     * @brief Update GPS fix state.
     * @param acquired true  = fix obtained -> indicator hides (solid off).
     *                 false = searching   -> indicator blinks at period rate.
     */
    void setAcquired(bool acquired);

protected:
    virtual void handleTickEvent() override;

private:
    void registerTimer();
    void unregisterTimer();

    bool     mAcquired    = false;
    bool     mRegistered  = false;
    uint32_t mPeriod      = 0;
    uint32_t mCounter     = 0;
};

#endif // GPSINDICATOR_HPP
