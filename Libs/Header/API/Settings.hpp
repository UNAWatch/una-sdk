
#pragma once

#include <cstdint>
#include <cstdbool>

namespace sdk::api
{

/**
 * @brief   Settings Interface.
 */
class Settings
{

public:

    /**
     * @brief   Check if the units are imperial.
     * @retval  true if the units are imperial, false otherwise.
     */
    virtual bool isUnitsImperial() = 0;

protected:

    /**
     * @brief   Destructor.
     */
    virtual ~Settings() = default;

};

}
