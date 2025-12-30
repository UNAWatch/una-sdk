/**
 * @file   AppCore.cpp
 * @date   30-December-2025
 * @author Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief  Application core implementation
 */

#include "SDK/Simulator/App/AppCore.hpp"

#include <cstring>

#define LOG_MODULE_PRX      "Application"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

namespace App
{

Core::Core()
	: mMessageManager()
	, mBacklight()
	, mBuzzer()
	, mVibro()
    , mKernelMsgDispatcher(mMessageManager,
						   mVibro,
						   mBacklight,
						   mBuzzer)
    , mAppComm(1,
			   2,
			   mMessageManager,
			   mKernelMsgDispatcher)
{
}

void Core::run()
{
    LOG_INFO("Application core is running...\n");


}

} // namespace App

