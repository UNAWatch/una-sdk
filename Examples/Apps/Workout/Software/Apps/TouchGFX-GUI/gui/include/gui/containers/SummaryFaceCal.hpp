#ifndef SUMMARYFACECAL_HPP
#define SUMMARYFACECAL_HPP

#include <gui_generated/containers/SummaryFaceCalBase.hpp>
#include <ctime>

/**
 * @brief Summary face: session time and total / active / resting calories (kcal).
 *
 * Swipe order on TrackSummary: after overview, before the heart-rate face.
 * Label text widgets are set in TouchGFX Designer.
 */
class SummaryFaceCal : public SummaryFaceCalBase
{
public:
    SummaryFaceCal();
    virtual ~SummaryFaceCal() {}

    virtual void initialize();

    void setTotalTime(std::time_t sec);
    void setTotalCalories(float kcal);
    void setActiveCalories(float kcal);
    void setRestingCalories(float kcal);

protected:
};

#endif // SUMMARYFACECAL_HPP
