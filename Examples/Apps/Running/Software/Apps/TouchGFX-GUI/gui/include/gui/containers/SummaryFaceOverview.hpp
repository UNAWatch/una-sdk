#ifndef SUMMARYFACEOVERVIEW_HPP
#define SUMMARYFACEOVERVIEW_HPP

#include <gui_generated/containers/SummaryFaceOverviewBase.hpp>
#include <ctime>

/**
 * @brief Summary face showing overall session metrics: distance, average pace and elapsed time.
 *
 * One of four swipeable faces on the TrackSummary screen.
 * All values are display-ready -- unit conversion is the view's responsibility.
 */
class SummaryFaceOverview : public SummaryFaceOverviewBase
{
public:
    SummaryFaceOverview();
    virtual ~SummaryFaceOverview() {}

    virtual void initialize();

    /**
     * @brief Display total distance with unit label.
     * @param dist       Already-converted value in km or mi (view's responsibility).
     * @param isImperial Selects unit label: "mi" if true, "km" if false.
     */
    void setDistance(float dist, bool isImperial);

    /**
     * @brief Display average pace.
     * @param pace Already-converted value in sec/km or sec/mi (view's responsibility).
     *             Pass a value < 1.0 to show "---" (no data).
     */
    void setAvgPace(float pace);

    /** @brief Display total elapsed active time as "H:MM:SS". */
    void setTimer(std::time_t sec);

protected:
};

#endif // SUMMARYFACEOVERVIEW_HPP
