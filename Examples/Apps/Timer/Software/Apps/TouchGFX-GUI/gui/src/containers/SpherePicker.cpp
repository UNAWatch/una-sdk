#include <gui/containers/SpherePicker.hpp>
#include <touchgfx/Application.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <SDK/GUI/Color.hpp>
#include <math.h>

namespace {

// Vertical anchors: |y| distance from the centre line for the rest positions.
const int16_t kYoffPos1 = 46;   // +/-1
const int16_t kYoffPos2 = 78;   // +/-2

const int16_t kSlotBoxH = 64;   // per-row box height (fits the 40px centre)

float lerpf(float a, float b, float t) { return a + (b - a) * t; }

// Font by distance from the centre. Rest positions land on 40 / 25 / 25.
touchgfx::TypedTextId fontFor(float absD)
{
    if (absD < 0.35f) return T_TMP_SEMIBOLD_40;
    if (absD < 0.75f) return T_TMP_SEMIBOLD_30;
    return T_TMP_SEMIBOLD_25;
}

} // namespace


SpherePicker::SpherePicker()
{
}

void SpherePicker::initialize()
{
    for (int16_t i = 0; i < kVisible; i++) {
        mBuffers[i][0] = 0;
        mRows[i].setWildcard(mBuffers[i]);
        mRows[i].setTypedText(touchgfx::TypedText(T_TMP_SEMIBOLD_40));
        mRows[i].setColor(SDK::GUI::Color::WHITE);
        add(mRows[i]);
    }
    touchgfx::Application::getInstance()->registerTimerWidget(this);
}

void SpherePicker::setColumn(int16_t x, int16_t y, int16_t width, int16_t height)
{
    mX     = 0;            // rows are laid out in container-local X
    mWidth = width;
    setPosition(x, y, width, height);
    layout();
}

void SpherePicker::setRange(int16_t maxValue, int16_t step)
{
    mMax   = maxValue;
    mStep  = (step < 1) ? 1 : step;
    mCount = static_cast<int16_t>(maxValue / mStep) + 1;
    layout();
}

int16_t SpherePicker::centerLineY() const
{
    return static_cast<int16_t>(getHeight() / 2);
}

void SpherePicker::setValue(int16_t value)
{
    if (mCount <= 0) {
        return;
    }
    int16_t idx = static_cast<int16_t>(lroundf(static_cast<float>(value) / mStep));
    if (idx < 0)        idx = 0;
    if (idx >= mCount)  idx = mCount - 1;
    mScrollPos = static_cast<float>(idx);
    mTargetPos = mScrollPos;
    mAnimating = false;
    layout();
}

int16_t SpherePicker::getValue() const
{
    if (mCount <= 0) {
        return 0;
    }
    int16_t idx = static_cast<int16_t>(lroundf(mTargetPos)) % mCount;
    if (idx < 0) {
        idx += mCount;
    }
    return static_cast<int16_t>(idx * mStep);
}

void SpherePicker::setActive(bool active)
{
    mActive = active;
    layout();
}

void SpherePicker::setAnimationSteps(int16_t steps)
{
    mAnimSteps = (steps < 1) ? 1 : steps;
}

void SpherePicker::selectNext()
{
    if (mCount <= 1) {
        return;
    }
    mTargetPos += 1.0f;   // cyclic: wraps via the modulo in layout()
    startAnimation();
}

void SpherePicker::selectPrev()
{
    if (mCount <= 1) {
        return;
    }
    mTargetPos -= 1.0f;
    startAnimation();
}

void SpherePicker::startAnimation()
{
    mAnimStart = mScrollPos;
    mAnimStep  = 0;
    mAnimating = true;
}

void SpherePicker::handleTickEvent()
{
    if (!mAnimating) {
        return;
    }

    mAnimStep++;
    const float f = static_cast<float>(mAnimStep) / static_cast<float>(mAnimSteps);
    mScrollPos = mAnimStart + (mTargetPos - mAnimStart) * f;

    if (mAnimStep >= mAnimSteps) {
        mScrollPos = mTargetPos;
        mAnimating = false;
    }

    layout();
}

void SpherePicker::layout()
{
    invalidate();   // clear the old row positions

    if (mCount <= 0) {
        for (int16_t s = 0; s < kVisible; s++) {
            mRows[s].setVisible(false);
        }
        return;
    }

    const int16_t baseCenter = static_cast<int16_t>(lroundf(mScrollPos));

    for (int16_t s = 0; s < kVisible; s++) {
        const int16_t offset = s - kVisible / 2;      // -2..+2
        const int16_t virt   = baseCenter + offset;
        const float   d      = static_cast<float>(virt) - mScrollPos;
        const float   absD   = fabsf(d);
        const bool    isCenter = absD < 0.5f;

        // Show the centre and the rows below it only (the design has no rows
        // above the selected value); the inactive column shows just the centre.
        if (d < -0.5f || (!mActive && !isCenter)) {
            mRows[s].setVisible(false);
            continue;
        }

        int16_t idx = virt % mCount;
        if (idx < 0) {
            idx += mCount;
        }
        const int16_t value = static_cast<int16_t>(idx * mStep);

        // Interpolate the row's y between the two bracketing anchors.
        float yOff;
        if (absD < 1.0f) {
            yOff = lerpf(0.0f, kYoffPos1, absD);
        } else if (absD < 2.0f) {
            yOff = lerpf(kYoffPos1, kYoffPos2, absD - 1.0f);
        } else {
            yOff = kYoffPos2;
        }
        const int16_t y = static_cast<int16_t>(centerLineY() + (d < 0.0f ? -yOff : yOff));

        mRows[s].setVisible(true);
        renderRow(s, value, absD, y, isCenter);
    }

    invalidate();   // draw the new row positions
}

void SpherePicker::renderRow(int16_t slot, int16_t value, float absD,
                             int16_t centerY, bool isCenter)
{
    touchgfx::TextAreaWithOneWildcard& row = mRows[slot];

    const touchgfx::colortype color = isCenter
        ? (mActive ? SDK::GUI::Color::TEAL : SDK::GUI::Color::WHITE)
        : SDK::GUI::Color::GRAY;

    touchgfx::Unicode::snprintf(mBuffers[slot], kBufSize, "%02d", value);
    row.setWildcard(mBuffers[slot]);
    row.setTypedText(touchgfx::TypedText(fontFor(absD)));   // centre-aligned digits
    row.setColor(color);

    // Full-column-width box centres the digits horizontally; the baseline is
    // placed so the digit's cap box centres vertically on centerY.
    row.setPosition(mX, 0, mWidth, kSlotBoxH);
    row.resizeHeightToCurrentText();
    row.setWidth(mWidth);

    const touchgfx::Font*      font = row.getTypedText().getFont();
    const touchgfx::GlyphNode* zero = (font != nullptr) ? font->getGlyph('0') : nullptr;
    const int16_t digitH = (zero != nullptr) ? zero->height()
                                             : static_cast<int16_t>(row.getHeight());
    row.setBaselineY(static_cast<int16_t>(centerY + digitH / 2));
}
