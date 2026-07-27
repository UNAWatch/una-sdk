#ifndef ORBITMENU_HPP
#define ORBITMENU_HPP

#include <gui_generated/containers/OrbitMenuBase.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <touchgfx/hal/Types.hpp>
#include <touchgfx/Callback.hpp>

/**
 * @brief One row of OrbitMenu: an icon plus a label.
 *
 * Presentational only -- holds both icon sizes (60 and 30) and the label, and
 * renders itself at the size/position/alpha the parent computes for the current
 * frame. The parent (OrbitMenu) owns all geometry and animation logic.
 */
class OrbitMenuItem : public touchgfx::Container
{
public:
    OrbitMenuItem();
    ~OrbitMenuItem();
    void initialize();

    /**
     * @brief Assign the content shown by this row (called when the slot recycles).
     *
     * Icons may be static (BitmapId) or external raw ABGR2222 pixels; external
     * takes priority when its pointer is non-null. Both sizes are kept so the
     * tier swap in render() is just a setBitmap (no dynamic-bitmap churn).
     * Owned dynamic bitmaps are freed here and in the destructor.
     */
    void setData(touchgfx::BitmapId static60, touchgfx::BitmapId static30,
                 const uint8_t *ext60, const uint8_t *ext30,
                 const char *label, touchgfx::TypedTextId labelId);

    /**
     * @brief Render this row for the current frame.
     * @param bigIcon   true -> 60px icon; false -> 30px icon.
     * @param labelFont Typed text (font + left alignment) for the label.
     * @param labelColor Label colour (must be an exact palette entry).
     * @param iconCenterX Icon centre X (row-local) -- keeps the size swap from
     *                    shifting the icon sideways.
     * @param textX     Text left X (row-local).
     * @param centerY   Vertical centre of the row.
     */
    void render(bool bigIcon, touchgfx::TypedTextId labelFont,
                touchgfx::colortype labelColor, int16_t iconCenterX, int16_t textX, int16_t centerY);

private:
    static const uint16_t kLabelSize = 24;

    void freeDynamic();        ///< Delete any dynamic bitmaps this row owns.

    touchgfx::Image                    icon;
    touchgfx::TextAreaWithOneWildcard  label;
    touchgfx::Unicode::UnicodeChar     labelBuffer[kLabelSize];

    touchgfx::BitmapId mIcon60 = 0;                       ///< Currently used id (static or dynamic).
    touchgfx::BitmapId mIcon30 = 0;
    touchgfx::BitmapId mDyn60  = touchgfx::BITMAP_INVALID;///< Owned dynamic bitmap, if external.
    touchgfx::BitmapId mDyn30  = touchgfx::BITMAP_INVALID;
};


/**
 * @brief Layout anchor for one visible position (centre, +/-1 or +/-2).
 *
 * yOffset is the distance from the menu centre (applied up or down by the sign
 * of the row's distance); iconX / textX are the row-local left edges. Rows at
 * fractional distances are linear-interpolated between the two bracketing
 * anchors.
 */
struct PosAnchor
{
    int16_t yOffset;
    int16_t iconX;
    int16_t textX;
};

/**
 * @brief COROS-style "sphere" wheel: items shrink and curve toward the centre.
 *
 * Path B -- a self-contained, button-stepped scroller (no ScrollWheel). Every
 * frame each visible row's position/size/colour is computed from a single
 * scalar mScrollPos (fractional selected index), so the list reads as if
 * wrapped around a ball. Navigation is discrete: selectNext()/selectPrev().
 *
 * Layout is screen-tunable at runtime:
 *   - the container's own size is the visible area (children are clipped to it),
 *     so position the container below the title and call setVisibleHeight();
 *   - setCenterOffset() shifts the selected row from the container centre;
 *   - setAnchors() overrides the three position anchors.
 * Max 5 rows are shown (centre + 2 above + 2 below). Font/colour tiers live as
 * constants in OrbitMenu.cpp.
 */
/**
 * @brief One label tier: the font + colour used while |distance| < maxAbsD.
 *
 * Ordered nearest-first. The last entry should use a very large maxAbsD so it
 * acts as the catch-all for the farthest rows.
 */
struct OrbitTier
{
    float                 maxAbsD;
    touchgfx::TypedTextId font;
    touchgfx::colortype   color;
};

class OrbitMenu : public OrbitMenuBase
{
public:
    struct Entry
    {
        touchgfx::BitmapId    icon60  = 0;       ///< Static icon (fixed lists / quick checks).
        touchgfx::BitmapId    icon30  = 0;
        const char           *label   = nullptr; ///< Raw label; overrides labelId when set (dynamic text).
        touchgfx::TypedTextId labelId = TYPED_TEXT_INVALID; ///< Localised label for a known item.
        const uint8_t        *ext60   = nullptr; ///< External 60x60 ABGR2222 (real app data); wins if set.
        const uint8_t        *ext30   = nullptr; ///< External 30x30 ABGR2222.
    };

    /** The three position anchors: centre, +/-1 (pos1), +/-2 (pos2). */
    struct Anchors
    {
        PosAnchor center;
        PosAnchor pos1;
        PosAnchor pos2;
    };

    OrbitMenu();
    virtual ~OrbitMenu() {}

    virtual void initialize() override;

    /** Populate the menu. Copies up to kMaxEntries entries. */
    void setItems(const Entry *entries, int16_t count);

    /**
     * @brief Replace one entry's label and re-render in place.
     *
     * Does not disturb the scroll position. The label pointer must stay valid
     * for as long as the entry is shown (same rule as setItems).
     */
    void setEntryLabel(int16_t index, const char *label);

    /** @brief Replace one entry's label with a localised text id (see above). */
    void setEntryLabel(int16_t index, touchgfx::TypedTextId labelId);

    void    selectNext();
    void    selectPrev();
    int16_t getSelected() const;
    /** Jump to an item instantly (no animation) -- e.g. to restore a position. */
    void    setSelected(int16_t index);

    // ---- Runtime layout ----------------------------------------------------
    /** The container height IS the visible area (children clip to it); resizing relayouts. */
    void           setHeight(int16_t height) override;
    /** Shift the selected row up/down from the container centre (default 0). */
    void           setCenterOffset(int16_t offset);
    /** Override the three position anchors (default = built-in). */
    void           setAnchors(const Anchors &anchors);
    const Anchors &getAnchors() const { return mAnchors; }
    /**
     * @brief Minimum item count at which the menu wraps (circular); below it the
     * menu is bounded. Default 3. Set to match the visible window: a window of
     * N rows needs >= N items to wrap without duplicates.
     */
    void           setCircularMinItems(int16_t minItems);
    /**
     * @brief Ticks per item transition (default 3 => current + 2 intermediate +
     * final, ~300ms at 10Hz). Higher = slower/smoother. Clamped to >= 1.
     */
    void           setAnimationSteps(int16_t steps);

    /** @brief Register a callback fired once when a selectNext/Prev animation settles. */
    void setAnimationEndedCallback(touchgfx::GenericCallback<> &cb) { mpAnimEndedCb = &cb; }

    /**
     * @brief Override the label tiers (font + colour by distance).
     * @param tiers Nearest-first array; the pointer must outlive the menu.
     * @param count Number of entries. Pass nullptr/0 to restore the default.
     *
     * The default is a large-centre number ladder; a single uniform tier turns
     * the wheel into a plain same-size scroller (e.g. a text option list).
     */
    void setTiers(const OrbitTier *tiers, int16_t count);

protected:
    virtual void handleTickEvent() override;

private:
    static const int16_t kVisible    = 5;   ///< Slots rendered (centre +/- 2).
    static const int16_t kMaxEntries = 16;

    void    refreshEntry(int16_t index); ///< Re-fetch an entry's slot after a label change.
    void    layout();           ///< Place every slot from mScrollPos.
    void    reorderByDepth();   ///< Draw farthest rows first, centre on top.
    void    startAnimation();   ///< (Re)start easing toward mTargetPos.
    int16_t centerLineY() const;///< Container-local Y of the selected row.

    OrbitMenuItem mItems[kVisible];
    int16_t       mSlotDataIdx[kVisible]; ///< Data index currently in each slot.
    float         mSlotAbsD[kVisible];    ///< |distance from centre| per slot.

    Entry   mEntries[kMaxEntries];
    int16_t mCount           = 0;
    int16_t mCircularMinItems = 3;    ///< Wrap at/above this count; bounded below it.
    bool    mCircular        = false; ///< mCount >= mCircularMinItems.

    int16_t mCenterOffset = 0;            ///< Selected-row shift from container centre.
    Anchors mAnchors = { { 0, 22,  87 },  ///< centre  (60px icon)
                         { 50, 45,  92 },  ///< +/-1    (30px icon, pushed right)
                         { 80, 71, 108 } };///< +/-2

    touchgfx::GenericCallback<>* mpAnimEndedCb = nullptr;

    const OrbitTier* mpTiers    = nullptr;  ///< nullptr = built-in default ladder.
    int16_t          mTierCount = 0;
    const OrbitTier& pickTier(float absD) const;

    float   mScrollPos = 0.0f; ///< Animated fractional selected index.
    float   mTargetPos = 0.0f; ///< Integer target we are easing toward.
    float   mAnimStart = 0.0f;
    int16_t mAnimSteps = 3;    ///< 4 positions total: current + 2 intermediate + final.
    int16_t mAnimStep  = 0;    ///< 0..mAnimSteps, advanced each tick.
    bool    mAnimating = false;
};

#endif // ORBITMENU_HPP
