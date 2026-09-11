/**
 ******************************************************************************
 * @file    LvglHost.cpp
 * @brief   PC host for an LVGL GUI process (see LvglHost.hpp).
 ******************************************************************************
 */

#include "SDK/Simulator/LVGL/LvglHost.hpp"

#include <chrono>

#include <SDL.h>

#include "SDK/GUI/Config.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Port/LVGL/LvglPort.hpp"
#include "SDK/Simulator/Components/InstanceSensorLayer.hpp"
#include "SDK/Simulator/Kernel/Mock/System.hpp"

#define LOG_MODULE_PRX      "LvglHost"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace SDK::Simulator
{

namespace
{

/// The kernel reports a CLICK for a press shorter than this (HWButtons.cpp).
constexpr uint32_t kClickMaxMs = 500;

/// Texture format with bytes R, G, B, A in memory order. Spelled out because
/// the alias SDL_PIXELFORMAT_RGBA32 only exists from SDL 2.0.5 and the copy
/// TouchGFX ships is 2.0.4.
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
constexpr Uint32 kTextureFormat = SDL_PIXELFORMAT_RGBA8888;
#else
constexpr Uint32 kTextureFormat = SDL_PIXELFORMAT_ABGR8888;
#endif

using Btn = SDK::Message::EventButton;

/// Expand one ABGR2222 byte (A[7:6] B[5:4] G[3:2] R[1:0]) to 8-bit channels.
inline uint32_t abgr2222ToRgba32(uint8_t px)
{
    const uint32_t r = (px & 0x03) * 85u;
    const uint32_t g = ((px >> 2) & 0x03) * 85u;
    const uint32_t b = ((px >> 4) & 0x03) * 85u;
    // kTextureFormat: bytes R, G, B, A in memory order.
    return SDL_BYTEORDER == SDL_LIL_ENDIAN ? (0xFFu << 24) | (b << 16) | (g << 8) | r
                                           : (r << 24) | (g << 16) | (b << 8) | 0xFFu;
}

} // namespace

LvglHost* LvglHost::sInstance = nullptr;

LvglHost::LvglHost(SDK::App::DualAppComm& comm, const SDK::Kernel& kernel, const Options& options)
    : mComm(comm)
    , mKernel(kernel)
    , mOptions(options)
{
    sInstance = this;
}

LvglHost::~LvglHost()
{
    shutdown();
    sInstance = nullptr;
}

bool LvglHost::init()
{
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LOG_ERROR("SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    const int scale = mOptions.scale > 0 ? mOptions.scale : 1;
    mWindow = SDL_CreateWindow(mOptions.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               SDK::LVGL::kDisplayWidth * scale, SDK::LVGL::kDisplayHeight * scale,
                               SDL_WINDOW_SHOWN);
    if (!mWindow) {
        LOG_ERROR("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    mRenderer = SDL_CreateRenderer(mWindow, -1, 0);
    if (!mRenderer) {
        LOG_ERROR("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_RenderSetLogicalSize(mRenderer, SDK::LVGL::kDisplayWidth, SDK::LVGL::kDisplayHeight);

    mTexture = SDL_CreateTexture(mRenderer, kTextureFormat, SDL_TEXTUREACCESS_STREAMING,
                                 SDK::LVGL::kDisplayWidth, SDK::LVGL::kDisplayHeight);
    if (!mTexture) {
        LOG_ERROR("SDL_CreateTexture failed: %s\n", SDL_GetError());
        return false;
    }

    SDK::LVGL::Port::GetInstance().setFrameHook(&LvglHost::frameHook, this);
    Mock::SystemGUI::SetStopHandler(&LvglHost::stopFromSystem);

    LOG_INFO("Window %dx%d (x%d). Keys: 1=L1 2=L2 3=R1 4=R2, Esc quits.\n",
             SDK::LVGL::kDisplayWidth, SDK::LVGL::kDisplayHeight, scale);
    return true;
}

void LvglHost::start()
{
    // What the kernel sends when it brings a GUI to the front.
    sendToService(SDK::MessageType::COMMAND_APP_NOTIF_GUI_RUN);
    sendToGui(SDK::MessageType::COMMAND_APP_GUI_RESUME);

    mTicking    = true;
    mTickThread = std::thread(&LvglHost::tickThread, this);
}

void LvglHost::run()
{
    SDK::LVGL::Port::GetInstance().run();
}

void LvglHost::shutdown()
{
    if (mTicking.exchange(false) && mTickThread.joinable()) {
        mTickThread.join();
    }

    if (mWindow) {
        // The GUI has already stopped (run() returned); now the service.
        sendToService(SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP);
        sendToService(SDK::MessageType::COMMAND_APP_STOP);
    }

    if (mTexture) {
        SDL_DestroyTexture(mTexture);
        mTexture = nullptr;
    }
    if (mRenderer) {
        SDL_DestroyRenderer(mRenderer);
        mRenderer = nullptr;
    }
    if (mWindow) {
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
        SDL_Quit();
    }
}

// --- hooks -----------------------------------------------------------------

void LvglHost::frameHook(void* ctx)
{
    auto* self = static_cast<LvglHost*>(ctx);
    self->pumpEvents();
    self->present(false);
}

void LvglHost::stopFromSystem()
{
    // The app called sys.exit(), or the kernel-side stop reached the GUI.
    SDK::LVGL::Port::GetInstance().stop();
}

// --- window ----------------------------------------------------------------

void LvglHost::pumpEvents()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                requestStop();
                break;

            case SDL_KEYDOWN: {
                if (e.key.repeat) {
                    break;
                }
                const int32_t key = e.key.keysym.sym;
                Btn::Id id;
                if (key == SDLK_ESCAPE) {
                    requestStop();
                } else if (keyToButton(key, id)) {
                    const auto i = static_cast<unsigned>(id);
                    mPressed[i]     = true;
                    mPressedAtMs[i] = mKernel.sys.getTimeMs();
                    sendButton(id, Btn::Event::PRESS);
                } else if (key > 0 && key < 0x80) {
                    // Simulator-only keys, e.g. '5' raises a wrist-motion event.
                    Instance::SensorLayer::getInstance().handlerButtons(static_cast<uint8_t>(key));
                }
            } break;

            case SDL_KEYUP: {
                Btn::Id id;
                if (keyToButton(e.key.keysym.sym, id)) {
                    const auto i = static_cast<unsigned>(id);
                    if (mPressed[i]) {
                        mPressed[i] = false;
                        if (mKernel.sys.getTimeMs() - mPressedAtMs[i] < kClickMaxMs) {
                            sendButton(id, Btn::Event::CLICK);
                        }
                        sendButton(id, Btn::Event::RELEASE);
                    }
                }
            } break;

            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_EXPOSED) {
                    present(true);
                }
                break;

            default:
                break;
        }
    }
}

void LvglHost::present(bool force)
{
    auto& port = SDK::LVGL::Port::GetInstance();
    if (!force && port.frameCount() == mShownFrame) {
        return;
    }
    mShownFrame = port.frameCount();

    void* pixels = nullptr;
    int   pitch  = 0;
    if (SDL_LockTexture(mTexture, nullptr, &pixels, &pitch) != 0) {
        return;
    }
    const uint8_t* src = port.frame();
    for (int y = 0; y < SDK::LVGL::kDisplayHeight; ++y) {
        auto* row = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(pixels) + y * pitch);
        for (int x = 0; x < SDK::LVGL::kDisplayWidth; ++x) {
            row[x] = abgr2222ToRgba32(*src++);
        }
    }
    SDL_UnlockTexture(mTexture);

    SDL_RenderClear(mRenderer);
    SDL_RenderCopy(mRenderer, mTexture, nullptr, nullptr);
    SDL_RenderPresent(mRenderer);
}

void LvglHost::requestStop()
{
    if (mStopRequested) {
        return;
    }
    mStopRequested = true;
    // Same order as the kernel: the GUI is taken off screen, then stopped.
    // The port handles COMMAND_APP_STOP by calling sys.exit(), which reaches
    // stopFromSystem() and ends run().
    sendToGui(SDK::MessageType::COMMAND_APP_GUI_SUSPEND);
    sendToGui(SDK::MessageType::COMMAND_APP_STOP);
}

// --- kernel emulation --------------------------------------------------------

void LvglHost::tickThread()
{
    const auto period = std::chrono::milliseconds(1000 / SDK::GUI::Config::kFrameRate);
    uint32_t frame = 0;
    while (mTicking) {
        std::this_thread::sleep_for(period);
        auto msg = SDK::make_msg<SDK::Message::EventGuiTick>(mKernel);
        if (!msg) {
            continue;
        }
        msg->timestamp   = mKernel.sys.getTimeMs();
        msg->frameNumber = frame++;
        if (!mComm.sendToGui(msg.get())) {
            continue;   // the guard releases it
        }
        msg.release();  // the GUI releases it after handling
    }
}

void LvglHost::sendToGui(SDK::MessageType::Type type)
{
    auto msg = SDK::make_msg(mKernel, type);
    if (msg && mComm.sendToGui(msg.get())) {
        msg.release();
    }
}

void LvglHost::sendToService(SDK::MessageType::Type type)
{
    auto msg = SDK::make_msg(mKernel, type);
    if (msg && mComm.sendToService(msg.get())) {
        msg.release();
    }
}

void LvglHost::sendButton(Btn::Id id, Btn::Event event)
{
    auto msg = SDK::make_msg<Btn>(mKernel);
    if (!msg) {
        return;
    }
    msg->timestamp = mKernel.sys.getTimeMs();
    msg->id        = id;
    msg->event     = event;
    if (mComm.sendToGui(msg.get())) {
        msg.release();
    }
}

bool LvglHost::keyToButton(int32_t key, Btn::Id& id)
{
    // Same key layout as the TouchGFX simulators: 1=L1 2=L2 3=R1 4=R2.
    // Button ids follow the hardware: SW1=L1, SW2=R1, SW3=L2, SW4=R2.
    switch (key) {
        case SDLK_1: id = Btn::Id::SW1; return true;
        case SDLK_2: id = Btn::Id::SW3; return true;
        case SDLK_3: id = Btn::Id::SW2; return true;
        case SDLK_4: id = Btn::Id::SW4; return true;
        default:     return false;
    }
}

} // namespace SDK::Simulator
