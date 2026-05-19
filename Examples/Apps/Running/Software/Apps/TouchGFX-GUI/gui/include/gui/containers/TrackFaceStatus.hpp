#ifndef TRACKFACESTATUS_HPP
#define TRACKFACESTATUS_HPP

#include <gui_generated/containers/TrackFaceStatusBase.hpp>

/**
 * @brief Track face showing device status: current time of day and battery level.
 *
 * Intended as a low-information "status" face that the user can switch to during a session.
 */
class TrackFaceStatus : public TrackFaceStatusBase
{
public:
    TrackFaceStatus();
    virtual ~TrackFaceStatus() {}

    virtual void initialize();

    /**
     * @brief Display the current time of day.
     * @param h Hour   (0-23).
     * @param m Minute (0-59).
     */
    void setTime(uint8_t h, uint8_t m);

    /**
     * @brief Display the battery charge level.
     * @param level Charge percentage (0-100).
     */
    void setBatteryLevel(uint8_t level);

protected:
};

#endif // TRACKFACESTATUS_HPP
