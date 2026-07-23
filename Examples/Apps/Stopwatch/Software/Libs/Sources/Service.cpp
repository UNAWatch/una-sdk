#include "Service.hpp"

#define LOG_MODULE_PRX      "Service"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

// The service has no periodic work: elapsed time is derived from the kernel
// tick on demand, never accumulated by polling. Blocking here until something
// happens is what lets the chip reach its low power state with the stopwatch
// still running.
static constexpr uint32_t kWaitForever = 0xFFFFFFFF;

Service::Service(SDK::Kernel &kernel)
    : mKernel(kernel)
    , mSender(kernel)
    , mStopwatch()
    , mGuiStarted(false)
{
}

void Service::run()
{
    LOG_INFO("Started\n");

    while (true) {
        SDK::MessageBase *msg;
        if (!mKernel.comm.getMessage(msg, kWaitForever)) {
            continue;
        }

        switch (msg->getType()) {
            case SDK::MessageType::COMMAND_APP_STOP:
                LOG_INFO("Force exit from the application\n");
                // We must release message because this is the last event.
                mKernel.comm.releaseMessage(msg);
                return;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN:
                LOG_INFO("GUI is now running\n");
                mGuiStarted = true;
                publish();
                break;

            case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
                LOG_INFO("GUI has stopped\n");
                mGuiStarted = false;
                // The service earns its resident thread only while the clock is
                // running. A stopped stopwatch -- paused or cleared -- does no
                // work, so once the GUI is gone there is nothing left to keep it
                // alive; its state goes with it.
                if (!mStopwatch.isRunning()) {
                    LOG_INFO("Clock stopped and GUI gone, exiting service\n");
                    mKernel.comm.releaseMessage(msg);
                    return;
                }
                break;

            default:
                handleCommand(msg);
                break;
        }

        mKernel.comm.releaseMessage(msg);
    }
}

void Service::handleCommand(SDK::MessageBase *msg)
{
    const uint32_t now = mKernel.sys.getTimeMs();

    switch (msg->getType()) {
        case CustomMessage::STOPWATCH_START:
            mStopwatch.start(now);
            break;

        case CustomMessage::STOPWATCH_PAUSE:
            mStopwatch.pause(now);
            break;

        case CustomMessage::STOPWATCH_LAP:
            mStopwatch.lap(now);
            break;

        case CustomMessage::STOPWATCH_RESET:
            mStopwatch.reset();
            break;

        case CustomMessage::STOPWATCH_REQUEST:
            // A plain request for the current state; publish() below answers it.
            break;

        default:
            // Not one of ours -- nothing to publish.
            return;
    }

    publish();
}

void Service::publish()
{
    if (!mGuiStarted) {
        return;
    }
    mSender.state(mStopwatch.state());
}
