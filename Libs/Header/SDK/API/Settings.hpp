
#pragma once

#include <cstdint>
#include <cstdbool>
#include <array>

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

    /**
     * @brief   Get heart rate thresholds array in beats per minute (BPM).
     * @note    Array contains 4 threshold values that define zone boundaries.
     *          Zone 1: < thresholds[0], Zone 2: thresholds[0] to thresholds[1], etc.
     *          Default values: 150, 170, 190, 210 BPM.
     * 
     * @return  An array of 4 uint8_t values representing heart rate thresholds.
     */
    virtual std::array<uint8_t, 4> getHrThresholds() = 0;

protected:

    /**
     * @brief   Destructor.
     */
    virtual ~Settings() = default;

};

}
