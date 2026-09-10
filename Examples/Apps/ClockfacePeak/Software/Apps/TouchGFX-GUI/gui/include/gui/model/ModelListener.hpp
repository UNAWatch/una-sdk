#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
#include <gui/common/FrontendApplication.hpp>

/**
 * @class ModelListener
 * @brief What the screen is told when something it draws has changed.
 *
 * All of these are pushed: the service is what watches the clock, the sensors
 * and the settings, and the screen only hears about a value that differs from
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

    /** @brief One or more of the day's readings changed. */
    virtual void onHealth(const DailyHealth &health) { (void)health; }

    /** @brief The daily targets changed, or arrived for the first time. */
    virtual void onGoals(const DailyGoals &goals) { (void)goals; }

    /** @brief The watch's 12/24-hour setting changed, or arrived for the first time. */
    virtual void onClockFormat(bool is12h) { (void)is12h; }

    /** @brief The mute state changed. */
    virtual void onAlertsMuted(bool muted) { (void)muted; }

protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
