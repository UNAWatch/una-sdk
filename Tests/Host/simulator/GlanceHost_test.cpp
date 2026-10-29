#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>
#include <SDL.h>

#include "SDK/Simulator/App/AppMessageCore.hpp"
#include "SDK/Simulator/Glance/GlanceHost.hpp"
#include "SDK/Simulator/Kernel/Kernel.hpp"
#include "SDK/Simulator/OS/OS.hpp"
#include "SDK/UnaLogger/Logger.h"

namespace
{
bool check(bool condition, const char* message)
{
    if (!condition) std::fprintf(stderr, "FAILED: %s\n", message);
    return condition;
}

bool pixel(SDL_Surface* surface, int column, int row, uint8_t red, uint8_t green, uint8_t blue)
{
    uint32_t packed = 0;
    const auto* address = static_cast<const uint8_t*>(surface->pixels) + row * surface->pitch + column * surface->format->BytesPerPixel;
    std::memcpy(&packed, address, surface->format->BytesPerPixel);
    uint8_t actualRed, actualGreen, actualBlue;
    SDL_GetRGB(packed, surface->format, &actualRed, &actualGreen, &actualBlue);
    return actualRed == red && actualGreen == green && actualBlue == blue;
}
}

int main()
{
    bool passed = true;
    OS::Semaphore semaphore(1, 0);
    passed &= check(!semaphore.take(0), "empty semaphore must not be acquired");
    const auto before = std::chrono::steady_clock::now();
    passed &= check(!semaphore.take(15), "empty semaphore must time out");
    passed &= check(std::chrono::steady_clock::now() - before >= std::chrono::milliseconds(10), "timeout must wait");
    semaphore.give();
    semaphore.give();
    passed &= check(semaphore.take(0) && !semaphore.take(0), "semaphore count must be bounded");
    std::thread producer([&] { semaphore.give(); });
    passed &= check(semaphore.take(UINT32_MAX), "semaphore must wake across threads");
    producer.join();

    SDK::Simulator::Kernel kernel("glance-test");
    SDK::App::MessageCore core;
    auto& comm = core.getAppComm();
    kernel.setIAppComm(comm.getServiceComm());
    Logger_init(kernel.getKernel().log);
    SDK::Simulator::GlanceHost::Options options;
    options.durationMs = 40;
    options.screenshot = "glance-controls.bmp";
    SDK::Simulator::GlanceHost host(comm, kernel.getKernel(), options);
    if (!check(host.init(), SDL_GetError())) return 1;

    GlanceControl_t controls[6]{};
    for (auto& control : controls) control.visible = true;
    controls[0].type = GLANCE_TYPE_RECT;
    controls[0].payload.rect = {{0, 0}, {40, 40}, GLANCE_COLOR_RED, GLANCE_COLOR_BLUE, true};
    uint8_t image[] = {0xC3, 0xCC, 0x00, 0x43};
    controls[1].type = GLANCE_TYPE_IMAGE;
    controls[1].payload.image = {{10, 10}, {4, 1}, image};
    controls[2].type = GLANCE_TYPE_LINE;
    controls[2].payload.line = {{50, 10}, {70, 10}, GLANCE_COLOR_WHITE};
    controls[3].type = GLANCE_TYPE_TEXT;
    auto& text = controls[3].payload.text;
    text.pos = {80, 0};
    text.size = {150, 30};
    text.font = GLANCE_FONT_POPPINS_SEMIBOLD_20;
    text.color = GLANCE_COLOR_TEAL;
    std::strcpy(text.str, "GLANCE");
    const auto copiedText = text;
    controls[4] = controls[0];
    controls[4].visible = false;
    controls[4].payload.rect.size = {240, 60};
    controls[5].type = GLANCE_TYPE_RECT;
    controls[5].payload.rect = {{235, 55}, {20, 20}, GLANCE_COLOR_GREEN, GLANCE_COLOR_GREEN, true};

    SDK::Message::RequestGlanceUpdate update;
    update.controls = controls;
    update.controlsNumber = 6;
    passed &= check(host.capture(update), "valid controls must be accepted");
    std::memset(image, 0, sizeof(image));
    std::memset(controls, 0, sizeof(controls));
    passed &= check(host.run(), "frame must render and save");
    auto* surface = SDL_LoadBMP(options.screenshot.c_str());
    if (!check(surface != nullptr, "screenshot must load")) return 1;
    passed &= check(surface->w == 240 && surface->h == 60, "frame dimensions must match glance config");
    passed &= check(pixel(surface, 0, 0, 255, 0, 0), "rectangle border must render");
    passed &= check(pixel(surface, 5, 5, 0, 0, 255), "rectangle fill and hidden controls must render correctly");
    passed &= check(pixel(surface, 10, 10, 255, 0, 0), "image red channel and copied pixels must be preserved");
    passed &= check(pixel(surface, 11, 10, 0, 255, 0), "image green channel must render");
    passed &= check(pixel(surface, 12, 10, 0, 0, 255), "transparent image pixels must preserve background");
    passed &= check(!pixel(surface, 13, 10, 255, 0, 0) && !pixel(surface, 13, 10, 0, 0, 255), "partial alpha must blend");
    passed &= check(pixel(surface, 60, 10, 255, 255, 255), "line must render");
    passed &= check(pixel(surface, 239, 59, 0, 255, 0), "controls must clip at the frame edge");
    unsigned textPixels = 0;
    for (int row = 0; row < 30; ++row)
        for (int column = 80; column < 230; ++column)
            if (!pixel(surface, column, row, 0, 0, 0)) ++textPixels;
    passed &= check(textPixels > 50, "copied text must produce visible glyphs");
    SDL_FreeSurface(surface);

    update.controlsNumber = 33;
    passed &= check(!host.capture(update), "too many controls must fail");
    update.controlsNumber = 1;
    update.controls = nullptr;
    passed &= check(!host.capture(update), "null controls must fail");
    update.controls = controls;
    controls[0].type = GLANCE_TYPES_COUNT;
    passed &= check(!host.capture(update), "unknown control types must fail");
    controls[0].type = GLANCE_TYPE_IMAGE;
    controls[0].visible = true;
    passed &= check(!host.capture(update), "invalid image storage must fail");
    passed &= check(host.updateCount() == 1, "rejected updates must not replace the frame");

    controls[0].type = GLANCE_TYPE_TEXT;
    controls[0].payload.text = copiedText;
    std::memset(controls[0].payload.text.str, 'X', GLANCE_TEXT_SIZE + 1);
    passed &= check(host.capture(update) && host.run(), "unterminated text must be bounded safely");
    update.controls = nullptr;
    update.controlsNumber = 0;
    passed &= check(host.capture(update) && host.run(), "empty update must clear the frame");
    surface = SDL_LoadBMP(options.screenshot.c_str());
    if (!check(surface != nullptr, "cleared screenshot must load")) return 1;
    passed &= check(pixel(surface, 0, 0, 0, 0, 0) && pixel(surface, 239, 59, 0, 0, 0), "old controls must be removed");
    SDL_FreeSurface(surface);
    return passed ? 0 : 1;
}
