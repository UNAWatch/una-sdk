/**
 * @file   KernelMessageDispatcher.hpp
 * @date   30-December-2025
 * @author Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief  Kernel-bound message dispatcher
 */

#ifndef __KERNEL_MESSAGE_DISPATCHER_HPP
#define __KERNEL_MESSAGE_DISPATCHER_HPP

#include <cstdint>

#include "SDK/Simulator/App/AppComm.hpp"
#include "SDK/Simulator/App/MessageManager.hpp"
#include "SDK/Interfaces/IVibro.hpp"
#include "SDK/Interfaces/IBacklight.hpp"
#include "SDK/Interfaces/IBuzzer.hpp"

namespace SDK::App
{

class KernelMessageDispatcher : public SDK::App::Comm::IDispatch
{
public:
    /**
     * @brief Construct message manager
     */
    KernelMessageDispatcher(::App::MessageManager&      messageManager,
                            SDK::Interface::IVibro&     vibro,
                            SDK::Interface::IBacklight& backlight,
                            SDK::Interface::IBuzzer&    buzzer);
    

    /**
     * @brief Destructor
     */
    ~KernelMessageDispatcher() = default;
    
    bool dispatchMessage(SDK::MessageBase* msg) override;

private:
    struct HeartRateZones
    {

        /// Maximum number of zones.
        static constexpr uint8_t kMaxNum = 5;

        /// Maximum number of threshold.
        static constexpr uint8_t kMaxThreshold = kMaxNum - 1;

        /**
         * @brief   Heart rate thresholds array in beats per minute (BPM).
         * @note    Array contains (kMaxNum - 1) threshold values that define zone boundaries.
         *          Zone 1: < thresholds[0], Zone 2: thresholds[0] to thresholds[1], etc.
         *          Default values: 120, 140, 160, 170 BPM.
         */
        uint8_t thresholds[kMaxThreshold]{ 120, 140, 160, 170 };

        /**
         * @brief Equality operator to compare two heart rate zones configurations.
         * @param other The other HeartRateZones object to compare.
         * @return 'true' if the configurations are equal, 'false' otherwise.
         *
         * Compares all threshold values to determine equality.
         */
        bool operator==(const HeartRateZones& other) const
        {
            for (uint8_t i = 0; i < kMaxThreshold; i++) {
                if (thresholds[i] != other.thresholds[i]) {
                    return false;
                }
            }
            return true;
        }

        /**
         * @brief Inequality operator to compare two heart rate zones configurations.
         * @param other The other HeartRateZones object to compare.
         * @return 'true' if the configurations are not equal, 'false' otherwise.
         */
        bool operator!=(const HeartRateZones& other) const
        {
            return !(*this == other);
        }
    };

    struct DailyGoals
    {

        uint32_t activityMinutes = 30;   ///< Target number of active minutes per day.
        uint32_t steps = 5000; ///< Target number of steps per day.
        uint32_t floors = 10;   ///< Target number of floors climbed per day.

        /**
         * @brief Equality operator to compare two daily goals configurations.
         * @param other The other DailyGoals object to compare.
         * @return 'true' if the configurations are equal, 'false' otherwise.
         *
         * Compares all goal values to determine equality.
         */
        bool operator==(const DailyGoals& other) const
        {
            return (activityMinutes == other.activityMinutes &&
                    steps == other.steps &&
                    floors == other.floors);
        }

        /**
         * @brief Inequality operator to compare two daily goals configurations.
         * @param other The other DailyGoals object to compare.
         * @return 'true' if the configurations are not equal, 'false' otherwise.
         */
        bool operator!=(const DailyGoals& other) const
        {
            return !(*this == other);
        }
    };

    struct WatchSettings
    {

        /**
         * @brief   Unit system preference.
         * @note    'false' for metric units (default), 'true' for imperial units.
         */
        bool unitsImperial = false;

        /**
         * @brief Phone-related settings group.
         */
        struct
        {
            bool notifications = true;  ///< Enable/disable phone notifications on the device.
        } phone;

        uint64_t watchfaceAppId = 0;        ///< Identifier of the currently selected watchface application.
        HeartRateZones heartRateZones{};   ///< Heart rate zones configuration for fitness tracking.
        DailyGoals dailyGoals{};           ///< Daily fitness goals and activity targets.

        /**
         * @brief Equality operator to compare two settings configurations.
         * @param other The other Settings object to compare.
         * @return 'true' if the configurations are equal, 'false' otherwise.
         *
         * Compares all settings fields to determine equality.
         */
        bool operator==(const WatchSettings& other) const
        {
            return (unitsImperial == other.unitsImperial &&
                    phone.notifications == other.phone.notifications &&
                    watchfaceAppId == other.watchfaceAppId &&
                    heartRateZones == other.heartRateZones &&
                    dailyGoals == other.dailyGoals);
        }

        /**
         * @brief Inequality operator to compare two settings configurations.
         * @param other The other Settings object to compare.
         * @return 'true' if the configurations are not equal, 'false' otherwise.
         */
        bool operator!=(const WatchSettings& other) const
        {
            return !(*this == other);
        }
    };

    struct WatchSettingsLocal
    {

        /**
         * @brief GPS power mode enumeration.
         * @note Defines different GPS power consumption and accuracy modes.
         */
        enum GpsPowerMode
        {
            GPS_PM_SAVE,        ///< Power saving mode with reduced GPS accuracy.
            GPS_PM_BALANCED,    ///< Balanced mode between power consumption and accuracy.
            GPS_PM_PRECISION,   ///< High precision mode with increased power consumption.

            GPS_PM_COUNT        ///< Total number of GPS power modes (for iteration).
        };

        /**
         * @brief Notification settings structure.
         * @note Controls feedback settings for different device events.
         */
        struct Notification
        {
            bool buttons = false;   ///< Enable feedback for button presses.
            bool msgCalls = false;  ///< Enable feedback for messages and calls.
            bool alerts = false;    ///< Enable feedback for system alerts and notifications.

            /**
             * @brief Equality operator to compare two notification configurations.
             * @param other The other Notification object to compare.
             * @return 'true' if the configurations are equal, 'false' otherwise.
             */
            bool operator==(const Notification& other) const
            {
                return (buttons == other.buttons &&
                        msgCalls == other.msgCalls &&
                        alerts == other.alerts);
            }

            /**
             * @brief Inequality operator to compare two notification configurations.
             * @param other The other Notification object to compare.
             * @return 'true' if the configurations are not equal, 'false' otherwise.
             */
            bool operator!=(const Notification& other) const
            {
                return !(*this == other);
            }
        };

        bool alertMutedFlag = false;                ///< Global flag to mute all alerts temporarily.
        Notification vibro;                         ///< Vibration notification settings.
        Notification sound;                         ///< Sound notification settings.
        GpsPowerMode gpsPowerMode = GPS_PM_BALANCED;    ///< GPS power consumption mode setting.

        // Temporary debug config
        struct
        {
            bool manufLog = false;
        } ppgCfg;

        struct
        {
            bool nmeaLog = false;
            bool imuLog = false;
            uint8_t binLog = 0;
            uint8_t MinSNR = 9;
            float hdopTh = 0.0f;
            uint32_t haacLim = 0;
            int32_t elevMask = 5;
        } gpsCfg;



        /**
         * @brief Equality operator to compare two local settings configurations.
         * @param other The other SettingsLocal object to compare.
         * @return 'true' if the configurations are equal, 'false' otherwise.
         *
         * Compares all local settings fields to determine equality.
         */
        bool operator==(const WatchSettingsLocal& other) const
        {
            return (alertMutedFlag == other.alertMutedFlag &&
                    vibro == other.vibro &&
                    sound == other.sound &&
                    gpsPowerMode == other.gpsPowerMode);
        }

        /**
         * @brief Inequality operator to compare two local settings configurations.
         * @param other The other SettingsLocal object to compare.
         * @return 'true' if the configurations are not equal, 'false' otherwise.
         */
        bool operator!=(const WatchSettingsLocal& other) const
        {
            return !(*this == other);
        }
    };

    static constexpr WatchSettings mSettings{};
    static constexpr uint32_t      GLANCE_WIDTH        = 240;
    static constexpr uint32_t      GLANCE_HEIGHT       = 60;
    static constexpr uint32_t      GLANCE_MAX_CONTROLS = 32;

    void appLifeCycleHandler(SDK::MessageBase* msg);
    void appMsgHandler(SDK::MessageBase* msg);

    // Prevent copying
    KernelMessageDispatcher(const KernelMessageDispatcher&)            = delete;
    KernelMessageDispatcher& operator=(const KernelMessageDispatcher&) = delete;

    ::App::MessageManager&      mMessageManager;
    SDK::Interface::IVibro&     mVibro;
	SDK::Interface::IBacklight& mBacklight;
	SDK::Interface::IBuzzer&    mBuzzer;
	WatchSettingsLocal          mLocalSettings;
};

} // namespace App

#endif // __KERNEL_MESSAGE_DISPATCHER_HPP
