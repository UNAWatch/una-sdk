#ifndef HEARTRATEZONE_HPP
#define HEARTRATEZONE_HPP

#include <gui_generated/containers/HeartRateZoneBase.hpp>

/**
 * @brief Segmented arc indicator showing the current heart rate zone.
 *
 * Displays one of five zone segments (hr1-hr5) based on the current HR value
 * measured against a threshold array. Segments represent zones from lowest (1)
 * to highest (5). No segment is shown when HR is below all thresholds.
 *
 * Usage:
 * @code
 *   heartRateZone.setHR(bpm, thresholds, kHrThresholdsCount);
 * @endcode
 */
class HeartRateZone : public HeartRateZoneBase
{
public:
    HeartRateZone();
    virtual ~HeartRateZone() {}

    virtual void initialize();

    /**
     * @brief Update the displayed heart rate zone.
     * @param hr      Current heart rate in bpm.
     * @param thresholds     Array of zone lower-bound thresholds in ascending order (bpm).
     * @param thresholdCount Number of thresholds (must not exceed kZoneCount).
     */
    void setHR(float hr, const uint8_t* thresholds, uint8_t thresholdCount);

protected:
    static constexpr uint8_t kZoneCount = 5;

    /** Ordered references to zone segment images (index 0 = zone 1, lowest). */
    touchgfx::Image* const mZones[kZoneCount];
};

#endif // HEARTRATEZONE_HPP
