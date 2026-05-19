/**
 ******************************************************************************
 * @file    SettingsSerializer.hpp
 * @date    08-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Serializes/Deserializes settings to a file.
 ******************************************************************************
 *
 * The serialized format follows this structure:
 *
 * @code{.json}
 * {
 *     "version":1,
 *     "auto_pause_en":false,
 *     "phone_notif_en":true,
 *     "alert_distance_id":0,
 *     "alert_time_id":0
 * }
 * @endcode
 *
 ******************************************************************************
 */

#ifndef SETTINGS_SERIALIZER_HPP
#define SETTINGS_SERIALIZER_HPP

#include <cstdint>

#include "SDK/Kernel/Kernel.hpp"

#include "Settings.hpp"


/**
 * @brief Serializes/Deserializes settings to a file.
 */
class SettingsSerializer {

public:

    /**
     * @brief Constructor.
     * @param kernel    Reference to the kernel interface.
     * @param pathToFile Path to the settings file.
     */
    SettingsSerializer(const SDK::Kernel& kernel, const char *pathToFile);

    ~SettingsSerializer() = default;

    /**
     * @brief Save settings to file.
     * @param settings Reference to the Settings structure to save.
     * @return True if settings saved successfully.
     */
    bool save(const Settings &settings);

    /**
     * @brief Load settings from file.
     * @param settings Reference to the Settings structure to populate.
     * @return True if settings loaded successfully.
     */
    bool load(Settings &settings);

private:
    const SDK::Kernel& mKernel; ///< Reference to the kernel interface.
    const char*        mPath;   ///< Path to the settings file.
};

#endif // SETTINGS_SERIALIZER_HPP
