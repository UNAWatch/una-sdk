# HelloWorld LVGL PC simulator: the app's service and LVGL GUI processes
# running against the SDK's mock kernel, with the display in an SDL2 window.
#
# Needs a host C++ compiler, CMake, Ninja (or a Visual Studio generator) and
# SDL2; no IDE. Linux or WSL: GCC, ninja-build and libsdl2-dev. Windows: MSVC,
# from the free Build Tools for Visual Studio or the IDE (its "C++ CMake tools"
# component brings CMake and Ninja); without an installed SDL2 the build reuses
# the 32-bit copy TouchGFX Designer ships and must then be 32-bit (an x86
# developer command prompt, or -A Win32).
#
# Linux, WSL, or a Windows x86 Native Tools Command Prompt:
#   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build
# Windows, any shell, also writing a Visual Studio solution ("Visual Studio 18
# 2026" needs CMake 4.2 or newer; "Visual Studio 17 2022" works with 3.21):
#   cmake -S . -B build -G "Visual Studio 18 2026" -A Win32
#   cmake --build build --config Debug
# Run from build/bin so the mock file system lands in ../../../../../Output,
# i.e. this tutorial's Software/Output (ignored by Docs/Tutorials/.gitignore).
cmake_minimum_required(VERSION 3.21)

if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Point it at the SDK root.")
endif()

project(HelloWorldLVGLSimulator C CXX)

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

add_executable(HelloWorldLVGLSimulator
    main.cpp
    ${GUI_APP_SOURCES}
    ${LIBS_SOURCES}
    ${UNA_SDK_SOURCES_SIMULATOR}
    ${UNA_SDK_SOURCES_SIMULATOR_SERVICE}
    ${UNA_SDK_SOURCES_SIMULATOR_LVGL}
)

target_include_directories(HelloWorldLVGLSimulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}          # ConfigurationSimulator.hpp
    ${GUI_APP_INCLUDE_DIRS}
    ${LIBS_INCLUDE_DIRS}
    ${UNA_SDK_INCLUDE_DIRS_SIMULATOR}
    ${UNA_SDK_INCLUDE_DIRS_GUI_LVGL}
)

# The identity the .uapp build derives from CMakeLists.txt and git.
target_compile_definitions(HelloWorldLVGLSimulator PRIVATE
    ${UNA_SDK_DEFINES_SIMULATOR}
    ${UNA_SDK_DEFINES_GUI_LVGL}
    APP_ID="94D02A02E9B3EC2E"
    DEV_ID="UNA Simulator"
    BUILD_VERSION="1.0.0-sim"
)

find_package(Threads REQUIRED)
target_link_libraries(HelloWorldLVGLSimulator PRIVATE Threads::Threads)
una_simulator_link_sdl2(HelloWorldLVGLSimulator)

if(MSVC)
    target_compile_options(HelloWorldLVGLSimulator PRIVATE /W3 /MP)
else()
    target_compile_options(HelloWorldLVGLSimulator PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
