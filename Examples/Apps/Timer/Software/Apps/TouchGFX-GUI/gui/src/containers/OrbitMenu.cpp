#include <gui/containers/OrbitMenu.hpp>
#include <touchgfx/Application.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <SDK/GUI/Color.hpp>
#include <cmath>

// ============================================================================
// Tuning knobs -- adjust these to shape the effect, then rebuild the simulator.
// ============================================================================
namespace {

// Position anchors are runtime-configurable; defaults live in OrbitMenu.hpp.

float lerpf(float a, float b, float t) { return a + (b - a) * t; }

// --- Default label tiers: font + colour by distance from centre ----------
// Large centre value with an even step-down toward the edges. Rest positions
// land at |d| = 0 / 1 / 2 (=> 60 / 35 / 20); the extra thresholds cover the
// in-between animation frames so the size eases smoothly. A screen may replace
// this via setTiers() (e.g. a uniform tier for a plain text list).
const OrbitTier kDefaultTiers[] = {
    { 0.30f,  T_TMP_SEMIBOLD_60, SDK::GUI::Color::WHITE },
    { 0.60f,  T_TMP_SEMIBOLD_40, SDK::GUI::Color::WHITE },
    { 0.90f,  T_TMP_SEMIBOLD_30, SDK::GUI::Color::GRAY  },
    { 1.0e9f, T_TMP_SEMIBOLD_25, SDK::GUI::Color::GRAY  }, // all rest neighbours 25
};

} // namespace


// ============================================================================
// OrbitMenuItem
// ============================================================================

OrbitMenuItem::OrbitMenuItem()
{
}

void OrbitMenuItem::initialize()
{
    label.setColor(SDK::GUI::Color::WHITE);
    labelBuffer[0] = 0;
    label.setWildcard(labelBuffer);
    label.setTypedText(touchgfx::TypedText(T_TMP_SEMIBOLD_25_L));
    add(label);
}

void OrbitMenuItem::setData(const char *text, touchgfx::TypedTextId labelId)
{
    // A raw string wins (dynamic values); otherwise pull a localised label.
    if (text != nullptr) {
        touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t *>(text),
                                    labelBuffer, kLabelSize);
    } else if (labelId != TYPED_TEXT_INVALID) {
        touchgfx::Unicode::snprintf(labelBuffer, kLabelSize, "%s",
                                    touchgfx::TypedText(labelId).getText());
    } else {
        labelBuffer[0] = 0;
    }
}

void OrbitMenuItem::render(touchgfx::TypedTextId labelFont,
                           touchgfx::colortype labelColor, int16_t centerY)
{
    // Tall enough that the 60px centre value never clips top/bottom.
    const int16_t rowH = 84;

    label.setTypedText(touchgfx::TypedText(labelFont));
    label.setColor(labelColor);
    // Centre the (left-aligned) label across the full row width.
    label.setPosition(0, 0, 240, rowH);
    const int16_t w = label.getTextWidth();
    label.setPosition(static_cast<int16_t>((240 - w) / 2), 0,
                      static_cast<int16_t>(w + 4), rowH);
    label.resizeHeightToCurrentText();
    // Centre by the digit's real cap box, not the line box: a digit rests on the
    // baseline with height == glyph height, so placing the baseline at
    // rowH/2 + digitH/2 lands the digit's centre on the row centre for every
    // font size. This removes the ascent-space bias that made large rows read
    // low (tight top gap) and keeps the value from clipping.
    const touchgfx::Font* font = label.getTypedText().getFont();
    const touchgfx::GlyphNode* zero = (font != nullptr) ? font->getGlyph('0') : nullptr;
    if (zero != nullptr) {
        label.setBaselineY(static_cast<int16_t>(rowH / 2 + zero->height() / 2));
    } else {
        label.setY(static_cast<int16_t>((rowH - label.getHeight()) / 2));
    }

    setWidthHeight(240, rowH);
    setXY(0, static_cast<int16_t>(centerY - rowH / 2));
}


// ============================================================================
// OrbitMenu
// ============================================================================

OrbitMenu::OrbitMenu()
{
}

OrbitMenu::~OrbitMenu()
{
    // initialize() registered this container as a timer widget; the FrontendHeap
    // partition is reused, so drop the stale pointer or TouchGFX ticks a corpse.
    touchgfx::Application::getInstance()->unregisterTimerWidget(this);
}

void OrbitMenu::initialize()
{
    OrbitMenuBase::initialize();

    setHeight(200); // default visible area; the screen may override at runtime.

    for (int16_t i = 0; i < kVisible; i++) {
        mItems[i].initialize();
        mSlotDataIdx[i] = -1;
        mSlotAbsD[i]    = 0.0f;
        add(mItems[i]);
    }

    touchgfx::Application::getInstance()->registerTimerWidget(this);
}

int16_t OrbitMenu::centerLineY() const
{
    return static_cast<int16_t>(getHeight() / 2 + mCenterOffset);
}

void OrbitMenu::setHeight(int16_t height)
{
    invalidate();        // clear the old (possibly taller) area first
    OrbitMenuBase::setHeight(height);
    layout();
    invalidate();
}

void OrbitMenu::setCenterOffset(int16_t offset)
{
    mCenterOffset = offset;
    layout();
    invalidate();
}

void OrbitMenu::setAnchors(const Anchors &anchors)
{
    mAnchors = anchors;
    layout();
    invalidate();
}

void OrbitMenu::setCircularMinItems(int16_t minItems)
{
    mCircularMinItems = (minItems < 2) ? 2 : minItems; // 1 item is always single
    mCircular = (mCount >= mCircularMinItems);
    layout();
    invalidate();
}

void OrbitMenu::setAnimationSteps(int16_t steps)
{
    mAnimSteps = (steps < 1) ? 1 : steps; // takes effect on the next transition
}

void OrbitMenu::setItems(const Entry *entries, int16_t count)
{
    mCount = (count > kMaxEntries) ? kMaxEntries : count;
    for (int16_t i = 0; i < mCount; i++) {
        mEntries[i] = entries[i];
    }

    // Wrap only when there are enough items that the visible window shows no
    // duplicates; below the threshold stay bounded (no repeated rows).
    mCircular = (mCount >= mCircularMinItems);

    mScrollPos = 0.0f;
    mTargetPos = 0.0f;
    mAnimating = false;

    for (int16_t i = 0; i < kVisible; i++) {
        mSlotDataIdx[i] = -1;
    }

    layout();
    invalidate();
}

void OrbitMenu::setTiers(const OrbitTier *tiers, int16_t count)
{
    if (tiers != nullptr && count > 0) {
        mpTiers    = tiers;
        mTierCount = count;
    } else {
        mpTiers    = nullptr;   // fall back to the built-in default
        mTierCount = 0;
    }
    layout();
    invalidate();
}

const OrbitTier& OrbitMenu::pickTier(float absD) const
{
    const OrbitTier* tiers = (mpTiers != nullptr) ? mpTiers : kDefaultTiers;
    const int16_t    count = (mpTiers != nullptr)
        ? mTierCount
        : static_cast<int16_t>(sizeof(kDefaultTiers) / sizeof(kDefaultTiers[0]));

    for (int16_t i = 0; i < count; i++) {
        if (absD < tiers[i].maxAbsD) {
            return tiers[i];
        }
    }
    return tiers[count - 1];
}

void OrbitMenu::setEntryLabel(int16_t index, const char *label)
{
    if (index < 0 || index >= mCount) {
        return;
    }
    mEntries[index].label   = label;
    mEntries[index].labelId = TYPED_TEXT_INVALID;
    refreshEntry(index);
}

void OrbitMenu::setEntryLabel(int16_t index, touchgfx::TypedTextId labelId)
{
    if (index < 0 || index >= mCount) {
        return;
    }
    mEntries[index].label   = nullptr;
    mEntries[index].labelId = labelId;
    refreshEntry(index);
}

void OrbitMenu::refreshEntry(int16_t index)
{
    // Force a re-fetch of the slot that currently holds this entry.
    for (int16_t s = 0; s < kVisible; s++) {
        if (mSlotDataIdx[s] == index) {
            mSlotDataIdx[s] = -1;
        }
    }
    layout();
    invalidate();
}

int16_t OrbitMenu::getSelected() const
{
    if (mCount <= 0) {
        return 0;
    }
    int16_t idx = static_cast<int16_t>(lroundf(mTargetPos));
    if (mCircular) {
        idx %= mCount;
        return (idx < 0) ? idx + mCount : idx;
    }
    if (idx < 0) {
        idx = 0;
    } else if (idx >= mCount) {
        idx = mCount - 1;
    }
    return idx;
}

void OrbitMenu::setSelected(int16_t index)
{
    if (mCount <= 0) {
        return;
    }
    if (!mCircular) {
        if (index < 0) {
            index = 0;
        } else if (index >= mCount) {
            index = mCount - 1;
        }
    }
    mScrollPos = static_cast<float>(index);
    mTargetPos = mScrollPos;
    mAnimating = false;
    layout();
    invalidate();
}

void OrbitMenu::selectNext()
{
    if (mCount <= 1) {
        return;
    }
    // Bounded (1/2 items): stop at the last item; circular: wrap freely.
    if (!mCircular && lroundf(mTargetPos) >= mCount - 1) {
        return;
    }
    mTargetPos += 1.0f;
    startAnimation();
}

void OrbitMenu::selectPrev()
{
    if (mCount <= 1) {
        return;
    }
    if (!mCircular && lroundf(mTargetPos) <= 0) {
        return;
    }
    mTargetPos -= 1.0f;
    startAnimation();
}

void OrbitMenu::startAnimation()
{
    mAnimStart = mScrollPos;
    mAnimStep  = 0;
    mAnimating = true;
}

void OrbitMenu::handleTickEvent()
{
    if (!mAnimating) {
        return;
    }

    mAnimStep++;
    // Linear progress so the intermediate font sizes land evenly across the
    // 2 in-between frames (easeOut would bunch them up at the start).
    const float f = static_cast<float>(mAnimStep) / static_cast<float>(mAnimSteps);
    mScrollPos = mAnimStart + (mTargetPos - mAnimStart) * f;

    bool justEnded = false;
    if (mAnimStep >= mAnimSteps) {
        mScrollPos = mTargetPos;
        mAnimating = false;
        justEnded  = true;
    }

    layout();
    invalidate();

    if (justEnded && mpAnimEndedCb != nullptr && mpAnimEndedCb->isValid()) {
        mpAnimEndedCb->execute();
    }
}

void OrbitMenu::layout()
{
    if (mCount <= 0) {
        // No data: hide every row so a previously-populated list leaves nothing behind.
        for (int16_t s = 0; s < kVisible; s++) {
            mItems[s].setVisible(false);
            mSlotDataIdx[s] = -1;
        }
        return;
    }

    const int16_t baseCenter = static_cast<int16_t>(lroundf(mScrollPos));

    for (int16_t s = 0; s < kVisible; s++) {
        const int16_t offset  = s - kVisible / 2;          // -2..+2
        const int16_t virt    = baseCenter + offset;
        const float   d       = static_cast<float>(virt) - mScrollPos;

        int16_t dataIdx;
        if (mCircular) {
            dataIdx = virt % mCount;
            if (dataIdx < 0) {
                dataIdx += mCount;
            }
        } else if (virt < 0 || virt >= mCount) {
            // Bounded (1/2 items): slots past either end hold no item -- hide them.
            mItems[s].setVisible(false);
            mSlotDataIdx[s] = -1;
            mSlotAbsD[s]    = static_cast<float>(kVisible); // sort to the back
            continue;
        } else {
            dataIdx = virt;
        }

        if (dataIdx != mSlotDataIdx[s]) {
            const Entry &e = mEntries[dataIdx];
            mItems[s].setData(e.label, e.labelId);
            mSlotDataIdx[s] = dataIdx;
        }

        const float ad = fabsf(d);

        // Interpolate the y-offset between the two bracketing anchors:
        // |d| in [0,1) -> centre..near, [1,2) -> near..far, >=2 -> far.
        const PosAnchor *a0;
        const PosAnchor *a1;
        float            f;
        if (ad < 1.0f) {
            a0 = &mAnchors.center; a1 = &mAnchors.pos1; f = ad;
        } else if (ad < 2.0f) {
            a0 = &mAnchors.pos1;   a1 = &mAnchors.pos2; f = ad - 1.0f;
        } else {
            a0 = &mAnchors.pos2;   a1 = &mAnchors.pos2; f = 0.0f;
        }

        const float yOff = lerpf(a0->yOffset, a1->yOffset, f);
        const float y    = centerLineY() + (d < 0.0f ? -yOff : yOff);

        // Rows outside the container are clipped automatically (top + bottom),
        // so no manual visibility limit is needed -- the container size is the
        // visible area.
        const OrbitTier &tier = pickTier(ad);
        mItems[s].setVisible(true); // may have been hidden by an empty list
        mItems[s].render(tier.font, tier.color, static_cast<int16_t>(y));
        mSlotAbsD[s] = ad;
    }

    reorderByDepth();
}

void OrbitMenu::reorderByDepth()
{
    // Re-add rows farthest-first so the (largest) centre row draws on top.
    int16_t order[kVisible];
    for (int16_t i = 0; i < kVisible; i++) {
        order[i] = i;
    }
    for (int16_t i = 0; i < kVisible - 1; i++) {
        for (int16_t j = 0; j < kVisible - 1 - i; j++) {
            if (mSlotAbsD[order[j]] < mSlotAbsD[order[j + 1]]) {
                const int16_t tmp = order[j];
                order[j]     = order[j + 1];
                order[j + 1] = tmp;
            }
        }
    }

    for (int16_t i = 0; i < kVisible; i++) {
        remove(mItems[order[i]]);
    }
    for (int16_t i = 0; i < kVisible; i++) {
        add(mItems[order[i]]);
    }
}
