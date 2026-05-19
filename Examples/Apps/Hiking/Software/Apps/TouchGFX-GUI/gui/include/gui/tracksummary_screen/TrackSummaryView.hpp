#ifndef TRACKSUMMARYVIEW_HPP
#define TRACKSUMMARYVIEW_HPP

#include <gui_generated/tracksummary_screen/TrackSummaryViewBase.hpp>
#include <gui/tracksummary_screen/TrackSummaryPresenter.hpp>
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
        FACE_MAP = 0,
        FACE_OVERVIEW = 1,
        FACE_HEARTRATE = 2,
    };

    uint8_t mCurrentFace = FACE_MAP;
    bool    mIsImperial = false;
    bool    mTrackIsPaused = false;

    virtual void handleKeyEvent(uint8_t key) override;
    void updateFace();
    void updateScrollIndicator();
};

#endif // TRACKSUMMARYVIEW_HPP
