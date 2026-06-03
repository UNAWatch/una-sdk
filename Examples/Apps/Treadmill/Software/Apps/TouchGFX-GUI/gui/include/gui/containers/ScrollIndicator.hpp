#ifndef SCROLLINDICATOR_HPP
#define SCROLLINDICATOR_HPP

#include <gui_generated/containers/ScrollIndicatorBase.hpp>
#include <cstdint>

/**
 * @brief Appearance and geometry configuration for ScrollIndicator.
 *
 * Two predefined configs are available as ScrollIndicator::kBig and kSmall.
 * Both share the same circle geometry set by the Designer (center, radius,
 * line width) but differ in rail arc range, handle length, and colours.
 *
 * You can also build a custom config:
 * @code
 *   ScrollIndicatorConfig cfg = ScrollIndicator::kBig;
 *   cfg.handleColor = SDK::GUI::Color::rgb(255, 128, 0);
 *   scrollIndicator.setConfig(cfg);
 * @endcode
 */
struct ScrollIndicatorConfig
{
    float    handleLen   = 18.0f;  ///< Handle arc span (degrees)
    float    railMin     = 232.0f; ///< Rail low bound (degrees)
    float    railMax     = 308.0f; ///< Rail high bound (degrees)
    uint32_t handleColor = 0;      ///< Handle and overflow-handle colour (0xFFRRGGBB)
    uint32_t railColor   = 0;      ///< Background rail colour (0xFFRRGGBB)
};

// ---------------------------------------------------------------------------

/**
 * @brief Circular arc position indicator for scroll-wheel menus.
 *
 * Renders a sliding arc handle on a circular rail that tracks the currently
 * selected item in a list. Supports smooth animated transitions including
 * wrap-around (last -> first and first -> last) using a dual-handle technique.
 *
 * The geometry (circle center, radius, line width) is fixed by the
 * Designer-generated base. Only the arc range and colours are configurable
 * at runtime via setConfig().
 *
 * Usage:
 * @code
 *   scrollIndicator.setConfig(ScrollIndicator::kSmall);  // optional
 *   scrollIndicator.setCount(5);
 *   scrollIndicator.setActiveId(0);
 *   scrollIndicator.animateToId(2, 8);
 * @endcode
 */
class ScrollIndicator : public ScrollIndicatorBase
{
public:
    ScrollIndicator();
    virtual ~ScrollIndicator();

    virtual void initialize();

    // ------------------------------------------------------------------
    // Predefined configs
    // ------------------------------------------------------------------

    /** Config matching the Big indicator: 76 deg rail, 18 deg handle. */
    static constexpr ScrollIndicatorConfig kBig   = { 18.0f, 232.0f, 308.0f, 0xC0C0C0u, 0x404040u };

    /** Config matching the Small indicator: 36 deg rail, 10 deg handle. */
    static constexpr ScrollIndicatorConfig kSmall = { 10.0f, 252.0f, 288.0f, 0xC0C0C0u, 0x404040u };

    // ------------------------------------------------------------------
    // Configuration
    // ------------------------------------------------------------------

    /**
     * @brief Apply an arc config (range + colours). Safe to call at any time.
     *
     * The Designer-generated base already applies kBig defaults in its
     * constructor, so this call is only needed to switch to kSmall or a
     * custom config.
     */
    void setConfig(const ScrollIndicatorConfig& cfg);

    // ------------------------------------------------------------------
    // Item navigation
    // ------------------------------------------------------------------

    /**
     * @brief Set the total number of items. Resets position to 0.
     * The handle is hidden when count <= 1.
     */
    void setCount(uint16_t count);

    /** @brief Instantly move the handle to the given item index. */
    void setActiveId(uint16_t index);

    /**
     * @brief Animate the handle to the given item index.
     * @param index          Target item (wraps around if out of range).
     * @param animationSteps Tick count for the animation; <= 0 means instant.
     */
    void animateToId(int16_t index, int16_t animationSteps = 0);

    /** @brief Return the current (or target, if animating) item index. */
    uint16_t getActiveId();

    virtual void handleTickEvent() override;

protected:
    // Arc geometry -- mirrors ScrollIndicatorConfig, updated by setConfig()
    float mHandleLen = 18.0f;
    float mRailMin   = 232.0f;
    float mRailMax   = 308.0f;

    uint16_t mCount    = 1;
    uint16_t mPosition = 0;

    bool     mAnimationRunning  = false;
    uint16_t mAnimationCounter  = 0;
    uint16_t mAnimationDuration = 0;
    float    mAnimationStartPos = 0.0f;
    float    mAnimationEndPos   = 0.0f;

    bool  mAnimWrap          = false; ///< True when the animation wraps around the rail ends
    float mAnimOutgoingEnd   = 0.0f;  ///< Outgoing handle's target position (slides off rail edge)
    float mAnimIncomingStart = 0.0f;  ///< Incoming handle's start position (slides in from opposite edge)

private:
    void  nextAnimationStep();
    float getStartAngle(uint16_t index) const;

    /**
     * @brief Stop any running animation and clean up the overflow handle.
     *
     * Must be called before setActiveId() or before starting a new animation
     * when an overflow animation might be in progress.
     */
    void stopAnimation();
};

#endif // SCROLLINDICATOR_HPP
