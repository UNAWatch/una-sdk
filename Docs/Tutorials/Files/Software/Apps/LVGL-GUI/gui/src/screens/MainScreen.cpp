/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   Files' one screen: three settings in a column, one selected.
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"

#include <cstdio>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "gui/Assets.hpp"

namespace Draw = SDK::LVGL::Draw;
namespace Color = SDK::GUI::Color;

namespace
{
constexpr int32_t kRowY[] = { 36, 99, 161 };   // the TouchGFX design's rows

const char* activityName(CustomMessage::ActivityType type)
{
    switch (type) {
        case CustomMessage::ActivityType::RUNNING:  return "RUNNING";
        case CustomMessage::ActivityType::CYCLING:  return "CYCLING";
        case CustomMessage::ActivityType::SWIMMING: return "SWIMMING";
        case CustomMessage::ActivityType::WALKING:  return "WALKING";
        default:                                    return "*";
    }
}

const char* displayName(CustomMessage::DisplayMode mode)
{
    switch (mode) {
        case CustomMessage::DisplayMode::SIMPLE:   return "SIMPLE";
        case CustomMessage::DisplayMode::DETAILED: return "DETAILED";
        case CustomMessage::DisplayMode::COMPACT:  return "COMPACT";
        default:                                   return "-";
    }
}
} // namespace

MainScreen::MainScreen(Model& model)
    : mModel(model)
{
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &MainScreen::keyEventCb, LV_EVENT_KEY, this);

    // Each row: a dash marker at x = 40 and the value beside it at x = 74. The
    // value box runs to the right edge, left-aligned, so a long name is not cut.
    for (int i = 0; i < SETTING_COUNT; ++i) {
        Draw::label(mRoot, &poppins_semibold_35, "-", 40, kRowY[i], 20, LV_TEXT_ALIGN_LEFT);
        mValue[i] = Draw::label(mRoot, &poppins_medium_25, "", 74, kRowY[i], 160, LV_TEXT_ALIGN_LEFT);
    }

    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::WHITE, SDK::LVGL::Buttons::WHITE,
                  SDK::LVGL::Buttons::AMBER, SDK::LVGL::Buttons::WHITE);

    bind(&mModel);
    mModel.bind(this);

    // Show the defaults now; the service's answer replaces them.
    updateSettingsDisplay();
    mModel.requestSettings();
}

MainScreen::~MainScreen()
{
    mModel.bind(nullptr);
    lv_obj_delete(mRoot);
}

void MainScreen::keyEventCb(lv_event_t* e)
{
    auto* self = static_cast<MainScreen*>(lv_event_get_user_data(e));
    self->onKey(static_cast<uint8_t>(lv_event_get_key(e)));
}

void MainScreen::onSettingsUpdate(float decimalCounter, CustomMessage::ActivityType activityType,
                                  CustomMessage::DisplayMode displayMode)
{
    mDecimalCounter = static_cast<int32_t>(decimalCounter);
    mActivityType   = activityType;
    mDisplayMode    = displayMode;
    updateSettingsDisplay();
}

void MainScreen::changeSelected(int direction)
{
    switch (mSelected) {
        case STG1:
            mDecimalCounter += direction;
            break;
        case STG2: {
            const int n = CustomMessage::ActivityType::_MAX_ACTIVITY_TYPE;
            mActivityType = static_cast<CustomMessage::ActivityType>((mActivityType + direction + n) % n);
            break;
        }
        case STG3: {
            const int n = CustomMessage::DisplayMode::_MAX_DISPLAY_MODE;
            mDisplayMode = static_cast<CustomMessage::DisplayMode>((mDisplayMode + direction + n) % n);
            break;
        }
        default:
            break;
    }
}

void MainScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1:
            changeSelected(+1);
            break;
        case Btn::L2:
            changeSelected(-1);
            break;
        case Btn::R1:
            // Save, then move the selection to the next setting.
            mModel.updateSettings(static_cast<float>(mDecimalCounter), mActivityType, mDisplayMode);
            mSelected = static_cast<Setting>((mSelected + 1) % SETTING_COUNT);
            break;
        case Btn::R2:
            mModel.updateSettings(static_cast<float>(mDecimalCounter), mActivityType, mDisplayMode);
            mModel.exitApp();
            return;
        default:
            return;   // press and release codes
    }
    updateSettingsDisplay();
}

void MainScreen::updateSettingsDisplay()
{
    char counter[16];
    snprintf(counter, sizeof(counter), "%ld", static_cast<long>(mDecimalCounter));
    lv_label_set_text(mValue[STG1], counter);
    lv_label_set_text(mValue[STG2], activityName(mActivityType));
    lv_label_set_text(mValue[STG3], displayName(mDisplayMode));

    // The selected setting is red, the others white.
    for (int i = 0; i < SETTING_COUNT; ++i) {
        lv_obj_set_style_text_color(mValue[i], Draw::rgb(i == mSelected ? Color::RED : Color::WHITE), LV_PART_MAIN);
    }
}
