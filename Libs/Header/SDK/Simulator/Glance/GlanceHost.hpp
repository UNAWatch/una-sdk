#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/Messages/CommandMessages.hpp"
#include "SDK/Simulator/App/DualAppComm.hpp"
#include "lvgl.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace SDK::Simulator
{

class GlanceHost
{
public:
    static constexpr int kWidth = 240;
    static constexpr int kHeight = 60;
    static constexpr unsigned kMaxControls = 32;

    struct Options {
        const char* title = "UNA Glance";
        int scale = 3;
        uint32_t durationMs = 0;
        std::string screenshot;
    };

    GlanceHost(SDK::App::DualAppComm& comm, const SDK::Kernel& kernel, const Options& options);
    ~GlanceHost();
    GlanceHost(const GlanceHost&) = delete;
    GlanceHost& operator=(const GlanceHost&) = delete;

    bool init();
    bool capture(const SDK::Message::RequestGlanceUpdate& update);
    bool start();
    bool run(const std::function<bool()>& keepRunning = {});
    bool stop();
    bool saveScreenshot(const std::string& path) const;
    uint32_t updateCount() const;

private:
    struct Control {
        GlanceControl_t value{};
        std::vector<uint8_t> image;
        lv_image_dsc_t descriptor{};
        std::array<lv_point_precise_t, 2> points{};
    };

    static void flush(lv_display_t* display, const lv_area_t* area, uint8_t* pixels);
    void render();
    bool send(SDK::MessageType::Type type);

    SDK::App::DualAppComm& mComm;
    const SDK::Kernel& mKernel;
    Options mOptions;
    SDL_Window* mWindow = nullptr;
    SDL_Renderer* mRenderer = nullptr;
    SDL_Texture* mTexture = nullptr;
    lv_display_t* mDisplay = nullptr;
    alignas(LV_DRAW_BUF_ALIGN) std::array<uint16_t, kWidth * kHeight> mBuffer{};
    std::array<uint16_t, kWidth * kHeight> mFrame{};
    mutable std::mutex mMutex;
    std::vector<Control> mPending;
    std::vector<Control> mShown;
    uint32_t mUpdates = 0;
    uint32_t mRendered = 0;
    uint32_t mFrameNumber = 0;
    bool mSdlReady = false;
    bool mStarted = false;
};

}