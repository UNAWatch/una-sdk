
#pragma once

#include <cstdint>
#include <cstdbool>

namespace SDK::Interface
{

/**
 * @brief   App Capabilities interface.
 */
class IAppCapabilities {
public:

    /**
     * @brief   Enable/Disable displaying notifications from the phone when
     *          the APP is running.
     * @note    Notifications are enabled by default.
     * @param   enable: True - enable notifications. False - disable.
     */
    virtual void enablePhoneNotification(bool enable) = 0;

    /**
     * @brief   Enable/disable L2 button hold response for music control when
     *          the APP is running.
     * @note    Music control enabled by default.
     * @param   enable: True - enable notifications. False - disable.
     */
    virtual void enableMusicControl(bool enable) = 0;

    /**
     * @brief   Enable/Disable displaying USB/charging screen when
     *          the APP is running.
     * @note    Switching to USB/charging screen enabled by default.
     * @param   enable: True - enable USB/charging screen. False - disable.
     */
    virtual void enableUsbCharging(bool enable) = 0;

    /**
     * @brief   Notify the kernel that the activity session has ended and the
     *          data has been saved.
     */
    virtual void notifyActivityEnd() = 0;


protected:

    /**
     * @brief   Destructor.
     */
    virtual ~IAppCapabilities() = default;

};

} // namespace SDK::Interface
