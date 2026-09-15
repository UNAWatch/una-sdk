# ScrollMenu LVGL PC simulator: the app's service and LVGL GUI processes
# running against the SDK's mock kernel, with the display in an SDL2 window.
#
# Windows (Visual Studio; uses the SDL2 shipped with TouchGFX, which is 32-bit):
#   cmake -S . -B build -G "Visual Studio 18 2026" -A Win32
#   cmake --build build --config Debug
# Linux (needs libsdl2-dev):
#   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build
# Run from build/bin so the mock file system lands in ../../../../../Output,
# i.e. this tutorial's Software/Output (ignored by Docs/Tutorials/.gitignore).
cmake_minimum_required(VERSION 3.21)

if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Point it at the SDK root.")
endif()

project(ScrollMenuLVGLSimulator C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# Source lists: the mock kernel, the LVGL port and host, the app's own code.
include($ENV{UNA_SDK}/cmake/una-simulator.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../../Libs/libs.cmake)   # LIBS_SOURCES, LIBS_INCLUDE_DIRS
include(${CMAKE_CURRENT_SOURCE_DIR}/../lvgl-gui.cmake)          # GUI_APP_SOURCES, GUI_APP_INCLUDE_DIRS

# One output directory for every configuration, so the relative mock file
# system path resolves the same everywhere.
set(SIM_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SIM_BIN_DIR}")
foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${cfg} CFG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} "${SIM_BIN_DIR}")
endforeach()

add_executable(ScrollMenuLVGLSimulator
    main.cpp
    ${GUI_APP_SOURCES}
    ${LIBS_SOURCES}
    ${UNA_SDK_SOURCES_SIMULATOR}
    ${UNA_SDK_SOURCES_SIMULATOR_SERVICE}
    ${UNA_SDK_SOURCES_SIMULATOR_LVGL}
)

target_include_directories(ScrollMenuLVGLSimulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}          # ConfigurationSimulator.hpp
    ${GUI_APP_INCLUDE_DIRS}
    ${LIBS_INCLUDE_DIRS}
    ${UNA_SDK_INCLUDE_DIRS_SIMULATOR}
    ${UNA_SDK_INCLUDE_DIRS_GUI_LVGL}
)

# The identity the .uapp build derives from CMakeLists.txt and git.
target_compile_definitions(ScrollMenuLVGLSimulator PRIVATE
    ${UNA_SDK_DEFINES_SIMULATOR}
    ${UNA_SDK_DEFINES_GUI_LVGL}
    APP_ID="1DF9AA3590D71631"
    DEV_ID="UNA Simulator"
    BUILD_VERSION="1.0.0-sim"
)

find_package(Threads REQUIRED)
target_link_libraries(ScrollMenuLVGLSimulator PRIVATE Threads::Threads)
una_simulator_link_sdl2(ScrollMenuLVGLSimulator)

if(MSVC)
    target_compile_options(ScrollMenuLVGLSimulator PRIVATE /W3 /MP)
else()
    target_compile_options(ScrollMenuLVGLSimulator PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
