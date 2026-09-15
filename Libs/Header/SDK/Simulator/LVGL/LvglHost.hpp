/**
 ******************************************************************************
 * @file    LvglHost.hpp
 * @brief   PC host for an LVGL GUI process running against the mock kernel.
 *
 * The TouchGFX simulators let the TouchGFX HAL own the window and the frame
 * loop. An LVGL GUI process instead keeps the loop it has on the watch
 * (SDK::LVGL::Port::run()), and this class supplies everything the kernel
 * normally does around it:
 *
 *   window  - an SDL2 window shows the ABGR2222 frame the port sends to the
 *             kernel, scaled up, so the simulator displays exactly the pixels
 *             the watch would.
 *   ticks   - a thread posts EVENT_GUI_TICK at SDK::GUI::Config::kFrameRate,
 *             so the GUI renders at the watch's frame rate.
 *   buttons - keys 1..4 are the watch buttons L1, L2, R1, R2. A key down is a
 *             PRESS, a key up a RELEASE, and a hold shorter than the kernel's
 *             500 ms is a CLICK first, the same order the kernel emits. The
 *             press row q w e r and the release row a s d f of
 *             SDK/GUI/Button.hpp send that one event, as they do in the
 *             TouchGFX simulators (the L1+R2 chord 'z' has no key here).
 *             Escape or closing the window stops the app.
 *   lifecycle - the resume/suspend/stop commands the kernel sends around a
 *             GUI's life, and the matching notifications to the service.
 *
 * The window is pumped from the port's frame hook, so all SDL calls and all
 * LVGL calls stay on the one GUI thread.
 ******************************************************************************
 */

#ifndef SDK_SIMULATOR_LVGL_LVGL_HOST_HPP
#define SDK_SIMULATOR_LVGL_LVGL_HOST_HPP

#include <atomic>
#include <cstdint>
#include <thread>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Simulator/App/DualAppComm.hpp"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace SDK::Simulator
{

class LvglHost
{
public:
    struct Options {
        const char* title = "UNA LVGL simulator";
        int         scale = 2;   ///< window pixels per display pixel
    };

    /**
     * @param comm    The simulator's Service/GUI message queues.
     * @param kernel  A kernel whose comm allocates messages (the service's, as
     *                the TouchGFX simulator core does).
     */
    LvglHost(SDK::App::DualAppComm& comm, const SDK::Kernel& kernel, const Options& options);
    ~LvglHost();

    /// Create the window and hook the port and the mock system. False on SDL failure.
    bool init();

    /// Tell both processes the GUI is running and start the tick thread.
    /// Call after SDK::LVGL::Port::init() and the app's own init.
    void start();

    /// Run the GUI until the app exits or the window is closed.
    void run();

    /// Stop the ticks, tell the service to stop and destroy the window.
    void shutdown();

private:
    LvglHost(const LvglHost&)            = delete;
    LvglHost& operator=(const LvglHost&) = delete;

    static void frameHook(void* ctx);
    static void stopFromSystem();

    void pumpEvents();
    void present(bool force);
    void requestStop();
    void tickThread();

    bool sendToGui(SDK::MessageType::Type type);
    void sendToService(SDK::MessageType::Type type);
    void sendButton(SDK::Message::EventButton::Id id, SDK::Message::EventButton::Event event);
    static bool keyToButton(int32_t key, SDK::Message::EventButton::Id& id);
    static bool keyToRawEvent(int32_t key, SDK::Message::EventButton::Id& id,
                              SDK::Message::EventButton::Event& event);

    SDK::App::DualAppComm& mComm;
    const SDK::Kernel&     mKernel;
    Options                mOptions;

    SDL_Window*   mWindow   = nullptr;
    SDL_Renderer* mRenderer = nullptr;
    SDL_Texture*  mTexture  = nullptr;

    std::thread       mTickThread;
    std::atomic<bool> mTicking { false };
    std::atomic<bool> mTickPending { false };   ///< a tick is queued that the GUI thread has not seen
    bool              mSdlReady = false;   ///< SDL_Init() succeeded; SDL_Quit() owed
    bool              mStarted  = false;   ///< start() told the processes the GUI runs

    uint32_t mShownFrame    = 0;
    bool     mStopRequested = false;
    uint32_t mPressedAtMs[4] = {};
    bool     mPressed[4]     = {};
};

} // namespace SDK::Simulator

#endif // SDK_SIMULATOR_LVGL_LVGL_HOST_HPP
