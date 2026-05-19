#ifndef SUMMARYFACEOVERVIEW_HPP
#define SUMMARYFACEOVERVIEW_HPP

#include <gui_generated/containers/SummaryFaceOverviewBase.hpp>
#include <ctime>

/**
 * @brief Summary face: session time (H:MM:SS), average HR, and max HR.
 *
 * One of the swipeable faces on the TrackSummary screen.
 * Label text widgets are expected to be set in TouchGFX Designer.
 */
class SummaryFaceOverview : public SummaryFaceOverviewBase
{
public:
    SummaryFaceOverview();
    virtual ~SummaryFaceOverview() {}

    virtual void initialize();

    /** @brief Total active time as "H:MM:SS" in the top value area; no unit suffix. */
    void setTotalTime(std::time_t sec);

    /** @brief Average heart rate (bpm), or "---" when below @ref App::Display::kMinHR. */
    void setAvgHR(float hr);

    /** @brief Maximum heart rate (bpm), or "---" when below @ref App::Display::kMinHR. */
    void setMaxHR(float hr);

protected:
};

#endif // SUMMARYFACEOVERVIEW_HPP
