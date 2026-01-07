/**
 * @file   AppCore.cpp
 * @date   30-December-2025
 * @author Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief  Application core implementation
 */

#include "SDK/Simulator/App/AppCore.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include <gui/common/FrontendApplication.hpp>

#include <cstring>

#define LOG_MODULE_PRX      "App.Core"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

namespace App {

	Core::Core(SDK::App::Comm&         appComm,
			   SDK::Simulator::Kernel& srvKernel,
			   SDK::Simulator::Kernel& guiKernel)
		: mAppComm(appComm)
		, mSrvKernel(srvKernel)
		, mGuiKernel(guiKernel)
		, mStopRequested(false)
		, mMutex()
	{
	}

	void Core::stopRequest()
	{
		OS::MutexCS lock(mMutex);

		mStopRequested = true;
	}
	
	void Core::run()
	{
		LOG_INFO("Application core is running...\n");

		mAppComm.sendToService(SDK::make_msg(mSrvKernel.getKernel(), SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN).release());

		while (true) {
			OS::Delay(100);

			if (isStopRequest()) {
				LOG_INFO("Stop requested, exiting application core loop...\n");
				FrontendApplication& app = *static_cast<FrontendApplication*>(touchgfx::Application::getInstance());

				mAppComm.sendToGui(SDK::make_msg(mSrvKernel.getKernel(), SDK::MessageType::COMMAND_APP_GUI_SUSPEND).release());
				
				app.handleTickEvent();
				
				mAppComm.sendToService(SDK::make_msg(mSrvKernel.getKernel(), SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP).release());
				mAppComm.sendToGui(SDK::make_msg(mSrvKernel.getKernel(), SDK::MessageType::COMMAND_APP_STOP).release());
				
				app.handleTickEvent();
				
				mAppComm.sendToService(SDK::make_msg(mSrvKernel.getKernel(), SDK::MessageType::COMMAND_APP_STOP).release());
				break;
			}
		}
	}

	bool Core::isStopRequest()
	{
		OS::MutexCS lock(mMutex);

		return mStopRequested;
	}

} // namespace App
