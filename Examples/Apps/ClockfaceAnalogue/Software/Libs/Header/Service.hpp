/**
 ******************************************************************************
 * @file    Service.hpp
 * @date    27-August-2026
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Analogue clockface service: the clock and the charge level.
 ******************************************************************************
 */

#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorTypes.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"

#include <cstdint>
#include <ctime>

/**
 * @class Service
 * @brief Background half of the app: it owns the clock and the charge level.
 *
 * The face has no second hand, so the fastest thing it draws turns once a
 * minute. The clock is read once a turn round the loop: once for the minute
 * that expires, and once more for any message that arrives before it. The
 * battery sensor speaks only when the level moves, so on the watch that comes
 * to about once a minute all told, and between those the thread is blocked.
 */
class Service
{
public:
    Service(SDK::Kernel &kernel);

    virtual ~Service();

    void run();

private:
    void connect();
    void disconnect();

    /** Parse one sensor-layer batch and hand the level on. */
    void handleSensorData(uint16_t handle, SDK::Sensor::DataBatch &data);

    /** Send the reading on, unless it matches the one last sent. */
    void publishTime(const std::tm &local);

    // Everything the face shows leaves through one of these. Each drops a
    // value equal to the one it last sent, so a source may call them as often
    // as it likes and only a real change costs an IPC round trip.

    /** @brief Tell the GUI the charge level, 0-100. */
    void publishBatteryLevel(uint8_t level);

    /**
     * @brief Tell the GUI whether alerts are currently silenced.
     *
     * Nothing calls this yet, and that is the whole story: the SDK exposes no
     * mute state for an app to read, so the face has the icon and the path to
     * drive it but no source to drive it from. When one appears, calling this
     * is all the wiring the GUI side needs -- it already hides and shows the
     * icon from what arrives here.
     */
    void publishAlertsMuted(bool muted);

    SDK::Kernel            &mKernel;
    SDK::Sensor::Connection mBatterySensor;   ///< Charge level, event driven
    uint8_t                 mHour;            ///< Last reading sent to the GUI,
    uint8_t                 mMinute;          ///< compared in full, because a
    uint8_t                 mMday;            ///< clock can be set to the same
    uint8_t                 mWday;            ///< time on a different day
    bool                    mTimeSent;        ///< A time has reached the GUI
    uint8_t                 mLevel;           ///< Last level sent to the GUI
    bool                    mLevelSent;       ///< A level has reached the GUI
    bool                    mMuted;           ///< Last mute state sent to the GUI
    bool                    mMutedSent;       ///< A mute state has reached the GUI
};

#endif // SERVICE_HPP
