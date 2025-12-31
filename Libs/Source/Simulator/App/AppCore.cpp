/**
 * @file   AppCore.cpp
 * @date   30-December-2025
 * @author Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief  Application core implementation
 */

#include "SDK/Simulator/App/AppCore.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Messages/MessageTypes.hpp"

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
	{
	}

	void Core::run()
	{
		LOG_INFO("Application core is running...\n");

		mAppComm.sendToService(SDK::make_msg(mSrvKernel.getKernel(), SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN).release());
	}

} // namespace App
