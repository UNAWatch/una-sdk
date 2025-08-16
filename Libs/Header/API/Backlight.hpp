
#pragma once

#include <cstdint>
#include <cstdbool>

namespace sdk::api {

/**
 * @brief   Backlight Interface.
 */
class Backlight
{
public:

    /**
     * @brief   Turn on backlight.
     * @param   timeout: Timeout for automatic turn off. 0 - no automatic turn off.
     * @retval  Execution status. 'true' - success, 'false' - otherwise.
     */
    virtual bool on(uint32_t timeout = 0) = 0;

    /**
     * @brief   Turn off backlight.
     * @retval  Execution status. 'true' - success, 'false' - otherwise.
     */
    virtual bool off() = 0;

    /**
     * @brief   Check if the backlight is on.
     * @retval  'true' - backlight is on, 'false' - otherwise.
     */
    virtual bool isOn() = 0;

protected:

    /**
     * @brief   Destructor.
     */
    virtual ~Backlight() = default;

};

} // namespace sdk::api
