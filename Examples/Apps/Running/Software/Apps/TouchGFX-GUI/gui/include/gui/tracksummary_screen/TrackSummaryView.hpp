#ifndef TRACKSUMMARYVIEW_HPP
#define TRACKSUMMARYVIEW_HPP

#include <gui_generated/tracksummary_screen/TrackSummaryViewBase.hpp>
#include <gui/tracksummary_screen/TrackSummaryPresenter.hpp>
#include <gui/containers/SummaryFaceLaps.hpp>
#include <gui/containers/SummaryFaceChart.hpp>
#include "ActivitySummary.hpp"

class TrackSummaryView : public TrackSummaryViewBase
{
public:
    TrackSummaryView();
    virtual ~TrackSummaryView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setSummary(const ActivitySummary& s, bool isImperial, bool isPaused);

protected:
    enum Face : uint8_t {
        FACE_MAP       = 0,
        FACE_OVERVIEW  = 1,
        FACE_HEARTRATE = 2,
        FACE_CHART     = 3,   ///< With two or more laps to chart
        FACE_LAPS      = 4,
        FACE_COUNT     = 5,
    };

    /// The lap pace chart, added in code (not a Designer container).
    SummaryFaceChart summaryFaceChart;

    uint8_t mCurrentFace    = FACE_MAP;
    bool    mIsImperial     = false;
    bool    mTrackIsPaused  = false;
    uint8_t mLapsPageCount  = 0;

    virtual void handleKeyEvent(uint8_t key) override;
    void updateFace();
    void updateScrollIndicator();
    uint8_t nextFace(uint8_t face) const;   ///< FACE_COUNT if none
    uint8_t prevFace(uint8_t face) const;
};

#endif // TRACKSUMMARYVIEW_HPP
