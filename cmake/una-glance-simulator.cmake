if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "Set UNA_SDK to the SDK root before configuring a glance simulator.")
endif()

set(UNA_LVGL_CONF "$ENV{UNA_SDK}/Libs/Header/SDK/Simulator/Glance/lv_conf.h")
include(${CMAKE_CURRENT_LIST_DIR}/una-simulator.cmake)

#[[
Create a host glance executable and its shared runtime from service sources.
APP_ID is required; SOURCES supplies Service implementations and INCLUDE_DIRS
must expose Service.hpp and the project's ConfigurationSimulator.hpp layout.
The target inherits the simulator kernel, SDL2, LVGL and C++17 requirements.
]]
function(una_add_glance_simulator target)
    cmake_parse_arguments(GLANCE "" "APP_ID" "SOURCES;INCLUDE_DIRS" ${ARGN})
    if(NOT GLANCE_APP_ID)
        message(FATAL_ERROR "una_add_glance_simulator requires APP_ID.")
    endif()
    add_library(${target}Runtime STATIC
        ${UNA_SDK_SOURCES_SIMULATOR}
        ${UNA_SDK_SOURCES_SIMULATOR_SERVICE}
        ${UNA_SDK_LVGL_SOURCES}
        "$ENV{UNA_SDK}/Libs/Source/Simulator/Glance/GlanceHost.cpp"
    )
    target_compile_features(${target}Runtime PUBLIC cxx_std_17)
    target_include_directories(${target}Runtime PUBLIC
        ${UNA_SDK_INCLUDE_DIRS_SIMULATOR}
        ${UNA_SDK_INCLUDE_DIRS_GUI_LVGL}
        ${GLANCE_INCLUDE_DIRS}
    )
    target_compile_definitions(${target}Runtime PUBLIC
        ${UNA_SDK_DEFINES_SIMULATOR}
        ${UNA_SDK_DEFINES_GUI_LVGL}
        APP_ID="${GLANCE_APP_ID}"
        DEV_ID="UNA Simulator"
        BUILD_VERSION="1.0.0-sim"
    )
    find_package(Threads REQUIRED)
    target_link_libraries(${target}Runtime PUBLIC Threads::Threads)
    una_simulator_link_sdl2(${target}Runtime)
    add_executable(${target}
        "$ENV{UNA_SDK}/Libs/Source/Simulator/Glance/main.cpp"
        ${GLANCE_SOURCES}
    )
    target_compile_definitions(${target} PRIVATE APP_NAME="${target}")
    target_link_libraries(${target} PRIVATE ${target}Runtime)
    una_simulator_link_sdl2(${target})
endfunction()
