/**
 ******************************************************************************
 * @file    LaneWheel.cpp
 * @brief   Track Run's lane picker (see LaneWheel.hpp).
 ******************************************************************************
 */

#include "gui/widgets/LaneWheel.hpp"

#include <cstdio>
#include <cstring>

#include "gui/theme/Theme.hpp"

namespace
{

// The SDK wheel's lens: 66 px high at y = 87 (SDK WheelMenu.cpp).
constexpr int32_t kLensY   = 87;
constexpr int32_t kLensH   = 66;
constexpr int32_t kBadgeW  = 36;   // Kit::LaneBadge, large
constexpr int32_t kBadgeH  = 42;
constexpr int32_t kBadgeGap = 10;

// RunLVGL's info line under the title (its workout list's group).
constexpr int32_t kStatusX = 35;
constexpr int32_t kStatusY = 57;
constexpr int32_t kStatusW = 170;

/// A 1 x 1 transparent image: the selected lane is an Icon item, so the SDK
/// wheel sets its text left-aligned beside the icon's place, where the badge
/// is drawn.
const lv_image_dsc_t* blankImage()
{
    static const uint8_t  kPixel[1] = { 0 };
    static lv_image_dsc_t dsc       = {};
    if (dsc.data == nullptr) {
        dsc.header.magic  = LV_IMAGE_HEADER_MAGIC;
        dsc.header.cf     = LV_COLOR_FORMAT_A8;
        dsc.header.w      = 1;
        dsc.header.h      = 1;
        dsc.header.stride = 1;
        dsc.data_size     = sizeof(kPixel);
        dsc.data          = kPixel;
    }
    return &dsc;
}

} // namespace

LaneWheel::LaneWheel(lv_obj_t* parent, uint8_t count, bool noLane)
    : mCount(count >= 1 && count <= kLanes ? count : kLanes), mNoLane(noLane)
{
    // The badge and the text as one group centred on the lens, measured on
    // the widest lane's text.
    const lv_font_t* font = Theme::font(Theme::Font::SemiBold30);
    int32_t textW = 0;
    for (uint8_t i = 0; i < mCount; ++i) {
        // Lanes are single digits (1 to 8).
        std::snprintf(mTexts[i], sizeof(mTexts[i]), "Lane %c", static_cast<char>('1' + i));
        lv_point_t size;
        lv_text_get_size(&size, mTexts[i], font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        if (size.x > textW) {
            textW = size.x;
        }
    }
    const int32_t groupX = 120 - (kBadgeW + kBadgeGap + textW) / 2;
    const int32_t textX  = groupX + kBadgeW + kBadgeGap;

    // "No lane" first: plain centred text, with no badge.
    const uint8_t first = mNoLane ? 1 : 0;
    if (mNoLane) {
        mItems[0].style = WheelMenu::Item::Style::Simple;
        mItems[0].text  = "No lane";
    }
    for (uint8_t i = 0; i < mCount; ++i) {
        WheelMenu::Item& it = mItems[first + i];
        it.style        = WheelMenu::Item::Style::Icon;
        it.text         = mTexts[i];
        it.centerIcon   = blankImage();
        it.centerLayout = { 0, 0, static_cast<int16_t>(textX), static_cast<int16_t>(240 - textX) };
        it.icon         = nullptr;   // the lanes around it: centred text
    }
    mMenu = std::make_unique<WheelMenu>(parent, mItems, static_cast<uint16_t>(first + mCount));
    mMenu->setCircular(false);
    mMenu->setSlideMidCallback(&LaneWheel::slideMid, this);

    mBadge  = std::make_unique<Kit::LaneBadge>(parent, groupX, kLensY + (kLensH - kBadgeH) / 2, true);
    mStatus = Theme::label(parent, Theme::Font::Italic18, "", kStatusX, kStatusY, kStatusW);
    Theme::setHidden(mStatus, true);
    showBadge(1);
}

void LaneWheel::showBadge(uint8_t lane)
{
    // No badge on "No lane".
    Theme::setHidden(mBadge->obj(), lane == 0);
    if (lane != 0) {
        mBadge->set(lane);
    }
}

void LaneWheel::slideMid(void* ctx, uint16_t target)
{
    // The badge changes as the incoming item takes the lens.
    auto* self = static_cast<LaneWheel*>(ctx);
    self->showBadge(static_cast<uint8_t>(self->mNoLane ? target : target + 1));
}

void LaneWheel::select(uint8_t lane)
{
    if ((lane == 0 && !mNoLane) || lane > mCount) {
        lane = 1;
    }
    mMenu->select(static_cast<uint16_t>(mNoLane ? lane : lane - 1));
    showBadge(lane);
}

uint8_t LaneWheel::lane() const
{
    return static_cast<uint8_t>(mNoLane ? mMenu->selected() : mMenu->selected() + 1);
}

void LaneWheel::up()
{
    mMenu->prev();
}

void LaneWheel::down()
{
    mMenu->next();
}

void LaneWheel::setStatus(const char* text)
{
    const bool show = text != nullptr && text[0] != '\0';
    if (show && std::strcmp(lv_label_get_text(mStatus), text) != 0) {
        lv_label_set_text(mStatus, text);
    }
    Theme::setHidden(mStatus, !show);
}
