#include "SDK/Simulator/Glance/GlanceHost.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <SDL.h>
#include "src/misc/cache/instance/lv_image_cache.h"

#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Simulator/Components/InstanceSensorLayer.hpp"

extern "C" const lv_font_t* una_lvgl_default_font(void)
{
    return &lv_font_montserrat_18;
}

extern "C" void una_lvgl_assert_failed(const char* file, int line)
{
    std::fprintf(stderr, "LVGL assertion at %s:%d\n", file, line);
    std::abort();
}

namespace SDK::Simulator
{
namespace
{
lv_color_t color(uint8_t value)
{
    return lv_color_make(((value >> 4) & 3) * 85, ((value >> 2) & 3) * 85, (value & 3) * 85);
}

const lv_font_t* font(GlanceFont_t value)
{
    switch (value) {
        case GLANCE_FONT_POPPINS_MEDIUM_10: return &lv_font_montserrat_10;
        case GLANCE_FONT_POPPINS_SEMIBOLD_20:
        case GLANCE_FONT_POPPINS_ITALIC_20: return &lv_font_montserrat_20;
        case GLANCE_FONT_POPPINS_MEDIUM_25:
        case GLANCE_FONT_POPPINS_SEMIBOLD_25: return &lv_font_montserrat_26;
        case GLANCE_FONT_POPPINS_SEMIBOLD_30: return &lv_font_montserrat_30;
        case GLANCE_FONT_POPPINS_SEMIBOLD_35: return &lv_font_montserrat_36;
        default: return &lv_font_montserrat_18;
    }
}
}

GlanceHost::GlanceHost(SDK::App::DualAppComm& comm, const SDK::Kernel& kernel, const Options& options)
    : mComm(comm), mKernel(kernel), mOptions(options)
{
}

GlanceHost::~GlanceHost()
{
    if (mDisplay) {
        lv_display_delete(mDisplay);
        lv_deinit();
    }
    if (mTexture) SDL_DestroyTexture(mTexture);
    if (mRenderer) SDL_DestroyRenderer(mRenderer);
    if (mWindow) SDL_DestroyWindow(mWindow);
    if (mSdlReady) SDL_Quit();
}

bool GlanceHost::init()
{
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;
    mSdlReady = true;
    const int scale = std::max(1, mOptions.scale);
    mWindow = SDL_CreateWindow(mOptions.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               kWidth * scale, kHeight * scale, SDL_WINDOW_SHOWN);
    if (!mWindow) return false;
    mRenderer = SDL_CreateRenderer(mWindow, -1, 0);
    if (!mRenderer) return false;
    SDL_RenderSetLogicalSize(mRenderer, kWidth, kHeight);
    mTexture = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                                 kWidth, kHeight);
    if (!mTexture) return false;

    lv_init();
    lv_tick_set_cb(SDL_GetTicks);
    mDisplay = lv_display_create(kWidth, kHeight);
    if (!mDisplay) return false;
    lv_display_set_user_data(mDisplay, this);
    lv_display_set_color_format(mDisplay, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(mDisplay, mBuffer.data(), nullptr, sizeof(mBuffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(mDisplay, &GlanceHost::flush);
    auto* screen = lv_display_get_screen_active(mDisplay);
    lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    std::puts("Glance simulator: Esc quits; simulated sensor keys follow ConfigurationSimulator.hpp.");
    return true;
}

bool GlanceHost::capture(const SDK::Message::RequestGlanceUpdate& update)
{
    if (update.controlsNumber > kMaxControls || (update.controlsNumber && !update.controls)) return false;
    std::vector<Control> controls;
    controls.reserve(update.controlsNumber);
    for (uint32_t index = 0; index < update.controlsNumber; ++index) {
        Control control;
        control.value = update.controls[index];
        if (control.value.type < GLANCE_TYPE_TEXT || control.value.type >= GLANCE_TYPES_COUNT) return false;
        if (control.value.type == GLANCE_TYPE_TEXT) control.value.payload.text.str[GLANCE_TEXT_SIZE] = '\0';
        if (control.value.type == GLANCE_TYPE_IMAGE && control.value.visible) {
            const auto& image = control.value.payload.image;
            if (!image.buff || !image.size.w || !image.size.h || image.size.w > kWidth || image.size.h > kHeight) return false;
            control.image.resize(static_cast<size_t>(image.size.w) * image.size.h * 4);
            for (size_t pixel = 0; pixel < control.image.size() / 4; ++pixel) {
                const uint8_t packed = image.buff[pixel];
                control.image[pixel * 4] = ((packed >> 4) & 3) * 85;
                control.image[pixel * 4 + 1] = ((packed >> 2) & 3) * 85;
                control.image[pixel * 4 + 2] = (packed & 3) * 85;
                control.image[pixel * 4 + 3] = ((packed >> 6) & 3) * 85;
            }
        }
        controls.push_back(std::move(control));
    }
    std::lock_guard<std::mutex> lock(mMutex);
    mPending = std::move(controls);
    ++mUpdates;
    return true;
}

uint32_t GlanceHost::updateCount() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mUpdates;
}

bool GlanceHost::send(SDK::MessageType::Type type)
{
    auto message = SDK::make_msg(mKernel, type);
    if (!message || !mComm.sendToService(message.get())) return false;
    message.release();
    return true;
}

bool GlanceHost::start()
{
    if (!mDisplay || mStarted) return false;
    mStarted = send(SDK::MessageType::EVENT_GLANCE_START);
    return mStarted;
}

bool GlanceHost::stop()
{
    if (!mStarted) return true;
    if (!send(SDK::MessageType::EVENT_GLANCE_STOP)) return false;
    mStarted = false;
    return true;
}

bool GlanceHost::run(const std::function<bool()>& keepRunning)
{
    const uint32_t startedAt = SDL_GetTicks();
    uint32_t lastTick = startedAt - 100;
    bool running = true;
    while (running && (!keepRunning || keepRunning())) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) running = false;
            else if (event.type == SDL_KEYDOWN && !event.key.repeat && event.key.keysym.sym > 0 && event.key.keysym.sym < 128)
                Instance::SensorLayer::getInstance().handlerButtons(static_cast<uint8_t>(event.key.keysym.sym));
        }
        const uint32_t now = SDL_GetTicks();
        if (mOptions.durationMs && now - startedAt >= mOptions.durationMs) running = false;
        if (mStarted && now - lastTick >= 100) {
            auto tick = SDK::make_msg<SDK::Message::EventGlanceTick>(mKernel);
            if (tick) {
                tick->frameNumber = ++mFrameNumber;
                tick->timestamp = mKernel.sys.getTimeMs();
                if (mComm.sendToService(tick.get())) tick.release();
            }
            lastTick = now;
        }
        render();
        lv_timer_handler();
        SDL_UpdateTexture(mTexture, nullptr, mFrame.data(), kWidth * sizeof(uint16_t));
        SDL_RenderClear(mRenderer);
        SDL_RenderCopy(mRenderer, mTexture, nullptr, nullptr);
        SDL_RenderPresent(mRenderer);
        SDL_Delay(5);
    }
    const bool saved = mOptions.screenshot.empty() || saveScreenshot(mOptions.screenshot);
    return stop() && saved;
}

void GlanceHost::render()
{
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mRendered == mUpdates) return;
        lv_obj_clean(lv_display_get_screen_active(mDisplay));
        for (auto& control : mShown) {
            if (control.value.type == GLANCE_TYPE_IMAGE) lv_image_cache_drop(&control.descriptor);
        }
        mShown = mPending;
        mRendered = mUpdates;
    }
    auto* screen = lv_display_get_screen_active(mDisplay);
    for (auto& control : mShown) {
        const auto& value = control.value;
        if (!value.visible) continue;
        lv_obj_t* object = nullptr;
        switch (value.type) {
            case GLANCE_TYPE_TEXT: {
                const auto& text = value.payload.text;
                object = lv_label_create(screen);
                lv_obj_remove_style_all(object);
                lv_obj_set_pos(object, text.pos.x, text.pos.y);
                lv_obj_set_size(object, text.size.w, text.size.h);
                lv_label_set_long_mode(object, LV_LABEL_LONG_CLIP);
                lv_obj_set_style_text_font(object, font(text.font), 0);
                lv_obj_set_style_text_color(object, color(text.color), 0);
                lv_obj_set_style_text_align(object, text.align == GLANCE_ALIGN_H_CENTER ? LV_TEXT_ALIGN_CENTER :
                                            text.align == GLANCE_ALIGN_H_RIGHT ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, 0);
                lv_label_set_text(object, text.str);
            } break;
            case GLANCE_TYPE_IMAGE: {
                const auto& image = value.payload.image;
                auto& descriptor = control.descriptor;
                descriptor.header.magic = LV_IMAGE_HEADER_MAGIC;
                descriptor.header.cf = LV_COLOR_FORMAT_ARGB8888;
                descriptor.header.w = image.size.w;
                descriptor.header.h = image.size.h;
                descriptor.header.stride = image.size.w * 4;
                descriptor.data_size = static_cast<uint32_t>(control.image.size());
                descriptor.data = control.image.data();
                object = lv_image_create(screen);
                lv_image_set_src(object, &descriptor);
                lv_obj_set_pos(object, image.pos.x, image.pos.y);
            } break;
            case GLANCE_TYPE_RECT: {
                const auto& rectangle = value.payload.rect;
                object = lv_obj_create(screen);
                lv_obj_remove_style_all(object);
                lv_obj_set_pos(object, rectangle.pos.x, rectangle.pos.y);
                lv_obj_set_size(object, rectangle.size.w, rectangle.size.h);
                lv_obj_set_style_border_width(object, 1, 0);
                lv_obj_set_style_border_color(object, color(rectangle.color), 0);
                lv_obj_set_style_bg_color(object, color(rectangle.bgColor), 0);
                lv_obj_set_style_bg_opa(object, rectangle.fill ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
            } break;
            case GLANCE_TYPE_LINE: {
                const auto& line = value.payload.line;
                object = lv_line_create(screen);
                lv_obj_remove_style_all(object);
                control.points = {{{static_cast<lv_value_precise_t>(line.start.x), static_cast<lv_value_precise_t>(line.start.y)},
                                    {static_cast<lv_value_precise_t>(line.stop.x), static_cast<lv_value_precise_t>(line.stop.y)}}};
                lv_line_set_points(object, control.points.data(), 2);
                lv_obj_set_style_line_color(object, color(line.color), 0);
                lv_obj_set_style_line_width(object, 1, 0);
            } break;
            default: break;
        }
        if (object) lv_obj_remove_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    }
}

void GlanceHost::flush(lv_display_t* display, const lv_area_t* area, uint8_t* pixels)
{
    auto* host = static_cast<GlanceHost*>(lv_display_get_user_data(display));
    const int width = area->x2 - area->x1 + 1;
    for (int row = area->y1; row <= area->y2; ++row) {
        std::memcpy(host->mFrame.data() + row * kWidth + area->x1, pixels, width * sizeof(uint16_t));
        pixels += width * sizeof(uint16_t);
    }
    lv_display_flush_ready(display);
}

bool GlanceHost::saveScreenshot(const std::string& path) const
{
    auto* surface = SDL_CreateRGBSurfaceFrom(const_cast<uint16_t*>(mFrame.data()), kWidth, kHeight,
                                             16, kWidth * sizeof(uint16_t), 0xF800, 0x07E0, 0x001F, 0);
    if (!surface) return false;
    const bool saved = SDL_SaveBMP(surface, path.c_str()) == 0;
    SDL_FreeSurface(surface);
    return saved;
}

}
