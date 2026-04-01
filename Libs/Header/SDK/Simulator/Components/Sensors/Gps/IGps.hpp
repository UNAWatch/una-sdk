/**
 ******************************************************************************
 * @file    IGps.hpp
 * @date    14-01-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   GPS Interface.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __INTERFACE_I_GPS_HPP
#define __INTERFACE_I_GPS_HPP

#include <ctime>
#include <stdint.h>

#ifndef M_PI
#define M_PI    3.14159265358979323846264338327950288   /* pi */
#endif

namespace Interface
{

/**
 * @brief GPS Interface.
 */
class IGps
{

public:

    /**
     * @brief Structure that describes GPS location information.
     */
    struct LocationInfo {
        bool valid;         /// < Indicates if the data is valid.
        float lat;          /// < Latitude in degrees.
        float lon;          /// < Longitude in degrees.
        float alt;          /// < Altitude in meters.
        float precision;    /// < Measurement precision in meters.
    };

    /**
     * @brief   GPS Status Callbacks interface
     */
    class Callback {
    public:

        /**
         * @brief   Called when the GPS Signal Acquired/Lost.
         * @param   state: 'true' if GPS Signal Acquired, 'false' signal lost.
         */
        virtual void onGpsFix(bool acquired) {}

        /**
         * @brief   Called when the time is received.
         * @note    It will send only wen time is valid.
         * @param   utc: Time in UTC format (seconds since epoch).
         */
        virtual void onGpsTime(std::time_t utc) {}

        /**
         * @brief   Called when the location is received.
         * @param   location: Structure with location information.
         */
        virtual void onGpsLocation(Interface::IGps::LocationInfo location) {}

    protected:

        /**
         * @brief   Destructor.
         */
        virtual ~Callback() = default;
    };

    /**
     * @brief   Initialize GPS object.
     * @retval  Execution status. 'true' - success, 'false' - otherwise.
     */
    virtual bool init() = 0;

    /**
     * @brief   Deinitialize GPS object and free all resources.
     * @retval  Execution status. 'true' - success, 'false' - otherwise.
     */
    virtual bool deinit() = 0;

    /**
     * @brief   Enable/Turn on GPS.
     * @retval  Execution status. 'true' - success, 'false' - otherwise.
     */
    virtual bool enable() = 0;

    /**
     * @brief   Disable/Turn off GPS or put it to low power mode.
     * @retval  Execution status. 'true' - success, 'false' - otherwise.
     */
    virtual bool disable() = 0;

    /**
     * @brief   Check if the GPS is on.
     * @retval  'true' - GPS is on, 'false' - otherwise.
     */
    virtual bool isEnabled() = 0;

    /**
     * @brief   Attach a callback for GPS events.
     * @note    It is allowed attach multiple callbacks.
     * @param   pCallback: Pointer to the callback interface.
     */
    virtual void attachCallback(Interface::IGps::Callback *pCallback) = 0;

    /**
     * @brief   Detach the currently attached status callback.
     * @param   pCallback: Pointer to the callback interface to be detached.
     */
    virtual void detachCallback(Interface::IGps::Callback *pCallback) = 0;

    /**
     * @brief   Set the data update period.
     * @note    This is only a recommended value to reduce consumption.
     *          GPS can ignore it.
     * @param   seconds: Period in seconds.
     */
    virtual void setPeriod(uint32_t seconds) = 0;

    /*
     * @brief   Check if the GPS receiver has successfully determined its location
     * @retval  'true' - GPS receiver has determined its location, 'false' - otherwise.
     */
    virtual bool hasFix() = 0;

    /**
     * @brief   Get current time.
     * @retval  Time in UTC format (seconds since epoch).
     *          If there is no fix, or GPS is disabled, 0 will be returned.
     */
    virtual time_t getTime() = 0;

    /**
     * @brief   Get current speed.
     * @return  Speed in meters per second (m/s).
     *          If there is no fix, or GPS is disabled, 0 will be returned.
     */
    virtual float getSpeed() = 0;

    /**
     * @brief   Get current distance.
     * @return  Distance in meters since power on.
     *          If there is no fix, the last valid value will be returned.
     *          If GPS is disabled, 0 will be returned.
     */
    virtual float getDistance() = 0;

    /**
 * @brief   Get current speed.
 * @return  Altitude in meters.
 *          If there is no fix, or GPS is disabled, 0 will be returned.
 */
    virtual float getAltitude() = 0;

    /**
     * @brief   Get current location.
     * @retval  location: Structure with location information.
     */
    virtual Interface::IGps::LocationInfo getLocation() = 0;


    /**
     * @brief   Represents the validity period of EPO (Extended Prediction Orbit)
     *          files for various GNSS constellations.
     * @details If there is no EPO file available for a given constellation,
     *          both `start` and `end` will be 0.
     */
    struct EpoPeriod {
        /// @brief Represents the start and end time of a period.
        struct Range {
            uint32_t start = 0; ///< Start time in hours since GPS epoch (January 6, 1980)
            uint32_t end   = 0; ///< End time in hours since GPS epoch (January 6, 1980)
        };

        /**
         * @brief Converts the stored hour value to calendar time (std::time_t).
         * @param hours Number of hours since GPS epoch
         * @return Calendar time as std::time_t (UTC)
         * @note GPS epoch starts at 1980-01-06 00:00:00 UTC. Leap seconds are ignored.
         */
        static std::time_t toTimeT(uint32_t hours) {
            constexpr std::time_t gpsEpoch = 315964800; // seconds since 1970-01-01 UTC for 1980-01-06 00:00:00 UTC
            return gpsEpoch + static_cast<std::time_t>(hours) * 3600;
        }

        Range gps;      ///< Validity period for GPS
        Range glonass;  ///< Validity period for GLONASS
        Range galileo;  ///< Validity period for Galileo
        Range beidou;   ///< Validity period for BeiDou
        Range qzss;     ///< Validity period for QZSS
    };

    /**
     * @brief   Retrieves the EPO file validity period for supported GNSS constellations.
     * @note This feature is optional and may not be implemented in all drivers.
     * @return EpoPeriod structure containing validity ranges for each constellation. Returns empty ranges by default.
     */
    virtual EpoPeriod getEpoFilePeriod() { return {}; }

    /**
     * @brief Sets and stores the reference location for the GNSS subsystem.
     *
     * @param lat Reference latitude in degrees (WGS-84). Positive for North, negative for South.
     * @param lon Reference longitude in degrees (WGS-84). Positive for East, negative for West.
     * @param alt Reference altitude in meters above mean sea level (MSL).
     *
     * @details
     * This method provides the GNSS driver with a fixed reference location that can be used
     * for aiding, fallback positioning, or improving acquisition performance.
     * Implementations may persist this reference location in non-volatile memory so it remains
     * available after reboot.
     * If the feature is not supported, the default implementation may ignore the call.
     */
    virtual void setReferenceLocation(float lat, float lon, float alt) {}

    /**
     * @brief Gets stored the reference location for the GNSS subsystem.
     *
     * @param lat Reference latitude in degrees (WGS-84). Positive for North, negative for South.
     * @param lon Reference longitude in degrees (WGS-84). Positive for East, negative for West.
     * @param alt Reference altitude in meters above mean sea level (MSL).
     *
     * @return True if successful, false otherwise. False means the data has not been saved yet.
     */
    virtual bool getReferenceLocation(float& lat, float& lon, float& alt) { return false; }

protected:

    /**
     * @brief   Destructor.
     */
    virtual ~IGps() = default;

};

} /* namespace Interface */

#endif /* __INTERFACE_I_GPS_HPP */
