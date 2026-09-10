/**
 ******************************************************************************
 * @file    GuiApp.cpp
 * @brief   RunLVGL GUI entry: builds the model and shows the first screen.
 *
 * una_lvgl_app_init() is the hook the SDK's LVGL entry point calls once LVGL
 * and the display are up (Libs/Source/AppSystem/EntryPoint/LVGL/main.cpp).
 * Objects are constructed here, not as globals, so nothing touches the kernel
 * before main() has bound it.
 ******************************************************************************
 */

#include <new>

#include "gui/model/Model.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/screens/ScreenManager.hpp"

namespace
{

alignas(Model) uint8_t sModelStorage[sizeof(Model)];
Model* sModel = nullptr;

} // namespace

extern "C" void una_lvgl_app_init(void)
{
    Theme::init();
    sModel = new (sModelStorage) Model();
    ScreenManager::instance().start(*sModel, ScreenId::Main);
}
