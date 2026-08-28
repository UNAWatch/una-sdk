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
 */
struct WallTime
{
    uint8_t hour;       ///< 0..23
    uint8_t minute;     ///< 0..59
    uint8_t mday;       ///< Day of month, 1..31
    uint8_t wday;       ///< Day of week, 0 = Sunday, as std::tm::tm_wday

    bool operator==(const WallTime &o) const
    {
        return (hour == o.hour) && (minute == o.minute) &&
               (mday == o.mday) && (wday == o.wday);
    }
};

class FrontendApplication;
class ModelListener;

/**
 * @class Model
 * @brief What the face shows, and the app lifecycle around it.
 *
 * The time, the charge level and the mute state all arrive from the service
 * as messages and are held here until a screen asks. The clock is also read
 * directly, but only at the two moments no message can be waiting for -- see
 * now().
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

    /**
     * @brief Whether alerts are silenced, false until the service says so.
     *
     * Nothing reports it yet, so the icon starts hidden and stays hidden.
     */
    bool alertsMuted() const { return mAlertsMuted; }

protected:
    ModelListener *modelListener;   ///< Pointer to model listener

    const SDK::Kernel &mKernel;     ///< Reference to kernel interface

    bool mResumed = false;          ///< Resume seen; handled on the next tick

    WallTime mTime {};              ///< Reading the service last reported
    uint8_t mBatteryLevel = 0;      ///< Last level the service reported
    bool    mAlertsMuted  = false;  ///< Last mute state the service reported

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
     * when the Model is built, and when the GUI resumes after being off screen.
     * Everything between the two is pushed by the service, once a minute.
     *
     * That this can be done from a constructor at all rests on the clock being
     * a plain call. A request to the kernel could be sent from there, but not
     * answered: replies are drained by callCustomMessageHandler(), which does
     * not run until the frames start. Anything that moves the clock behind an
     * exchange with the kernel takes this reading with it, and the first frame
     * then has to come from somewhere else.
     */
    WallTime now() const;

    /** Take a reading, and tell the screen if it differs from the last. */
    void adopt(const WallTime &time);
};

#endif // MODEL_HPP
