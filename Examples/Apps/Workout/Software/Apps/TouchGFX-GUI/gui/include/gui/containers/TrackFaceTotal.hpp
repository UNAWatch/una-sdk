#ifndef TRACKFACETOTAL_HPP
#define TRACKFACETOTAL_HPP

#include <gui_generated/containers/TrackFaceTotalBase.hpp>

/**
 * @brief Track face showing live HR, calories and elapsed time.
 *
 * All values are display-ready -- unit conversion is the view's responsibility.
 */
class TrackFaceTotal : public TrackFaceTotalBase
{
public:
    TrackFaceTotal();
    virtual ~TrackFaceTotal() {}

    virtual void initialize();

    void setHR(float hr, const uint8_t* thresholds, uint8_t thresholdCount);
    void setCalories(float calories);

    /** @brief Display total elapsed time as "H:MM:SS". */
    void setTimer(std::time_t sec);

protected:
};

#endif // TRACKFACETOTAL_HPP
