# Sensors with its GUI process built on LVGL. The service is the same
# Software/Libs as Sensors-CMake builds; only the GUI differs. Compare with
# ../Sensors-CMake/CMakeLists.txt: TOUCHGFX_PATH becomes GUI_PATH, and the
# *_GUI SDK variables become *_GUI_LVGL.
cmake_minimum_required(VERSION 3.21)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

# App configuration
set(APP_NAME "SensorsLVGL")
set(APP_TYPE "Activity")
set(DEV_ID "UNA")
set(APP_ID "61885BD04DBCE6DD")
set(RESOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Resources")
# Its own output directory, beside the TouchGFX build's Output/: the app
# manifest there is per app.
set(OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../OutputLVGL")
set(LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Libs")
set(GUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LVGL-GUI")

if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Point it at the SDK root.")
endif()

# Includes all build steps and required tools
# Sets toolchain before project
include($ENV{UNA_SDK}/cmake/una-app.cmake)

# Includes sets: SCRIPTS_PATH,
# UNA_SDK_SOURCES_COMMON, UNA_SDK_SOURCES_SERVICE, UNA_SDK_SOURCES_GUI_LVGL,
# UNA_SDK_INCLUDE_DIRS_COMMON, UNA_SDK_INCLUDE_DIRS_SERVICE, UNA_SDK_INCLUDE_DIRS_GUI_LVGL,
# UNA_SDK_DEFINES_GUI_LVGL
include($ENV{UNA_SDK}/cmake/una-sdk.cmake)

# Includes LIBS_SOURCES, LIBS_INCLUDE_DIRS
include(${LIBS_PATH}/libs.cmake)

# Includes GUI_APP_SOURCES, GUI_APP_INCLUDE_DIRS
include(${GUI_PATH}/lvgl-gui.cmake)

project(${APP_NAME})

# Set up version
una_app_setup_version(BUILD_VERSION ${CMAKE_CURRENT_SOURCE_DIR})

# Build service
set(SERVICE_SOURCES
    ${LIBS_SOURCES}
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_SERVICE}
)
set(SERVICE_INCLUDE_DIRS
    ${LIBS_INCLUDE_DIRS}
    ${UNA_SDK_INCLUDE_DIRS_COMMON}
    ${UNA_SDK_INCLUDE_DIRS_SERVICE}
)
una_app_build_service(${APP_NAME}Service.elf)

# Build GUI (LVGL)
set(GUI_SOURCES
    ${GUI_APP_SOURCES}
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_GUI_LVGL}
)
set(GUI_INCLUDE_DIRS
    ${LIBS_INCLUDE_DIRS}
    ${UNA_SDK_INCLUDE_DIRS_COMMON}
    ${UNA_SDK_INCLUDE_DIRS_GUI_LVGL}
    ${GUI_APP_INCLUDE_DIRS}
)
set(GUI_COMPILE_DEFINITIONS
    ${UNA_SDK_DEFINES_GUI_LVGL}
)
# LVGL's software renderer recurses deeper than TouchGFX; give the GUI task
# more stack than the 10 KB default.
set(GUI_STACK_SIZE "24*1024")
una_app_build_gui(${APP_NAME}GUI.elf)

# Build the complete app using SDK functions
una_app_build_app()
