/**
 ******************************************************************************
 * @file    ScrollIndicator.cpp
 * @brief   Arc position indicator on the left bezel.
 ******************************************************************************
 */

#include "SDK/GUI/LVGL/ScrollIndicator.hpp"

#include "SDK/GUI/LVGL/Draw.hpp"

namespace SDK::LVGL
{

namespace
{
namespace Color = SDK::GUI::Color;

constexpr int32_t kRailRadius = 112;
constexpr int32_t kRailWidth  = 9;   // the TouchGFX apps draw 8.5
} // namespace

ScrollIndicator::ScrollIndicator(lv_obj_t* parent, const Config& cfg)
    : mCfg(cfg)
{
    mRail      = Draw::arc(parent, kCx, kCy, kRailRadius, kRailWidth,
                           static_cast<int32_t>(cfg.railMin), static_cast<int32_t>(cfg.railMax),
                           Color::GRAY_DARK);
    mHandle    = Draw::arc(parent, kCx, kCy, kRailRadius, kRailWidth, 0, 1, Color::WHITE);
    mHandleOvf = Draw::arc(parent, kCx, kCy, kRailRadius, kRailWidth, 0, 1, Color::WHITE);
    lv_obj_add_flag(mHandleOvf, LV_OBJ_FLAG_HIDDEN);
    update();
}

ScrollIndicator::~ScrollIndicator()
{
    lv_anim_delete(this, nullptr);
}

void ScrollIndicator::setConfig(const Config& cfg)
{
    mCfg = cfg;
    Draw::setArc(mRail, static_cast<int32_t>(cfg.railMin), static_cast<int32_t>(cfg.railMax));
    update();
}

void ScrollIndicator::setCount(uint16_t count)
{
    // A slide in progress would move the handle on from its old endpoints.
    lv_anim_delete(this, nullptr);
    lv_obj_add_flag(mHandleOvf, LV_OBJ_FLAG_HIDDEN);
    mCount = (count == 0) ? 1 : count;
    mPos   = 0;
    update();
}

void ScrollIndicator::setActive(uint16_t index)
{
    lv_anim_delete(this, nullptr);
    lv_obj_add_flag(mHandleOvf, LV_OBJ_FLAG_HIDDEN);
    mPos = (mCount <= 1) ? 0 : (index >= mCount ? mCount - 1 : index);
    update();
}

void ScrollIndicator::animateTo(uint16_t index, uint32_t ms, int direction)
{
    if (mCount <= 1 || index >= mCount || ms == 0) {
        setActive(index);
        return;
    }
    const uint16_t from = mPos;
    if (index == from) {
        return;
    }
    lv_anim_delete(this, nullptr);
    lv_obj_add_flag(mHandleOvf, LV_OBJ_FLAG_HIDDEN);

    // Item 0 sits at the railMax end and later items step towards railMin, so
    // moving forward (to a later item) lowers the angle. A wrap is a forward
    // move that lands on an earlier item, or the reverse: the handle leaves
    // through one end of the rail while its stand-in enters from the other.
    const bool forward = direction > 0 || (direction == 0 && index > from);
    mAnimWrap  = (direction > 0 && index < from) || (direction < 0 && index > from);
    mAnimFrom  = startAngle(from);
    mAnimTo    = startAngle(index);
    if (mAnimWrap) {
        mAnimOutEnd  = forward ? mAnimFrom - mCfg.handleLen : mAnimFrom + mCfg.handleLen;
        mAnimInStart = forward ? mAnimTo + mCfg.handleLen : mAnimTo - mCfg.handleLen;
    }
    mPos = index;

    // Linear, like the TouchGFX indicator; the value is the progress in
    // thousandths so one animation drives both handles.
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, this);
    lv_anim_set_values(&a, 0, 1000);
    lv_anim_set_duration(&a, ms);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_exec_cb(&a, &ScrollIndicator::animExecCb);
    lv_anim_set_completed_cb(&a, &ScrollIndicator::animDoneCb);
    lv_anim_start(&a);
}

void ScrollIndicator::animExecCb(void* var, int32_t value)
{
    auto*       self = static_cast<ScrollIndicator*>(var);
    const float t    = static_cast<float>(value) / 1000.0f;
    if (self->mAnimWrap) {
        self->setClampedArc(self->mHandle, self->mAnimFrom + t * (self->mAnimOutEnd - self->mAnimFrom));
        self->setClampedArc(self->mHandleOvf, self->mAnimInStart + t * (self->mAnimTo - self->mAnimInStart));
    } else {
        self->setHandle(self->mAnimFrom + t * (self->mAnimTo - self->mAnimFrom));
    }
}

void ScrollIndicator::animDoneCb(lv_anim_t* a)
{
    auto* self = static_cast<ScrollIndicator*>(a->var);
    lv_obj_add_flag(self->mHandleOvf, LV_OBJ_FLAG_HIDDEN);
    self->update();
}

float ScrollIndicator::startAngle(uint16_t index) const
{
    // Same mapping as the TouchGFX indicator: item 0 sits at the top of the
    // rail (railMax end) and later items step towards railMin.
    if (mCount <= 1) {
        return (mCfg.railMax + mCfg.railMin - mCfg.handleLen) / 2.0f;
    }
    const float step = (mCfg.railMax - mCfg.railMin - mCfg.handleLen) / static_cast<float>(mCount - 1);
    return (mCfg.railMax - mCfg.handleLen) - step * static_cast<float>(index);
}

void ScrollIndicator::setHandle(float startDeg)
{
    Draw::setArc(mHandle, static_cast<int32_t>(startDeg + 0.5f),
                 static_cast<int32_t>(startDeg + mCfg.handleLen + 0.5f));
}

void ScrollIndicator::setClampedArc(lv_obj_t* arc, float startDeg)
{
    // Cut the handle at the rail ends so it grows out of, or shrinks into, an
    // end rather than floating past it.
    const float a = LV_CLAMP(mCfg.railMin, startDeg, mCfg.railMax);
    const float b = LV_CLAMP(mCfg.railMin, startDeg + mCfg.handleLen, mCfg.railMax);
    if (b - a < 0.5f) {
        lv_obj_add_flag(arc, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    Draw::setArc(arc, static_cast<int32_t>(a + 0.5f), static_cast<int32_t>(b + 0.5f));
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_HIDDEN);
}

void ScrollIndicator::update()
{
    if (mCount <= 1) {
        lv_obj_add_flag(mHandle, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    setHandle(startAngle(mPos));
    lv_obj_remove_flag(mHandle, LV_OBJ_FLAG_HIDDEN);
}

} // namespace SDK::LVGL
