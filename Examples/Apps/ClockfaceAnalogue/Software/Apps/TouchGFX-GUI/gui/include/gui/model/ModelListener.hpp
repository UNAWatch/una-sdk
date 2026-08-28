#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
#include <gui/common/FrontendApplication.hpp>

/**
 * @class ModelListener
 * @brief What the screen is told when something it draws has changed.
 *
 * All three are pushed: the service is what watches the clock, the sensor and
 * the mute state, and the screen only hears about a value that differs from
 * the one it was last given.
 */
class ModelListener
{
public:
    ModelListener() : model(0) {}

    virtual ~ModelListener() {}

    void bind(Model* m)
    {
        model = m;
    }

    /** @brief A new reading arrived, differing from the one on screen. */
    virtual void onTime(const WallTime &time) { (void)time; }

    /** @brief A new charge level arrived from the service. */
    virtual void onBatteryLevel(uint8_t level) { (void)level; }

    /** @brief The mute state changed. */
    virtual void onAlertsMuted(bool muted) { (void)muted; }

protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
