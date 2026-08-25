/**
 ******************************************************************************
 * @file    SensorStatusRow.hpp
 * @brief   Shared activity-app widget: a centred row of sensor-status icons
 *          (GPS, external HR) with three states each.
 ******************************************************************************
 *
 * Each icon has three states:
 *   Absent     - not shown (sensor disabled / not in use)
 *   Searching  - alternates dark <-> light (acquiring / connecting)
 *   Connected  - steady light (signal acquired / streaming)
 *
 * When only one icon is present it is centred in the row; when both are present
 * they are centred as a pair (GPS left, HR right).
 *
 * Header-only and configured with the host app's own BitmapIds, because
 * TouchGFX compiles bitmaps per-app. To adopt this in a new activity app:
 *   1. add the four icon assets to the app and regenerate,
 *   2. drop this container on the relevant screen(s) and call setIcons() once,
 *   3. forward two state values (setGps / setHr).
 * The widget self-registers for tick events while animating, so the host screen
 * does not need to forward ticks. Pass BITMAP_INVALID for a sensor the app does
 * not use (e.g. GPS on a non-GPS activity) and never leave it Absent.
 *
 ******************************************************************************
 */

#ifndef SDK_GUI_SENSOR_STATUS_ROW_HPP
#define SDK_GUI_SENSOR_STATUS_ROW_HPP

#include <touchgfx/Application.hpp>
#include <touchgfx/Bitmap.hpp>
#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Image.hpp>

#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateEx.hpp"  // HeartRateEx::Source

namespace SDK::GUI
{

class SensorStatusRow : public touchgfx::Container
{
public:
    enum class State : uint8_t { Absent, Searching, Connected };

    // Map a GPS fix flag to an icon state (a GPS activity is always acquiring
    // until it has a fix, so it is never Absent here).
    static State gpsState(bool fix)
    {
        return fix ? State::Connected : State::Searching;
    }

    // Map an SDK::Accessory::State value to the heart icon's state:
    // UNAVAILABLE(0)/IDLE(1) -> Absent, SEARCHING(2)/CONNECTING(3)/LOST(5) ->
    // Searching, CONNECTED(4) -> Connected.
    static State hrState(uint8_t accessoryState)
    {
        switch (accessoryState) {
            case 2: case 3: case 5: return State::Searching;
            case 4:                 return State::Connected;
            default:                return State::Absent;
        }
    }

    // In-activity variant: the heart icon reflects which source is actually
    // feeding HR right now (SDK::SensorDataParser::HeartRate::Source), not the
    // raw BLE link. Shown only while external HR is engaged (per the accessory
    // link state). Steady ("connected") requires BOTH the link to be CONNECTED
    // and the strap to be the live source; anything else — LOST/SEARCHING/
    // CONNECTING, or an optical/none source, or a stale EXTERNAL source while
    // the link is LOST — animates as searching. This tracks the arbiter's
    // freshness, so it updates within ~seconds of a dropout rather than waiting
    // out the BLE supervision timeout. (Pre-start screens keep using hrState()
    // to show acquisition progress.)
    static State hrStateFromSource(uint8_t accessoryState, uint8_t hrSource)
    {
        if (hrState(accessoryState) == State::Absent) {
            return State::Absent;   // external HR not engaged
        }
        constexpr uint8_t kAccessoryConnected = 4;  // SDK::Accessory::State::CONNECTED
        const bool live =
            accessoryState == kAccessoryConnected &&
            hrSource == static_cast<uint8_t>(
                    SDK::SensorDataParser::HeartRateEx::Source::EXTERNAL);
        return live ? State::Connected : State::Searching;
    }

    SensorStatusRow() = default;

    virtual ~SensorStatusRow()
    {
        if (mRegistered) {
            touchgfx::Application::getInstance()->unregisterTimerWidget(this);
        }
    }

    /**
     * @brief   One-time setup with the host app's own bitmaps. Pass
     *          BITMAP_INVALID for a sensor this app does not use.
     * @param   gap: horizontal spacing between the two icons when both shown.
     */
    void setIcons(touchgfx::BitmapId gpsDark, touchgfx::BitmapId gpsLight,
                  touchgfx::BitmapId hrDark, touchgfx::BitmapId hrLight,
                  int16_t gap = 24)
    {
        mGpsDark = gpsDark; mGpsLight = gpsLight;
        mHrDark  = hrDark;  mHrLight  = hrLight;
        mGap = gap;
        if (!mAdded) {
            add(mGpsImg);
            add(mHrImg);
            mAdded = true;
        }
        update();
    }

    void setGps(State s) { if (s != mGpsState) { mGpsState = s; update(); } }
    void setHr(State s)  { if (s != mHrState)  { mHrState  = s; update(); } }

    virtual void handleTickEvent() override
    {
        if (++mTick < kBlinkTicks) {
            return;
        }
        mTick = 0;
        mLightPhase = !mLightPhase;
        applyBitmaps();
    }

private:
    static const int16_t kBlinkTicks = 5;  // ~500 ms at the apps' 10 fps

    touchgfx::Image    mGpsImg;
    touchgfx::Image    mHrImg;
    touchgfx::BitmapId mGpsDark  { touchgfx::BITMAP_INVALID };
    touchgfx::BitmapId mGpsLight { touchgfx::BITMAP_INVALID };
    touchgfx::BitmapId mHrDark   { touchgfx::BITMAP_INVALID };
    touchgfx::BitmapId mHrLight  { touchgfx::BITMAP_INVALID };
    State    mGpsState { State::Absent };
    State    mHrState  { State::Absent };
    int16_t  mGap        { 24 };
    int16_t  mTick       { 0 };
    bool     mLightPhase { true };
    bool     mAdded      { false };
    bool     mRegistered { false };

    bool animating() const
    {
        return mGpsState == State::Searching || mHrState == State::Searching;
    }

    void setRegistered(bool on)
    {
        if (on == mRegistered) {
            return;
        }
        mRegistered = on;
        if (on) {
            touchgfx::Application::getInstance()->registerTimerWidget(this);
        } else {
            touchgfx::Application::getInstance()->unregisterTimerWidget(this);
        }
    }

    void update()
    {
        // Size the images from their (light) bitmaps so layout has dimensions.
        if (mGpsLight != touchgfx::BITMAP_INVALID) {
            mGpsImg.setBitmap(touchgfx::Bitmap(mGpsLight));
        }
        if (mHrLight != touchgfx::BITMAP_INVALID) {
            mHrImg.setBitmap(touchgfx::Bitmap(mHrLight));
        }
        layout();
        if (animating()) {
            mLightPhase = true;
            mTick = 0;
            setRegistered(true);
        } else {
            setRegistered(false);
        }
        applyBitmaps();
    }

    void layout()
    {
        const bool gpsOn = (mGpsState != State::Absent);
        const bool hrOn  = (mHrState  != State::Absent);
        const int16_t gw = mGpsImg.getWidth(),  gh = mGpsImg.getHeight();
        const int16_t hw = mHrImg.getWidth(),   hh = mHrImg.getHeight();
        const int16_t W = getWidth(), H = getHeight();

        // Icons are bottom-aligned in the row (per the design); centred
        // horizontally as a pair, or singly when only one is present.
        if (gpsOn && hrOn) {
            const int16_t total = static_cast<int16_t>(gw + mGap + hw);
            const int16_t x = static_cast<int16_t>((W - total) / 2);
            mGpsImg.setXY(x, static_cast<int16_t>(H - gh));
            mHrImg.setXY(static_cast<int16_t>(x + gw + mGap),
                         static_cast<int16_t>(H - hh));
        } else if (gpsOn) {
            mGpsImg.setXY(static_cast<int16_t>((W - gw) / 2),
                          static_cast<int16_t>(H - gh));
        } else if (hrOn) {
            mHrImg.setXY(static_cast<int16_t>((W - hw) / 2),
                         static_cast<int16_t>(H - hh));
        }
    }

    void applyBitmaps()
    {
        applyOne(mGpsImg, mGpsState, mGpsDark, mGpsLight);
        applyOne(mHrImg,  mHrState,  mHrDark,  mHrLight);
        invalidate();
    }

    void applyOne(touchgfx::Image& img, State s,
                  touchgfx::BitmapId dark, touchgfx::BitmapId light)
    {
        if (s == State::Absent) {
            img.setVisible(false);
            return;
        }
        img.setVisible(true);
        const touchgfx::BitmapId id = (s == State::Connected)
                ? light : (mLightPhase ? light : dark);
        img.setBitmap(touchgfx::Bitmap(id));
    }
};

} // namespace SDK::GUI

#endif  // SDK_GUI_SENSOR_STATUS_ROW_HPP
