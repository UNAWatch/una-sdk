# Watch App CMakeLists.txt - Simplified version using SDK
cmake_minimum_required(VERSION 3.21)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

# App configuration
set(APP_NAME "Treadmill")
set(APP_TYPE "Activity")
set(DEV_ID "UNA")
set(APP_ID "A1D7E4B92C8F6035")
set(RESOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Resources")
set(OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Output")
set(LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Libs")
set(TOUCHGFX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../TouchGFX-GUI")

set(FIT_SDK_VERSION "21.202.00")
if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Run SDK/Utilities/Scripts/set-una-sdk-env.ps1 or set UNA_SDK to the SDK root.")
endif()

# Includes all build steps and required tools
# Sets toolchain before project
include($ENV{UNA_SDK}/cmake/una-app.cmake)

set(UNA_FIT_SDK_DIR "$ENV{UNA_SDK}/ThirdParty/FitSDKRelease_${FIT_SDK_VERSION}")
if(NOT IS_DIRECTORY "${UNA_FIT_SDK_DIR}")
    message(FATAL_ERROR "UNA_FIT_SDK_DIR does not exist for FIT_SDK_VERSION ${FIT_SDK_VERSION}: ${UNA_FIT_SDK_DIR}")
endif()

# Includes sets: SCRIPTS_PATH,
# UNA_SDK_SOURCES_COMMON, UNA_SDK_SOURCES_SERVICE, UNA_SDK_SOURCES_GUI,
# UNA_SDK_INCLUDE_DIRS_COMMON, UNA_SDK_INCLUDE_DIRS_SERVICE, UNA_SDK_INCLUDE_DIRS_GUI
include($ENV{UNA_SDK}/cmake/una-sdk.cmake)

# Includes LIBS_SOURCES, LIBS_INCLUDE_DIRS
include(${LIBS_PATH}/libs.cmake)

# Includes TOUCHGFX_SOURCES, TOUCHGFX_INCLUDE_DIRS
if(DEFINED TOUCHGFX_PATH)
    include(${TOUCHGFX_PATH}/touchgfx.cmake)
endif()

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


if(DEFINED TOUCHGFX_PATH)
    set(GUI_SOURCES
        ${TOUCHGFX_SOURCES}
        ${UNA_SDK_SOURCES_COMMON}
        ${UNA_SDK_SOURCES_GUI}
    )
    set(GUI_INCLUDE_DIRS
        ${LIBS_INCLUDE_DIRS}
        ${UNA_SDK_INCLUDE_DIRS_COMMON}
        ${UNA_SDK_INCLUDE_DIRS_GUI}
        ${TOUCHGFX_INCLUDE_DIRS}
    )
    una_app_build_gui(${APP_NAME}GUI.elf)
endif()

# Build the complete app using SDK functions
una_app_build_app()