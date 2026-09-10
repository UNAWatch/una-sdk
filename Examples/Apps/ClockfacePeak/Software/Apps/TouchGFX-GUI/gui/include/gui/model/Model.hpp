#ifndef MODEL_HPP
#define MODEL_HPP

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Interfaces/ICustomMessageHandler.hpp"

#include <cstdint>

/**
 * @brief Wall-clock reading, to the minute and to the day.
 *
 * The face has no second hand, so the fastest thing it draws turns once a
 * minute and nothing finer is carried around. The date fields ride along
 * because they come out of the same std::tm and cost nothing to keep.
 *
 * @c hour is always 0..23. Folding it to a 12-hour dial is the view's job, so
 * that a change of clock format needs no new reading.
 */
struct WallTime
{
    uint8_t hour;       ///< 0..23
    uint8_t minute;     ///< 0..59
    uint8_t mday;       ///< Day of month, 1..31
    uint8_t wday;       ///< Day of week, 0 = Sunday, as std::tm::tm_wday
    uint8_t mon;        ///< Month, 0 = January, as std::tm::tm_mon

    bool operator==(const WallTime &o) const
    {
        return (hour == o.hour) && (minute == o.minute) &&
               (mday == o.mday) && (wday == o.wday) && (mon == o.mon);
    }
};

/**
 * @brief The day's three readings, as the service last reported them.
 *
 * @c bpm is zero when the service has no reading it trusts; the face draws its
 * placeholder rather than the number in that case.
 */
struct DailyHealth
{
    uint32_t steps;
    uint32_t activityMinutes;
    uint16_t bpm;

    bool operator==(const DailyHealth &o) const
    {
        return (steps == o.steps) &&
               (activityMinutes == o.activityMinutes) && (bpm == o.bpm);
    }
};

/**
 * @brief The daily targets the two rim arcs are fractions of.
 *
 * Zero until the service has read the system settings, which draws the rings
 * empty rather than full: an unknown target is not a met one.
 */
struct DailyGoals
{
    uint32_t steps;
    uint32_t activityMinutes;

    bool operator==(const DailyGoals &o) const
    {
        return (steps == o.steps) && (activityMinutes == o.activityMinutes);
    }
};

class FrontendApplication;
class ModelListener;

/**
 * @class Model
 * @brief What the face shows, and the app lifecycle around it.
 *
 * Everything the face draws arrives from the service as a message and is held
 * here until a screen asks. The clock is also read directly, but only at the
 * two moments no message can be waiting for -- see now().
 */
class Model : public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    Model();

    void bind(ModelListener *listener)
    {
        modelListener = listener;
    }

    FrontendApplication &application();
    void tick();

    /** @brief The reading the face is drawing. */
    WallTime currentTime() const { return mTime; }

    /** @brief Charge level in percent, 0 until the service has reported one. */
    uint8_t batteryLevel() const { return mBatteryLevel; }

    /** @brief The day's readings, all zero until the service has reported. */
    DailyHealth health() const { return mHealth; }

    /** @brief The daily targets, zero until the service has read them. */
    DailyGoals goals() const { return mGoals; }

    /**
     * @brief Whether the watch is set to a 12-hour clock.
     *
     * Starts false, so the face draws a 24-hour clock until the service has
     * read the setting. That is the right way round to be wrong: the reading
     * itself is correct either way, only its presentation is provisional, and
     * it is corrected within a frame or two of the GUI starting.
     */
    bool is12h() const { return mIs12h; }

    /**
     * @brief Whether alerts are silenced, false until the service says so.
     *
     * Nothing reports it yet, and this face draws no mute icon regardless.
     */
    bool alertsMuted() const { return mAlertsMuted; }

protected:
    ModelListener *modelListener;   ///< Pointer to model listener

    const SDK::Kernel &mKernel;     ///< Reference to kernel interface

    bool mResumed = false;          ///< Resume seen; handled on the next tick

    WallTime    mTime {};           ///< Reading the service last reported
    DailyHealth mHealth {};         ///< Readings the service last reported
    DailyGoals  mGoals {};          ///< Targets the service last reported
    uint8_t     mBatteryLevel = 0;  ///< Last level the service reported
    bool        mIs12h        = false;
    bool        mAlertsMuted  = false;

    // IGuiLifeCycleCallback
    void onStart()   override;
    void onResume()  override;
    void onSuspend() override;
    void onStop()    override;

    // ICustomMessageHandler
    bool customMessageHandler(SDK::MessageBase *message) override;

    /**
     * @brief The current local time and date, read straight from libc.
     *
     * Called at the two moments the face has to be right and no push is due:
     * when the Model is built, and when the GUI resumes after being off
     * screen. Everything between the two is pushed by the service, once a
     * minute.
     *
     * That this can be done from a constructor at all rests on the clock being
     * a plain call. A request to the kernel could be sent from there, but not
     * answered: replies are drained by callCustomMessageHandler(), which does
     * not run until the frames start. That is also why the clock format is not
     * read here -- it comes from the kernel, so it can only arrive later.
     */
    WallTime now() const;

    /** Take a reading, and tell the screen if it differs from the last. */
    void adopt(const WallTime &time);
};

#endif // MODEL_HPP
