#ifndef SPHEREPICKER_HPP
#define SPHEREPICKER_HPP

#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <touchgfx/Callback.hpp>

/**
 * @brief A single narrow numeric column with the COROS "sphere" scroll effect.
 *
 * Shares OrbitMenu's technique -- every visible row's position/size/colour is
 * recomputed each frame from one fractional scalar so the numbers curve and
 * shrink toward the centre -- but tailored for a value picker: digits only, a
 * configurable column width/position, a cyclic 0..max range with a settable
 * step, and an active/inactive style. When active the centre is teal and the
 * neighbours are shown; when inactive only the centre value shows, in white.
 *
 * The column self-registers as a timer widget, so its animation runs itself.
 * Navigation is discrete: selectNext() / selectPrev() step by one value.
 */
class SpherePicker : public touchgfx::Container
{
public:
    SpherePicker();
    virtual ~SpherePicker();

    /** @brief Build the row widgets. Call once after construction. */
    void initialize();

    // ---- Geometry ----------------------------------------------------------
    /**
     * @brief Column box + centre line.
     * @param centerLocalY Y of the selected row within the box. The box extends
     *        below it far enough to show the lower neighbours; rows past the box
     *        bottom are clipped so a new value slides in from there.
     */
    void setColumn(int16_t x, int16_t y, int16_t width, int16_t height,
                   int16_t centerLocalY);

    // ---- Data --------------------------------------------------------------
    /**
     * @brief Set the value range: entries are 0, step, 2*step ... <= maxValue.
     * @param maxValue Highest value (e.g. 99 for minutes, 59 for seconds).
     * @param step     Increment between entries (1, or 5 for coarse seconds).
     */
    void setRange(int16_t maxValue, int16_t step);

    /** @brief Snap to the entry nearest @p value (no animation). */
    void setValue(int16_t value);
    /** @brief Currently selected value (already multiplied by the step). */
    int16_t getValue() const;

    // ---- Style -------------------------------------------------------------
    /** @brief Active = teal centre + visible neighbours; inactive = white centre only. */
    void setActive(bool active);

    // ---- Navigation --------------------------------------------------------
    void selectNext();
    void selectPrev();
    void setAnimationSteps(int16_t steps);

protected:
    virtual void handleTickEvent() override;

private:
    static const int16_t kVisible = 7;   ///< centre + up to 3 below (+ off-box buffer)

    void layout();
    void renderRow(int16_t slot, int16_t value, float absD,
                   int16_t centerY, bool isCenter);

    // Row widgets (recycled as the column scrolls).
    touchgfx::TextAreaWithOneWildcard mRows[kVisible];
    static const uint16_t             kBufSize = 4;      // "00" + NUL + slack
    touchgfx::Unicode::UnicodeChar    mBuffers[kVisible][kBufSize];

    int16_t mX        = 0;
    int16_t mWidth    = 120;
    int16_t mCenterY  = 76;  ///< Y of the selected row within the box.
    int16_t mCurveDir = 0;   ///< +1/-1: rows curve toward the screen centre; 0 = straight

    int16_t mMax    = 59;
    int16_t mStep   = 1;
    int16_t mCount  = 60;   ///< number of entries = mMax/mStep + 1

    bool    mActive = false;

    // Animation (mirrors OrbitMenu).
    float   mScrollPos = 0.0f;
    float   mTargetPos = 0.0f;
    float   mAnimStart = 0.0f;
    int16_t mAnimSteps = 3;
    int16_t mAnimStep  = 0;
    bool    mAnimating = false;

    void startAnimation();
    int16_t centerLineY() const;

    // Timer-widget registration. initialize() runs on every screen entry and the
    // FrontendHeap partition is reused, so guard against a double-register and
    // drop the pointer in the destructor -- otherwise TouchGFX keeps ticking this
    // object through the memory the next screen now occupies.
    bool mRegistered = false;
    void registerTimer();
    void unregisterTimer();
};

#endif // SPHEREPICKER_HPP
