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
         * @brief   Destructor.
         */
        virtual ~Callback() = default;
    };

    virtual void setParamSimulation(float speedMin, float speedMidle, float speedMax, uint32_t seachSatteliteMs) = 0;

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

protected:

    /**
     * @brief   Destructor.
     */
    virtual ~IGps() = default;

};

} /* namespace Interface */

#endif /* __INTERFACE_I_GPS_HPP */
