
#pragma once

#include <cstdint>
#include <cstdbool>

namespace sdk::api
{

/**
 * @brief   Power Interface.
 */
class Power {

public:

    /**
     * @brief   Get actual battery level.
     * @retval  Actual battery level (0 - 100%).
     */
    virtual float getBatteryLevel() = 0;

protected:

    /**
     * @brief   Destructor.
     */
    virtual ~Power() = default;

};

} // namespace sdk::api