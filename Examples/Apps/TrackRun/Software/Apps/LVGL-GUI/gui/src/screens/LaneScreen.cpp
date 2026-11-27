/**
 ******************************************************************************
 * @file    LaneScreen.cpp
 * @brief   The lane picker (see LaneScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/LaneScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/TrackText.hpp"

LaneScreen::LaneScreen(Model& model)
    : Screen(model)
{
}

LaneScreen::~LaneScreen()
{
    if (mIdle != nullptr) {
        lv_timer_delete(mIdle);
    }
}

void LaneScreen::build()
{
    // "No lane" too: the road.
    mLanes   = std::make_unique<LaneWheel>(mRoot, Settings::kLanes, true);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, "LANE");

    // Where the run is, as the white line under the title.
    char where[24] = "";
    if (mModel.isTrackActive()) {
        TrackText::where(where, sizeof(where), mModel.getTrackData().onTrack);
    } else {
        const Model::PreRun& p = mModel.getPreRun();
        if (p.status == CustomMessage::PreRunTrack::STORED) {
            std::snprintf(where, sizeof(where), "Track %u", static_cast<unsigned>(p.trackNumber));
        } else if (p.status == CustomMessage::PreRunTrack::NEW_TRACK) {
            std::snprintf(where, sizeof(where), "New track");
        }
    }
    mLanes->setStatus(where);
}

void LaneScreen::onShow()
{
    mModel.resetIdleTimer();
    mLanes->select(mModel.currentLane());
    if (mModel.laneFrom() == Model::LaneFrom::Run && mIdle == nullptr) {
        mIdle = lv_timer_create(&LaneScreen::idleCb, kFromRunIdleMs, this);
        lv_timer_set_repeat_count(mIdle, 1);
    }
}

void LaneScreen::idleCb(lv_timer_t* t)
{
    auto* self  = static_cast<LaneScreen*>(lv_timer_get_user_data(t));
    self->mIdle = nullptr;   // one-shot: LVGL deletes the timer after this call
    self->back();
}

void LaneScreen::back()
{
    switch (mModel.laneFrom()) {
        case Model::LaneFrom::Run:   ScreenManager::instance().goTo(ScreenId::Track); break;
        case Model::LaneFrom::Pause: ScreenManager::instance().goTo(ScreenId::TrackAction); break;
        default:                     ScreenManager::instance().goTo(ScreenId::Main); break;
    }
}

void LaneScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    if (mIdle != nullptr) {
        lv_timer_reset(mIdle);
    }
    switch (code) {
        case Btn::L1: mLanes->up(); break;
        case Btn::L2: mLanes->down(); break;
        case Btn::R1:
            if (mLanes->lane() != mModel.currentLane()) {
                mModel.setLane(mLanes->lane());   // in a run, from now
            }
            back();
            break;
        case Btn::R2:
            back();
            break;
        default:
            break;
    }
}
