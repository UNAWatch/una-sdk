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

    void setPositionId(uint16_t id);
    uint16_t getPositionId();

    void setConfig(bool isImperial, const uint8_t* thresholds, uint8_t thresholdCount);
    void setTimeFormat(bool is12Hour);
    void setTrackData(const Track::Data& data);

    void setTime(uint8_t h, uint8_t m);
    void setBatteryLevel(uint8_t level);
    void setAccessoryStatus(uint8_t state);

protected:

    virtual void handleKeyEvent(uint8_t key) override;

    // In-activity HR icon reflects the live source, recomputed from both the
    // accessory link state (engaged?) and the latest HR-sample source.
    void updateHrIcon();

    uint16_t mCurrentFaceId = 0;
    bool     mIsImperial    = false;
    bool     mIs12Hour      = false;
    uint8_t  mHrThresholds[App::Config::kHrThresholdsCount] = {};
    uint8_t  mHrThresholdCount = 0;
    uint8_t  mAccessoryState = 0;  // last SDK::Accessory::State (engaged?)
    uint8_t  mHrSource       = 0;  // last HR-sample source (steady vs flashing)
};

#endif // TRACKVIEW_HPP
