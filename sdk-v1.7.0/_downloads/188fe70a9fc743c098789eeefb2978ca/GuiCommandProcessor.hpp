/**
 ******************************************************************************
 * @file    GuiCommandProcessor.hpp
 * @brief   The GUI process's kernel message pump, shared by every GUI toolkit.
 *
 * A GUI process talks to the kernel through one message queue. This singleton
 * drains it: lifecycle commands (start, resume, suspend, stop) become
 * IGuiLifeCycleCallback calls, EVENT_GUI_TICK paces the frame loop, button
 * events are translated to SDK::GUI::Button codes and queued for the toolkit,
 * app-private messages from the service are queued for the
 * ICustomMessageHandler, and finished frames are sent back with
 * REQUEST_DISPLAY_UPDATE.
 *
 * Nothing here depends on a toolkit: the TouchGFX port drives it from its HAL
 * (see TouchGFXHAL and OSWrappers) and the LVGL port from SDK::LVGL::Port.
 * The class was called TouchGFXCommandProcessor until the LVGL port arrived;
 * that name remains available as an alias in
 * SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp.
 ******************************************************************************
 */

#ifndef SDK_PORT_GUI_COMMAND_PROCESSOR_HPP
#define SDK_PORT_GUI_COMMAND_PROCESSOR_HPP

#include <cstdint>
#include <cstddef>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Interfaces/ICustomMessageHandler.hpp"
#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Tools/FixedQueue.hpp"

namespace SDK
{

/**
 * @brief Kernel message pump and lifecycle hub of a GUI process.
 */
class GuiCommandProcessor
{
public:
    static GuiCommandProcessor& GetInstance()
    {
        static GuiCommandProcessor sInstance;
        return sInstance;
    }

    /// The object told about start/stop/resume/suspend and each frame tick.
    void setAppLifeCycleCallback(SDK::Interface::IGuiLifeCycleCallback* cb) { mAppLifeCycleCallback = cb; }

    /// The object given the app-private (service -> GUI) messages.
    void setCustomMessageHandler(SDK::Interface::ICustomMessageHandler* h) { mCustomMessageHandler = h; }

    /**
     * @brief   Process kernel messages until the next frame tick.
     *
     * Blocks on the GUI's queue, handling lifecycle commands, button events
     * and queueing app-private messages on the way. Returns false when the
     * toolkit may render a frame (an EVENT_GUI_TICK arrived, or in the
     * simulator build the 50 ms poll ran out), and true when the kernel has
     * stopped the process: onStop() has run and sys.exit() was called.
     */
    bool waitForFrameTick();

    /**
     * @brief   Whether an EVENT_GUI_TICK arrived since the last call.
     *
     * On the watch waitForFrameTick() only returns on a tick, so this is
     * always true after it. The simulator build also returns from
     * waitForFrameTick() on a poll timeout; a toolkit that renders one frame
     * per kernel tick uses this to tell the two apart.
     */
    bool consumeFrameTick();

    /// frameNumber carried by the most recent EVENT_GUI_TICK (the kernel's
    /// running tick count), for correlating app-side and kernel-side timing logs.
    uint32_t lastFrameNumber() const { return mLastFrameNumber; }

    /// Pop the next queued SDK::GUI::Button code; false when there is none.
    bool getKeySample(uint8_t &key);

    /// Send a finished frame (one ABGR2222 byte per pixel) to the kernel.
    /// Ignored while the GUI is suspended.
    void writeDisplayFrameBuffer(const uint8_t* data);

    /// Deliver the queued app-private messages to the custom message handler.
    /// Call between frames, never from inside waitForFrameTick().
    void callCustomMessageHandler();

private:
    GuiCommandProcessor();
    virtual ~GuiCommandProcessor();

    GuiCommandProcessor(const GuiCommandProcessor&) = delete;
    GuiCommandProcessor& operator=(const GuiCommandProcessor&) = delete;

    void handleEvent(SDK::Message::EventButton* msg);

    const SDK::Kernel&                            mKernel;
    bool                                          mStartCallbackCalled;
    bool                                          mIsGuiResumed;
    bool                                          mFrameTickPending = false;
    uint32_t                                      mLastFrameNumber  = 0;
    SDK::Interface::IGuiLifeCycleCallback*        mAppLifeCycleCallback;
    SDK::Interface::ICustomMessageHandler*        mCustomMessageHandler;
    SDK::Tools::FixedQueue<SDK::MessageBase*, 10> mUserQueue {};
    // Button codes are queued, not latched: a single physical press expands to
    // press/click/release, several of which can arrive between two GUI ticks.
    SDK::Tools::FixedQueue<uint8_t, 16>           mButtonCodes {};
};

} // namespace SDK

#endif // SDK_PORT_GUI_COMMAND_PROCESSOR_HPP
