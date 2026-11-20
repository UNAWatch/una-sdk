#include "Screen.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"
#include <new>

namespace {
alignas(Model) uint8_t modelMemory[sizeof(Model)];
alignas(Screen) uint8_t screenMemory[sizeof(Screen)];
}
extern "C" const lv_font_t* una_lvgl_default_font() { return &lv_font_montserrat_14; }
extern "C" void una_lvgl_app_init() {
    SDK::LVGL::Draw::init();
    auto* model = new(modelMemory) Model;
    auto* screen = new(screenMemory) Screen(*model);
    lv_screen_load(screen->root());
}
