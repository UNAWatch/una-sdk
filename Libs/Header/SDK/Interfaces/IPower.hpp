
#pragma once

#include <cstdint>
#include <cstdbool>

namespace SDK::Interface
{

/**
 * @brief   Power Interface.
 */
class IPower {

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
    virtual ~IPower() = default;

};

} // namespace SDK::Interface
