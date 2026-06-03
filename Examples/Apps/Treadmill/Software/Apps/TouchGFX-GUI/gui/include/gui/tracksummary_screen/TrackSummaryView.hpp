#ifndef TRACKSUMMARYVIEW_HPP
#define TRACKSUMMARYVIEW_HPP

#include <gui_generated/tracksummary_screen/TrackSummaryViewBase.hpp>
#include <gui/tracksummary_screen/TrackSummaryPresenter.hpp>
#include <gui/containers/SummaryFaceLaps.hpp>
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
        FACE_OVERVIEW  = 0,
        FACE_HEARTRATE = 1,
        FACE_LAPS      = 2,
    };

    uint8_t mCurrentFace    = FACE_OVERVIEW;
    bool    mIsImperial     = false;
    bool    mTrackIsPaused  = false;
    uint8_t mLapsPageCount  = 0;

    virtual void handleKeyEvent(uint8_t key) override;
    void updateFace();
    void updateScrollIndicator();
};

#endif // TRACKSUMMARYVIEW_HPP
