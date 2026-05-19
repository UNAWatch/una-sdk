#ifndef SUMMARYFACEHEARTRATE_HPP
#define SUMMARYFACEHEARTRATE_HPP

#include <gui_generated/containers/SummaryFaceHeartRateBase.hpp>

/**
 * @brief Summary face showing heart rate summary: peak and average HR over the session.
 *
 * One of four swipeable faces on the TrackSummary screen.
 */
class SummaryFaceHeartRate : public SummaryFaceHeartRateBase
{
public:
    SummaryFaceHeartRate();
    virtual ~SummaryFaceHeartRate() {}

    virtual void initialize();

    /**
     * @brief Display peak heart rate.
     * @param hr Maximum HR recorded during the session, in bpm.
     */
    void setMaxHR(float hr);

    /**
     * @brief Display average heart rate.
     * @param hr Average HR over the session, in bpm.
     */
    void setAvgHR(float hr);

protected:
};

#endif // SUMMARYFACEHEARTRATE_HPP
