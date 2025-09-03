
#pragma once

#include <cstdint>
#include <cstdbool>

namespace SDK::Interface
{

/**
 * @brief   Settings Interface.
 */
class ISettings
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
    virtual ~ISettings() = default;

};

}
