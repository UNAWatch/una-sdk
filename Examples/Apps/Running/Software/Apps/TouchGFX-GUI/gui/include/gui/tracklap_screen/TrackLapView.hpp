#ifndef TRACKLAPVIEW_HPP
#define TRACKLAPVIEW_HPP

#include <gui_generated/tracklap_screen/TrackLapViewBase.hpp>
#include <gui/tracklap_screen/TrackLapPresenter.hpp>
#include <touchgfx/Callback.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <SDK/GUI/CountdownTimer.hpp>

#include "LapCompare.hpp"

class TrackLapView : public TrackLapViewBase
{
public:
    TrackLapView();
    virtual ~TrackLapView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setUnitsImperial(bool isImperial);
    void setLapNum(uint32_t n);
    void setDistance(float metres);
    void setTimer(std::time_t sec);
    void setPace(float secPerM);
    /// The chip under the pace: against the lap before, by pace.
    void setDelta(const LapCompare::Delta& d);
    /// Show for the full five seconds again from now.
    void restartDismiss() { mDismissTimer.start(); }

protected:
    virtual void handleKeyEvent(uint8_t key) override;

private:
    void onDismiss();

    bool mIsImperial = false;

    // The delta chip: black words on green (faster), coral (slower) or
    // white (even); hidden with nothing to compare.
    static constexpr uint16_t kChipTextSize = 24;
    touchgfx::Box                          mChip;
    touchgfx::TextAreaWithOneWildcard      mChipText;
    touchgfx::Unicode::UnicodeChar         mChipBuf[kChipTextSize] {};

    SDK::GUI::CountdownTimer            mDismissTimer;
    touchgfx::Callback<TrackLapView>    mDismissCb;
};

#endif // TRACKLAPVIEW_HPP
