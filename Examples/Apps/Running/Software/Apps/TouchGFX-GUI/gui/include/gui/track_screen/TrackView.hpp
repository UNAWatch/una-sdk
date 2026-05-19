#ifndef TRACKVIEW_HPP
#define TRACKVIEW_HPP

#include <gui_generated/track_screen/TrackViewBase.hpp>
#include <gui/track_screen/TrackPresenter.hpp>

class TrackView : public TrackViewBase
{
public:
    TrackView();
    virtual ~TrackView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setIntervalsMode(bool mode);
    void setPositionId(uint16_t id);
    uint16_t getPositionId();

    void setConfig(bool isImperial, const uint8_t* thresholds, uint8_t thresholdCount);
    void setTrackData(const Track::Data& data);

    void setTime(uint8_t h, uint8_t m);
    void setBatteryLevel(uint8_t level);
    void setGpsFix(bool state);

protected:

    virtual void handleKeyEvent(uint8_t key) override;

    bool     mIntervalsMode = false;
    uint16_t mCurrentFaceId = 0;
    bool     mIsImperial    = false;
    uint8_t  mHrThresholds[App::Config::kHrThresholdsCount] = {};
    uint8_t  mHrThresholdCount = 0;
};

#endif // TRACKVIEW_HPP
