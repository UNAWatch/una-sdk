/**
 ******************************************************************************
 * @file    IActivity.hpp
 * @date    14-01-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   ACTIVITY Interface.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __INTERFACE_I_ACTIVITY_HPP
#define __INTERFACE_I_ACTIVITY_HPP

#include <cstdint>

namespace Interface
{

/**
 * @brief   Interface for accessing user activity data.
 */
class IActivity
{
public:

    class Callback {
    public:
        virtual void onHeartRate(float hr) {}
    protected:
        virtual ~Callback() = default;
    };


    /**
     * @brief   Get the current heart rate of the user.
     * @retval  Heart rate in beats per minute (bpm).
     */
    virtual float heartRate() const = 0;

    /**
     * @brief   Get the number of steps taken since the start of the day.
     * @retval  Total step count from the beginning of the day.
     */
    virtual uint32_t steps() const = 0;

    /**
     * @brief   Get the number of floors taken since the start of the day.
     * @retval  Total floors count from the beginning of the day.
     */
    virtual uint32_t floors() const = 0;

    /**
     * @brief   Get the distance covered.
     * @retval  Distance in kilometers (km) traveled since the start of the day.
     */
    virtual float distance() const = 0;

    /**
     * @brief   Get the current speed of movement.
     * @retval  Speed in kilometers per hour (km/h).
     */
    virtual float speed() const = 0;

    /**
     * @brief   Get the current elevation above sea level.
     * @retval  Elevation in meters (m).
     */
    virtual float elevation() const = 0;

    /**
     * @brief   Get the current azimuth (angular direction)
     * @retval  Azimuth in degrees (0 - 360°)
     *          North - 0° East - 90° South - 180° West - 270°
     */
    virtual float azimuth() const = 0;

    /**
     * @brief   Get current location.
     * @param   lat: Reference to save latitude in degrees.
     * @param   lon: Reference to save longitude in degrees.
     * @param   prec: Reference to save measurement precision in meters.
     * @retval  The status indicates the validity of the data.
     *          'true' - the data is valid, 'false' - otherwise.
     */
    virtual bool location(float &lat, float &lon, float &prec) const = 0;

    /**
      * @brief   Attach a callback for Activity events.
      * @note    It is allowed attach multiple callbacks.
      * @param   pCallback: Pointer to the callback interface.
      */
     virtual void attachCallback(Interface::IActivity::Callback *pCallback) = 0;

     /**
      * @brief   Detach the currently attached status callback.
      * @param   pCallback: Pointer to the callback interface to be detached.
      */
     virtual void detachCallback(Interface::IActivity::Callback *pCallback) = 0;


protected:
    /**
     * @brief   Virtual destructor.
     */
    virtual ~IActivity() = default;
};

} /* namespace Interface */

#endif /* __INTERFACE_I_ACTIVITY_HPP */
