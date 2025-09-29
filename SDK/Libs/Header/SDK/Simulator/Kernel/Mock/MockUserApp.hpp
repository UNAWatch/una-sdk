/**
 ******************************************************************************
 * @file    MockUserApp.hpp
 * @date    04-02-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Mock for IUserApp interface.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SIMULATOR_KERNEL_USER_APP_HPP
#define __SIMULATOR_KERNEL_USER_APP_HPP

#include <cstdint>
#include <cstdarg>
#include <stdio.h>
#include <windows.h>

#include "SDK/Platform/OS/OS.hpp"
#include "SDK/Interfaces/IUserApp.hpp"
#include "SDK/Interfaces/IGlance.hpp"

#include <platform/hal/simulator/sdl2/HALSDL2.hpp>

namespace Simulator
{

class MockUserApp : public SDK::Interface::IUserApp {

public:

    class FakeMutex : public SDK::Interface::IMutex
    {
    public:
        FakeMutex() = default;

        void lock()    override {}
        void unLock()  override {}
        bool tryLock() override { return true; }
    };

    MockUserApp(bool useMutex = true)
        : mpCallback(nullptr)
        , mState(State::DESTROYED)
        , mMutex()
        , mFakeMutex()
        , mAppMutex(useMutex ? static_cast<SDK::Interface::IMutex*>(&mMutex)
                    : static_cast<SDK::Interface::IMutex*>(&mFakeMutex))
    {}

    virtual ~MockUserApp() = default;

    virtual void registerApp(SDK::Interface::IUserApp::Callback *pCallback) override
    {
        mpCallback = pCallback;
    }

    void registerGlance(SDK::Interface::IGlance* glance) override
    {
        mpGlance = glance;
    }

    struct GlanceArea getGlanceArea() override
    {
        struct GlanceArea area = {240, 240};
        return area;
    }

    LaunchReason getLaunchReason() override
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

    virtual void restartRequest() override
    {
        exit();
    }
    
    virtual void exit(int status = 0) override
    {
        static_cast<touchgfx::HALSDL2 *>(touchgfx::HAL::getInstance())->stopApplication();
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
    
    virtual void log(const char *format, ...) override
    {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
    
    virtual uint32_t getTimeMs() override 
    { 
        return static_cast<uint32_t>(GetTickCount64());
    }
    
    virtual void delay(uint32_t ms) override
    {
        mAppMutex->unLock();
        Sleep(ms);
        mAppMutex->lock();
    }

    virtual void yield() override { }

    virtual void notifySettingsChanged() override
    {
    }

    virtual void notifyActivityEnd() override
    {
    }

    virtual void notifyLapAlert() override
    {
    }

    virtual void enablePhoneNotification(bool enable) override
    {
    }

    virtual void enableMusicControl(bool enable) override
    {
    }

    virtual void enableUsbCharging(bool enable) override
    {
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

    State  getState()
    {
        return mState;
    }

    void create() 
    { 
        if (mState == State::DESTROYED) {
            mState = State::CREATED;
            if (mpCallback) {
                OS::MutexCS cs(*mAppMutex);
                mpCallback->onCreate();
            }
        }
    }


    void start()
    {
        if (mState == State::CREATED) {
            mState = State::STARTED;
            if (mpCallback) {
                OS::MutexCS cs(*mAppMutex);
                mpCallback->onStart();
            }
        }
    }


    void resume()
    {
        if (mState == State::STARTED) {
            mState = State::RESUMED;
            if (mpCallback) {
                OS::MutexCS cs(*mAppMutex);
                mpCallback->onResume();
            }
        }
    }


    void pause()
    {
        if (mState == State::RESUMED) {
            mState = State::STARTED;
            if (mpCallback) {
                OS::MutexCS cs(*mAppMutex);
                mpCallback->onPause();
            }
        }
    }

    void stop()
    {
        if (mState == State::STARTED) {
            mState = State::CREATED;
            if (mpCallback) {
                OS::MutexCS cs(*mAppMutex);
                mpCallback->onStop();
            }
        }
    }

    void destroy()
    {
        if (mState == State::CREATED) {
            mState = State::DESTROYED;
            OS::MutexCS cs(*mAppMutex);
            mpCallback->onDestroy();
        }
    }



private:
    SDK::Interface::IUserApp::Callback* mpCallback;
	SDK::Interface::IGlance*            mpGlance;
    State                               mState;
    OS::Mutex                           mMutex;
    FakeMutex                           mFakeMutex;
    SDK::Interface::IMutex*             mAppMutex;

};

} /* namespace Simulator */

#endif /* __SIMULATOR_KERNEL_USER_APP_HPP */
