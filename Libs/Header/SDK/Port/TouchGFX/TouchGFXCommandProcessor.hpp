/**
 ******************************************************************************
 * @file    TouchGFXCommandProcessor.hpp
 * @brief   Former name of SDK::GuiCommandProcessor, kept as an alias.
 *
 * The GUI process's kernel message pump never depended on TouchGFX; it was
 * renamed when the LVGL port started using it too. Existing code that includes
 * this header and uses SDK::TouchGFXCommandProcessor keeps building. New code
 * should include SDK/Port/GuiCommandProcessor.hpp and use the new name.
 ******************************************************************************
 */

#ifndef SDK_PORT_TOUCHGFX_TOUCHGFX_COMMAND_PROCESSOR_HPP
#define SDK_PORT_TOUCHGFX_TOUCHGFX_COMMAND_PROCESSOR_HPP

#include "SDK/Port/GuiCommandProcessor.hpp"

namespace SDK
{

using TouchGFXCommandProcessor = GuiCommandProcessor;

} // namespace SDK

#endif // SDK_PORT_TOUCHGFX_TOUCHGFX_COMMAND_PROCESSOR_HPP
