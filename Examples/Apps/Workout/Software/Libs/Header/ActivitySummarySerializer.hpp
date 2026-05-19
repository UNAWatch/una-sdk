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
 *     "hr_max":84,
 *     "hr_avg":74,
 *     "calories":42.5,
 *     "resting_calories":12.0,
 *     "active_calories":30.5,
 *     "zone_times_sec":[12,34,56,78,9],
 *     "lap_count":2,
 *     "laps":[
 *         {"dur":62,"hr_avg":120,"hr_max":135},
 *         {"dur":63,"hr_avg":118,"hr_max":132}
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
     * @param kernel: Reference to the kernel interface.
     * @param pathToFile: Path to the summary file.
     */
    ActivitySummarySerializer(const SDK::Kernel& kernel, const char* pathToFile);

    /**
     * @brief Destructor.
     */
    virtual ~ActivitySummarySerializer() = default;

    /**
     * @brief Save summary.
     * @param summary: Reference to the Summary structure.
     * @return True if summary saved successfully.
     */
    bool save(const ActivitySummary& summary);

    /**
     * @brief Save summary.
     * @param summary: Reference to load the Summary structure.
     * @return True if summary read successfully.
     */
    bool load(ActivitySummary& summary);

private:
    /// A constant reference to an Kernel object.
    const SDK::Kernel& mKernel;

    /// Path to summary file
    const char* mPath = nullptr;
};

#endif // ACTIVITY_SUMMARY_SERIALIZER_HPP
