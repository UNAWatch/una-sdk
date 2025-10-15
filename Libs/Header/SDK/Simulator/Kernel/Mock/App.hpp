/**
 ******************************************************************************
 * @file    App.hpp
 * @date    04-02-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Mock for IUserApp interface.
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include <cstdint>
#include <cstdarg>
#include <stdio.h>
#include <windows.h>

#include "SDK/Platform/OS/OS.hpp"
#include "SDK/Interfaces/IApp.hpp"
#include "SDK/Interfaces/IGlance.hpp"


#include "touchgfx/Utils.hpp"

namespace SDK::Simulator::Mock
{

class App : public SDK::Interface::IApp {

public:

    class FakeMutex : public SDK::Interface::IMutex
    {
    public:
        FakeMutex() = default;

        void lock()    override {}
        void unLock()  override {}
        bool tryLock() override { return true; }
    };

    App(OS::Mutex* appMutex)
        : mpCallback(nullptr)
        , mpGlance(nullptr)
        , mState(State::DESTROYED)
        , mFakeMutex()
        , mAppMutex(appMutex ? static_cast<SDK::Interface::IMutex*>(appMutex)
                    : static_cast<SDK::Interface::IMutex*>(&mFakeMutex))
    {}

    virtual ~App() = default;

    virtual void registerApp(SDK::Interface::IApp::Callback *pCallback) override
    {
        mpCallback = pCallback;
    }

    virtual void registerGlance(SDK::Interface::IGlance* glance) override
    {
        mpGlance = glance;
    }

    virtual void getGlanceArea(int16_t& width, int16_t& height) override
    {
        width = 240;
        height = 60;
    }

    virtual LaunchReason getLaunchReason() override
    {
        return LaunchReason::AUTO_START;
    }

    virtual void initialized() override
    {
        mAppMutex->lock();
    }

    virtual void waitForFrame() override { }
    virtual void pauseRequest() override 
    { 
        pause();
    }

    virtual void resumeRequest() override 
    {
        resume();
    }


    virtual void getDisplayResolution(int16_t &width, int16_t &height) override
    {
        width = 240; 
        height = 240;
    }
    
    virtual uint8_t getDisplayColorDepth() override
    {
        return 6;
    }

    virtual void writeFrameBuffer(const uint8_t *pBuf) override { }
    virtual bool keySample(uint8_t &key) override
    {
        return false;
    }
    


    virtual void lock() override
    {
        mAppMutex->lock();
    }

    virtual void unLock() override
    {
        mAppMutex->unLock();
    }


    enum class State {
        DESTROYED,
        CREATED,
        STARTED,
        RESUMED,
    };

    virtual State  getState()
    {
        return mState;
    }

    virtual void create()
    { 
        if (mState == State::DESTROYED) {
            mState = State::CREATED;
            if (mpCallback) {
                OS::MutexCS cs(*mAppMutex);
                mpCallback->onCreate();
            }
        }
    }


    virtual void start()
    {
        if (mState == State::CREATED) {
            mState = State::STARTED;
            if (mpCallback) {
                OS::MutexCS cs(*mAppMutex);
                mpCallback->onStart();
            }
        }
    }


    virtual void resume()
    {
        if (mState == State::STARTED) {
            mState = State::RESUMED;
            if (mpCallback) {
                OS::MutexCS cs(*mAppMutex);
                mpCallback->onResume();
            }
        }
    }


    virtual void pause()
    {
        if (mState == State::RESUMED) {
            mState = State::STARTED;
            if (mpCallback) {
                OS::MutexCS cs(*mAppMutex);
                mpCallback->onPause();
            }
        }
    }

    virtual void stop()
    {
        if (mState == State::STARTED) {
            mState = State::CREATED;
            if (mpCallback) {
                OS::MutexCS cs(*mAppMutex);
                mpCallback->onStop();
            }
        }
    }

    virtual void destroy()
    {
        if (mState == State::CREATED) {
            mState = State::DESTROYED;
            if (mpCallback) {
                OS::MutexCS cs(*mAppMutex);
                mpCallback->onDestroy();
            }
        }
    }

    virtual void guiState(bool isRun)
    {
        if (mState >= State::STARTED) {
            if (mpCallback) {
                OS::MutexCS cs(*mAppMutex);
                if (isRun) {
                    mpCallback->onStartGUI();
                } else {
                    mpCallback->onStopGUI();
                }
            }
        }
    }

private:
    SDK::Interface::IApp::Callback* mpCallback;
	SDK::Interface::IGlance*        mpGlance;
    State                           mState;
    FakeMutex                       mFakeMutex;
    SDK::Interface::IMutex*         mAppMutex;
    
};

} // namespace SDK::Simulator::Mock
