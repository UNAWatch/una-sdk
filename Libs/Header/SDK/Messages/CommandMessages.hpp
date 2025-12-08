/**
 * @file CommandMessages.hpp
 * @date 03-12-2024
 * @author Kernel Communication Module
 * @brief Command message definitions (kernel -> application)
 * 
 * Commands are sent from kernel to applications and expect response.
 * All command messages in range 0x3000 - 0x3FFF.
 * 
 * These messages control application lifecycle and state.
 */

#pragma once

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"


namespace SDK
{

// =============================================================================
// Application Lifecycle Commands (kernel -> app)
// =============================================================================

/**
 * @brief Command to run/start application
 * 
 * Sent by kernel to start application execution.
 * Application should initialize and begin operation.
 * Response indicates success/failure of startup.
 */
struct AppRunCommand : public MessageBase {
    /**
     * @brief Construct run command
     */
    AppRunCommand() 
        : MessageBase(MessageType::COMMAND_APP_RUN)
    {
    }
};

/**
 * @brief Command to stop application
 * 
 * Sent by kernel to stop application execution.
 * Application should cleanup resources and terminate gracefully.
 * Response indicates completion of shutdown.
 */
struct AppStopCommand : public MessageBase {
    /**
     * @brief Construct stop command
     */
    AppStopCommand() 
        : MessageBase(MessageType::COMMAND_APP_STOP)
    {
    }
};

// =============================================================================
// Service Notification Commands (kernel -> Service)
// =============================================================================

/**
 * @brief Notify Service that GUI has started
 * 
 * Sent by kernel to Service when GUI part starts running.
 * Service can now communicate with GUI if needed.
 */
struct AppNotifGuiRunCommand : public MessageBase {
    /**
     * @brief Construct GUI run notification
     */
    AppNotifGuiRunCommand() 
        : MessageBase(MessageType::COMMAND_APP_NOTIF_GUI_RUN)
    {
    }
};

/**
 * @brief Notify Service that GUI has stopped
 * 
 * Sent by kernel to Service when GUI part stops.
 * Service should stop sending messages to GUI.
 */
struct AppNotifGuiStopCommand : public MessageBase {
    /**
     * @brief Construct GUI stop notification
     */
    AppNotifGuiStopCommand() 
        : MessageBase(MessageType::COMMAND_APP_NOTIF_GUI_STOP)
    {
    }
};

// =============================================================================
// GUI State Commands (kernel -> GUI)
// =============================================================================

/**
 * @brief Command to suspend GUI
 * 
 * Sent by kernel to GUI when display is off or app goes to background.
 * GUI should stop rendering and reduce activity.
 */
struct AppGuiSuspendCommand : public MessageBase {
    /**
     * @brief Construct GUI suspend command
     */
    AppGuiSuspendCommand() 
        : MessageBase(MessageType::COMMAND_APP_GUI_SUSPEND)
    {
    }
};

/**
 * @brief Command to resume GUI
 * 
 * Sent by kernel to GUI when display turns on or app comes to foreground.
 * GUI should resume rendering and normal operation.
 */
struct AppGuiResumeCommand : public MessageBase {
    /**
     * @brief Construct GUI resume command
     */
    AppGuiResumeCommand() 
        : MessageBase(MessageType::COMMAND_APP_GUI_RESUME)
    {
    }
};


struct AppTerminate : public MessageBase {

    AppTerminate()
    : MessageBase(MessageType::REQUEST_APP_TERMINATE)
    , code(0)
    {}
    int code;
};

struct AppGuiTickCommand : public MessageBase {
    AppGuiTickCommand() : MessageBase(MessageType::EVENT_GUI_FRAME_TICK) {}
};

} // namespace SDK
