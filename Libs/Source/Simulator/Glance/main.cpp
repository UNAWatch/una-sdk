#include <atomic>
#include <charconv>
#include <cstdio>
#include <string>
#include <thread>

#include <SDL.h>
#include "Service.hpp"
#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Simulator/App/AppMessageCore.hpp"
#include "SDK/Simulator/App/KernelMessageDispatcher.hpp"
#include "SDK/Simulator/Glance/GlanceHost.hpp"
#include "SDK/Simulator/Kernel/Kernel.hpp"
#include "SDK/Simulator/Kernel/Mock/Backlight.hpp"
#include "SDK/Simulator/Kernel/Mock/Buzzer.hpp"
#include "SDK/Simulator/Kernel/Mock/Vibro.hpp"
#include "SDK/UnaLogger/Logger.h"

/**
 * @brief Parse host options and run the real glance service against the mock kernel.
 * @param argc Number of command-line arguments, including the executable name.
 * @param argv Options for duration, screenshot output and window scale.
 * @return Zero for help or a successful run with updates, one for initialization,
 * capture or stop failure (or no updates), and two for invalid arguments.
 * @details Joins the service and dispatcher before stopping background sensor threads.
 */
int main(int argc, char** argv)
{
    SDK::Simulator::GlanceHost::Options options;
    options.title = APP_NAME " (Glance)";
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--help") {
            std::puts("Options: --duration-ms N --screenshot FILE.bmp --scale N. Esc closes the preview.");
            return 0;
        }
        if (index + 1 >= argc) return 2;
        const std::string value = argv[++index];
        if (argument == "--screenshot") options.screenshot = value;
        else if (argument == "--duration-ms" || argument == "--scale") {
            uint32_t number = 0;
            const auto result = std::from_chars(value.data(), value.data() + value.size(), number);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || number == 0) return 2;
            if (argument == "--duration-ms") options.durationMs = number;
            else {
                if (number > 10) return 2;
                options.scale = static_cast<int>(number);
            }
        } else return 2;
    }

    SDK::Simulator::Mock::SystemService system;
    SDK::Simulator::Kernel kernel("service");
    SDK::App::MessageCore core;
    auto& comm = core.getAppComm();
    kernel.setIAppComm(comm.getServiceComm());
    kernel.setISystem(&system);
    SDK::Simulator::KernelHolder::Create(kernel);
    Logger_init(kernel.getKernel().log);
    SDK::KernelProviderService::CreateInstance(&kernel.getKernel());

    Service service(kernel.getKernel());
    SDK::Simulator::Mock::Backlight backlight;
    SDK::Simulator::Mock::Buzzer buzzer;
    SDK::Simulator::Mock::Vibro vibro;
    SDK::App::KernelMessageDispatcher dispatcher(comm, comm.getMsgManager(), vibro, backlight, buzzer);
    SDK::Simulator::GlanceHost host(comm, kernel.getKernel(), options);
    if (!host.init()) {
        std::fprintf(stderr, "Could not initialize glance simulator: %s\n", SDL_GetError());
        return 1;
    }
    dispatcher.setGlanceUpdateHandler([&host](const auto& update) { return host.capture(update); });
    if (!host.start()) return 1;
    std::atomic<bool> serviceRunning{true};
    std::thread dispatcherThread([&dispatcher] { dispatcher.run(); });
    std::thread serviceThread([&] {
        service.run();
        serviceRunning = false;
    });

    const bool success = host.run([&] { return serviceRunning.load(); });
    serviceThread.join();
    dispatcher.stop();
    dispatcherThread.join();
    SDK::Simulator::Mock::SystemGUI shutdown;
    shutdown.exit(0);
    std::printf("Glance simulator finished: %u updates captured.\n", host.updateCount());
    return success && host.updateCount() > 0 ? 0 : 1;
}
