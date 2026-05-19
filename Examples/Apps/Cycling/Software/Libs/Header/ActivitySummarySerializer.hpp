/**
 ******************************************************************************
 * @file    ActivitySummarySerializer.hpp
 * @date    08-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Serializes/Deserializes summary data to a file.
 ******************************************************************************
 *
 * The serialized format follows this structure:
 *
 * @code{.json}
 * {
 *     "utc":1744163530,
 *     "time":125,
 *     "distance":4015.4,
 *     "speed_avg":3.64264,
 *     "elevation":98.4013,
 *     "pace_avg":0.274536,
 *     "hr_max":84,
 *     "hr_avg":74,
 *     "map":"416A426B43...6643",
 *     "lap_count":2,
 *     "laps":[
 *         {"dur":62,"dist":2010.5,"speed":3.65},
 *         {"dur":63,"dist":2004.9,"speed":3.63}
 *     ]
 * }
 * @endcode
 *
 ******************************************************************************
 */

#ifndef ACTIVITY_SUMMARY_SERIALIZER_HPP
#define ACTIVITY_SUMMARY_SERIALIZER_HPP

#include "SDK/Kernel/Kernel.hpp"

#include "ActivitySummary.hpp"

/**
 * @class ActivitySummarySerializer
 * @brief Serializes/Deserializes summary data to a file.
 */
class ActivitySummarySerializer {

public:

    /**
     * @brief Constructor.
     * @param kernel    Reference to the kernel interface.
     * @param pathToFile Path to the summary file.
     */
    ActivitySummarySerializer(const SDK::Kernel& kernel, const char* pathToFile);

    ~ActivitySummarySerializer() = default;

    /**
     * @brief Save summary to file.
     * @param summary Reference to the Summary structure to save.
     * @return True if summary saved successfully.
     */
    bool save(const ActivitySummary& summary);

    /**
     * @brief Load summary from file.
     * @param summary Reference to the Summary structure to populate.
     * @return True if summary loaded successfully.
     */
    bool load(ActivitySummary& summary);

private:
    const SDK::Kernel& mKernel; ///< Reference to the kernel interface.
    const char*        mPath;   ///< Path to the summary file.
};

#endif // ACTIVITY_SUMMARY_SERIALIZER_HPP
