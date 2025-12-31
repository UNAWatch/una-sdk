/**
 * @file   AppCore.cpp
 * @date   30-December-2025
 * @author Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief  Application core implementation
 */

#include "SDK/Simulator/App/AppMessageCore.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include <cstring>

#define LOG_MODULE_PRX      "App.MessageCore"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

namespace SDK::App
{

	MessageCore::MessageCore()
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
	{}

	SDK::App::Comm& MessageCore::getAppComm()
	{
		return mAppComm;
	}

} // namespace App
