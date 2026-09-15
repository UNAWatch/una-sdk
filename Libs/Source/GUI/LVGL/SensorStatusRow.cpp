/**
 ******************************************************************************
 * @file    SensorStatusRow.cpp
 * @brief   Centred row of sensor icons; a searching icon blinks.
 ******************************************************************************
 */

#include "SDK/GUI/LVGL/SensorStatusRow.hpp"

#include "SDK/GUI/LVGL/Draw.hpp"

namespace SDK::LVGL
{

namespace Color = SDK::GUI::Color;

namespace
{
constexpr int32_t  kIconGap  = 24;
constexpr uint32_t kBlinkMs  = 500;
} // namespace

SensorStatusRow::SensorStatusRow(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h,
                                 const lv_image_dsc_t* gpsIcon, const lv_image_dsc_t* hrIcon)
    : mGpsIcon(gpsIcon), mHrIcon(hrIcon), mW(w), mH(h)
{
    // One alpha-only shape per sensor; the dark/light phases are tints.
    mRow = Draw::container(parent, x, y, w, h);
    lv_obj_add_event_cb(mRow, &SensorStatusRow::deleteCb, LV_EVENT_DELETE, this);
    if (mGpsIcon) {
        mGps = Draw::imageTinted(mRow, mGpsIcon, 0, 0, Color::WHITE);
    }
    if (mHrIcon) {
        mHr = Draw::imageTinted(mRow, mHrIcon, 0, 0, Color::WHITE);
    }
    update();
}

SensorStatusRow::~SensorStatusRow()
{
    if (mTimer) {
        lv_timer_delete(mTimer);
    }
    if (mRow) {
        // The objects outlive this widget; they must not call back into it.
        lv_obj_remove_event_cb_with_user_data(mRow, &SensorStatusRow::deleteCb, this);
    }
}

void SensorStatusRow::deleteCb(lv_event_t* e)
{
    // The parent went first: stop the blink and forget the objects, so the
    // widget's remaining calls do nothing.
    auto* self = static_cast<SensorStatusRow*>(lv_event_get_user_data(e));
    if (self->mTimer) {
        lv_timer_delete(self->mTimer);
        self->mTimer = nullptr;
    }
    self->mRow = self->mGps = self->mHr = nullptr;
}

void SensorStatusRow::setGps(State s)
{
    if (!mRow) {
        return;
    }
    if (!mGps) {
        s = State::Absent;
    }
    if (s != mGpsState) {
        mGpsState = s;
        update();
    }
}

void SensorStatusRow::setHr(State s)
{
    if (!mRow) {
        return;
    }
    if (!mHr) {
        s = State::Absent;
    }
    if (s != mHrState) {
        mHrState = s;
        update();
    }
}

void SensorStatusRow::tickCb(lv_timer_t* t)
{
    auto* self = static_cast<SensorStatusRow*>(lv_timer_get_user_data(t));
    self->mLight = !self->mLight;
    self->applyIcons();
}

void SensorStatusRow::update()
{
    layout();
    const bool animating = mGpsState == State::Searching || mHrState == State::Searching;
    if (animating && !mTimer) {
        mLight = true;
        mTimer = lv_timer_create(&SensorStatusRow::tickCb, kBlinkMs, this);
    } else if (!animating && mTimer) {
        lv_timer_delete(mTimer);
        mTimer = nullptr;
    }
    applyIcons();
}

void SensorStatusRow::layout()
{
    const bool gpsOn = mGpsState != State::Absent;
    const bool hrOn  = mHrState != State::Absent;
    const int32_t gw = gpsOn ? mGpsIcon->header.w : 0, gh = gpsOn ? mGpsIcon->header.h : 0;
    const int32_t hw = hrOn ? mHrIcon->header.w : 0,   hh = hrOn ? mHrIcon->header.h : 0;

    // Icons are bottom-aligned; centred as a pair, or singly.
    if (gpsOn && hrOn) {
        const int32_t x = (mW - (gw + kIconGap + hw)) / 2;
        lv_obj_set_pos(mGps, x, mH - gh);
        lv_obj_set_pos(mHr, x + gw + kIconGap, mH - hh);
    } else if (gpsOn) {
        lv_obj_set_pos(mGps, (mW - gw) / 2, mH - gh);
    } else if (hrOn) {
        lv_obj_set_pos(mHr, (mW - hw) / 2, mH - hh);
    }
}

void SensorStatusRow::applyIcons()
{
    auto applyOne = [this](lv_obj_t* img, State s) {
        if (!img) {
            return;
        }
        if (s == State::Absent) {
            lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
            return;
        }
        lv_obj_remove_flag(img, LV_OBJ_FLAG_HIDDEN);
        Draw::tint(img, (s == State::Connected || mLight) ? Color::WHITE : Color::GRAY_DARK);
    };
    applyOne(mGps, mGpsState);
    applyOne(mHr,  mHrState);
}

} // namespace SDK::LVGL
