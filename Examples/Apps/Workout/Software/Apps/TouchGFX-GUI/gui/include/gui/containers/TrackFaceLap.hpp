#ifndef TRACKFACELAP_HPP
#define TRACKFACELAP_HPP

#include <gui_generated/containers/TrackFaceLapBase.hpp>

/**
 * @brief Track face showing lap-focused metrics.
 */
class TrackFaceLap : public TrackFaceLapBase
{
public:
    TrackFaceLap();
    virtual ~TrackFaceLap() {}

    virtual void initialize();

    void setLapAvgHR(float hr);
    void setLapNumber(uint32_t lapNum);

    /** @brief Display current lap elapsed time as "M:SS" (or "H:MM" when >= 1 h). */
    void setTimer(std::time_t sec);

protected:
};

#endif // TRACKFACELAP_HPP
