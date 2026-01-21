# Watch App CMakeLists.txt - Service-Only Template
cmake_minimum_required(VERSION 3.21)
include($ENV{UNA_SDK}/cmake/watch_project.cmake)

project(MyApp)

# Load configuration from .env file
file(READ .env ENV_CONTENT)
string(REPLACE "\n" ";" ENV_LINES ${ENV_CONTENT})
foreach(line ${ENV_LINES})
    string(STRIP "${line}" line)
    if(line MATCHES "^([^=]+)=(.*)$")
        set(${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
    endif()
endforeach()

# App configuration
set(APP_ID "your_app_id_here")  # Replace with actual app ID
set(APP_NAME "MyApp")           # Replace with actual app name
set(DEV_ID "UNA")               # Developer ID

# Set up build version
watch_setup_version(BUILD_VERSION ${CMAKE_CURRENT_SOURCE_DIR})

# Make BUILD_VERSION available globally
set(BUILD_VERSION "${BUILD_VERSION}" PARENT_SCOPE)

# Build service executable
watch_build_service(${APP_NAME}Service.elf ${LIBS_PATH} "${TOUCHGFX_GUI_PATH}")

# Note: For service-only apps, no GUI component or app merging is performed