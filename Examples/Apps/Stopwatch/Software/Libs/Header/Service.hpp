/**
 ******************************************************************************
 * @file    Service.hpp
 * @date    17-07-2026
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Stopwatch service: owns the state, the GUI only renders it.
 ******************************************************************************
 */

#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "SDK/Kernel/Kernel.hpp"

#include "Commands.hpp"
#include "Stopwatch.hpp"

/**
 * @class Service
 * @brief Background half of the app.
 *
 * The kernel does not stop a service when its GUI closes, which is what lets
 * the stopwatch keep counting after the user walks back to the menu. The flip
 * side is that nothing else will ever reclaim the thread, so the service ends
 * itself once the GUI is gone and the clock is not running -- a stopped
 * stopwatch does no work and is not worth a resident thread.
 */
class Service
{
public:
    Service(SDK::Kernel &kernel);

    virtual ~Service() = default;

    void run();

private:
    SDK::Kernel           &mKernel;
    CustomMessage::Sender  mSender;
    Stopwatch::Core        mStopwatch;
    bool                   mGuiStarted;

    /**
     * @brief Apply one app-specific command to the stopwatch and publish it.
     * @param msg Message to handle.
     */
    void handleCommand(SDK::MessageBase *msg);

    void publish();
};

#endif // SERVICE_HPP
