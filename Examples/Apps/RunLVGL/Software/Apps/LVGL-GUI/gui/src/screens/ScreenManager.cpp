/**
 ******************************************************************************
 * @file    ScreenManager.cpp
 * @brief   Owns the active screen and switches between them.
 ******************************************************************************
 */

#include "gui/screens/ScreenManager.hpp"

#include "lvgl.h"

#define LOG_MODULE_PRX      "ScreenManager"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "gui/model/Model.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/screens/MainScreen.hpp"
#include "gui/screens/TrackStartConfirmScreen.hpp"
#include "gui/screens/TrackScreen.hpp"
#include "gui/screens/TrackActionScreen.hpp"
#include "gui/screens/TrackHoldConfirmScreen.hpp"
#include "gui/screens/TrackLapScreen.hpp"
#include "gui/screens/TrackResultScreen.hpp"
#include "gui/screens/TrackSummaryScreen.hpp"

ScreenManager& ScreenManager::instance()
{
    static ScreenManager sInstance;
    return sInstance;
}

void ScreenManager::start(Model& model, ScreenId first)
{
    mModel = &model;
    switchNow(first);
}

void ScreenManager::goTo(ScreenId id)
{
    mPending = id;
    if (!mPendingSet) {
        mPendingSet = true;
        lv_async_call(&ScreenManager::asyncCb, this);
    }
}

void ScreenManager::asyncCb(void* user)
{
    auto* self = static_cast<ScreenManager*>(user);
    self->mPendingSet = false;
    self->switchNow(self->mPending);
}

void ScreenManager::switchNow(ScreenId id)
{
    Screen* next = create(id);
    if (!next) {
        LOG_WARNING("No screen for id %u\n", static_cast<unsigned>(id));
        return;
    }

    Screen* old = mCurrent;
    if (old) {
        old->onHide();
        mModel->bind(nullptr);
    }

    // Build and show the new screen, then free the old one: the same order
    // as the TouchGFX MVP application, with the memory of only one screen's
    // widgets held while both exist.
    next->create();
    lv_screen_load(next->root());
    mCurrent = next;

    if (old) {
        old->destroy();
        delete old;
    }

    mModel->bind(next);
    next->onShow();
}

Screen* ScreenManager::create(ScreenId id)
{
    Model& m = *mModel;
    switch (id) {
        case ScreenId::Main:              return new MainScreen(m);
        case ScreenId::TrackStartConfirm: return new TrackStartConfirmScreen(m);
        case ScreenId::Track:             return new TrackScreen(m);
        case ScreenId::TrackAction:       return new TrackActionScreen(m);
        case ScreenId::TrackHoldConfirm:  return new TrackHoldConfirmScreen(m);
        case ScreenId::TrackLap:          return new TrackLapScreen(m);
        case ScreenId::TrackSaved:        return new TrackResultScreen(m, TrackResultScreen::Result::Saved);
        case ScreenId::TrackDiscarded:    return new TrackResultScreen(m, TrackResultScreen::Result::Discarded);
        case ScreenId::TrackSummary:      return new TrackSummaryScreen(m);
    }
    return nullptr;
}
