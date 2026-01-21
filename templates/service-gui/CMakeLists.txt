# Watch App CMakeLists.txt - Service+GUI Template
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

# Build service and GUI executables
watch_build_service(${APP_NAME}Service.elf ${LIBS_PATH} ${TOUCHGFX_GUI_PATH})
watch_build_gui(${APP_NAME}GUI.elf ${LIBS_PATH} ${TOUCHGFX_GUI_PATH})

# Scripts path
set(SCRIPTS_PATH "${UNA_SDK}/Utilities/Scripts")

# Final app merging
add_custom_target(${APP_NAME}App ALL
    DEPENDS ${APP_NAME}Service.elf ${APP_NAME}GUI.elf
    COMMAND python3 ${SCRIPTS_PATH}/app_merging/app_merging.py
        -normal_icon ${RESOURCES_PATH}/icon_60x60.png
        -small_icon ${RESOURCES_PATH}/icon_30x30.png
        -name ${APP_NAME}
        -type Activity
        -glance_capable
        -out ${CMAKE_CURRENT_BINARY_DIR}
        -appid ${APP_ID}
        -appver ${BUILD_VERSION}
        -scripts ${SCRIPTS_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/Tmp ${OUTPUT_PATH}/Tmp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}_${BUILD_VERSION}.uapp ${OUTPUT_PATH}/
    COMMENT "Merging ${APP_NAME} application"
)